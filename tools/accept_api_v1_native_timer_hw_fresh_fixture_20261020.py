#!/usr/bin/env python3
"""Resume API-v1 timer hardware acceptance on a NEW disposable EPG fixture.

Never modify a pre-existing native timer. The currently recording
"In aller Freundschaft" timer is explicitly excluded. No blind POST retry.
Journal survives daemon restart and host reboot.
"""
import atexit
import base64
from datetime import datetime
import getpass
import json
import os
from pathlib import Path
import socket
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid

BASE = "http://127.0.0.1:18080"
V1 = BASE + "/api/v1"
BACKEND = "default"
CHANNEL = "C-1-1051-10301"
EXCLUDED = "C-1-1051-10301:0:1792447200:2100:2145"
# The first attempt on 2026-10-21 was denied at the HTTP auth gate
# (403 Verified Device credential required) and admitted no operation.
# Preserve its uncertain marker untouched; use a distinct journal for the
# intentionally authorized administrator attempt.
JOURNAL = Path("/var/lib/vdr-suite/native-timer-hw-fresh-admin-20261021.json")
TERMINAL_FAILURE = {"failed_verified", "outcome_unknown", "failed",
                    "failed_before_dispatch", "rejected", "cancelled", "canceled"}


def stop(message):
    print("STOP=" + message, flush=True)
    raise SystemExit(1)


def save():
    tmp = JOURNAL.with_name(JOURNAL.name + ".tmp")
    fd = os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "w") as file:
        json.dump(journal, file, sort_keys=True)
        file.flush()
        os.fsync(file.fileno())
    os.replace(tmp, JOURNAL)


def login():
    username = input("VDR-Suite Admin-Benutzer [vdr-suite]: ").strip() or "vdr-suite"
    password = getpass.getpass("VDR-Suite Admin-Passwort: ")
    if ":" in username or not password:
        stop("LOGIN_INPUT_INVALID")
    encoded = base64.b64encode((username + ":" + password).encode()).decode()
    del password
    req = urllib.request.Request(
        BASE + "/api/security/browser-sessions",
        data=b"{}", method="POST",
        headers={"Authorization": "Basic " + encoded,
                 "Content-Type": "application/json", "Accept": "application/json",
                 "Origin": BASE})
    try:
        with urllib.request.urlopen(req, timeout=8) as response:
            cookie = response.headers.get("Set-Cookie", "").split(";", 1)[0]
            payload = json.load(response)
    except (OSError, ValueError, urllib.error.URLError):
        stop("ADMIN_LOGIN_FAILED")
    csrf = payload.get("csrfToken", "")
    if not cookie.startswith("vdr_suite_session=") or not csrf:
        stop("ADMIN_SESSION_INVALID")
    headers = {"Cookie": cookie, "X-CSRF-Token": csrf, "Origin": BASE}
    def logout():
        try:
            req = urllib.request.Request(
                BASE + "/api/security/browser-sessions/logout",
                method="POST", data=b"{}",
                headers={**headers, "Content-Type": "application/json"})
            urllib.request.urlopen(req, timeout=3).close()
        except (OSError, urllib.error.URLError):
            pass
    atexit.register(logout)
    print("AUTH=ADMIN_SESSION_OK")
    return headers


def call(method, path, body=None, etag=None, idem=None, public=True):
    url = (V1 if public else BASE) + path
    headers = {**auth, "Accept": "application/json"}
    data = None
    if body is not None:
        data = json.dumps(body, separators=(",", ":")).encode()
        headers["Content-Type"] = "application/json"
    if method != "GET":
        if not etag or not idem:
            stop("MUTATION_REQUIRES_ETAG_AND_JOURNALED_IDEMPOTENCY")
        headers["If-Match"] = etag
        headers["Idempotency-Key"] = idem
    req = urllib.request.Request(url, headers=headers, data=data, method=method)
    try:
        with urllib.request.urlopen(req, timeout=8) as response:
            raw = response.read()
            return response.status, response.headers, json.loads(raw) if raw else {}
    except urllib.error.HTTPError as error:
        return error.code, error.headers, {"error": error.read().decode(errors="replace")[:400]}


def get(path, public=True):
    try:
        return call("GET", path, public=public)
    except (OSError, socket.timeout, ValueError, urllib.error.URLError) as error:
        stop("READ_FAILED_" + type(error).__name__)


def timers():
    code, _, body = get("/native-timers?backendId=default&limit=30&offset=0")
    if code != 200 or not isinstance(body.get("items"), list):
        print("LIST_HTTP=", code, "DETAIL=", body)
        stop("PUBLIC_TIMER_LIST_UNAVAILABLE")
    return body["items"]


def detail(timer_id):
    return get("/native-timers/" + urllib.parse.quote(timer_id, safe="") +
               "?backendId=default")


def valid_detail(timer_id, title, enabled):
    if timer_id == EXCLUDED:
        stop("EXCLUDED_RECORDING_TIMER")
    for attempt in range(31):
        status, headers, item = detail(timer_id)
        if (status == 200 and item.get("timerId") == timer_id and
            item.get("title") == title and item.get("enabled") is enabled and
            item.get("ownership") == "managed" and
            item.get("bindingState") == "verified" and
            item.get("recording") is False and
            item.get("pending") is False):
            etag = headers.get("ETag")
            if etag and etag.startswith('"') and not etag.startswith('W/'):
                return item, etag
        if status not in (200, 503):
            print("DETAIL_HTTP=", status, "BODY=", item)
            stop("DETAIL_NOT_VERIFIED")
        if attempt != 30:
            time.sleep(2)
    print("DETAIL_HTTP=", status, "BODY=", item)
    stop("DETAIL_UNVERIFIED_OR_TIMER_RECORDING_NO_MUTATION")


def operation(stage):
    record = journal["stages"][stage]
    if record["status"] == "unknown":
        stop(stage + "_POSSIBLY_SUBMITTED_NO_RETRY")
    opid = record["operationId"]
    last = None
    for attempt in range(45):
        code, _, body = get("/operations/" + opid)
        if code != 200 or body.get("operationId") != opid:
            print("OPERATION_HTTP=", code, "BODY=", body)
            stop(stage + "_OPERATION_UNREADABLE_NO_RETRY")
        state = body.get("state")
        if state != last:
            print(stage, "OPERATION=", opid, "STATE=", state, flush=True)
            last = state
        if state == "succeeded":
            return
        if state in TERMINAL_FAILURE:
            stop(stage + "_" + str(state).upper() + "_NO_RETRY")
        if attempt != 44:
            time.sleep(2)
    stop(stage + "_NOT_TERMINAL_NO_RETRY")


def submit(stage, method, path, body, etag):
    record = journal["stages"].get(stage)
    if record is None:
        idem = "hw-fresh-" + uuid.uuid4().hex
        journal["stages"][stage] = {"status": "unknown", "idempotencyKey": idem}
        save()  # durable BEFORE any modifying HTTP request
        try:
            code, _, reply = call(method, path, body, etag, idem)
        except (OSError, socket.timeout, ValueError, urllib.error.URLError) as e:
            print("POST_ERROR=", type(e).__name__)
            stop(stage + "_RESULT_UNKNOWN_NO_RETRY")
        if code != 202 or not isinstance(reply.get("operationId"), str):
            print(stage, "HTTP=", code, "REPLY=", reply)
            stop(stage + "_NO_ACCEPTANCE_NO_RETRY")
        record = {"status": "accepted", "operationId": reply["operationId"],
                  "idempotencyKey": idem}
        journal["stages"][stage] = record
        save()
    else:
        print(stage, "REUSE_OPERATION=", record.get("operationId", "<unknown>"))
    operation(stage)


def choose_candidate():
    now = int(time.time())
    before = timers()
    ids = [t.get("timerId") for t in before]
    if not ids or len(ids) != len(set(ids)):
        stop("INVALID_EXISTING_TIMER_INVENTORY")
    for slot in range(14):
        start = now + 6 * 3600 + slot * 3 * 3600
        params = urllib.parse.urlencode({
            "backendId": BACKEND, "channelId": CHANNEL,
            "fromTime": str(start), "untilTime": str(start + 3 * 3600 - 1),
            "limit": "192"})
        code, _, events = get("/epg/window?" + params)
        if code != 200:
            continue
        for event in sorted(events.get("items", []),
                            key=lambda item: item.get("startTime", "")):
            try:
                event_start = int(event.get("startTime", 0))
                duration = int(event.get("durationSeconds", 0))
            except (TypeError, ValueError):
                continue
            event_id = event.get("eventId")
            if (not event_id or not event.get("title") or
                event_start < now + 6 * 3600 or duration < 600):
                continue
            local_start = datetime.fromtimestamp(event_start)
            day, hhmm = local_start.strftime("%Y-%m-%d"), local_start.strftime("%H%M")
            if any(t.get("channelId") == CHANNEL and t.get("day") == day and
                   str(t.get("startTime", "")).zfill(4) == hhmm for t in before):
                continue
            event_params = urllib.parse.urlencode({
                "backendId": BACKEND, "channelId": CHANNEL, "eventId": event_id})
            code, headers, item = get("/epg/event?" + event_params)
            tag = headers.get("ETag")
            if (code == 200 and item.get("eventId") == event_id and
                item.get("channelId") == CHANNEL and tag and
                tag.startswith('"') and not tag.startswith('W/')):
                return {"beforeIds": ids, "event": item, "etag": tag,
                        "day": day, "startHHMM": hhmm}
    stop("NO_SAFE_FUTURE_EPG_EVENT_FOR_DISPOSABLE_TEST_TIMER")


def find_fixture():
    c = journal["candidate"]
    known_ids = set(c["beforeIds"])
    event = c["event"]
    for attempt in range(45):
        items = timers()
        matches = [
            t for t in items if
            t.get("timerId") not in known_ids and
            t.get("timerId") != EXCLUDED and
            t.get("channelId") == CHANNEL and
            t.get("title") == event["title"] and
            t.get("day") == c["day"] and
            str(t.get("startTime", "")).zfill(4) == c["startHHMM"]
        ]
        if len(matches) == 1:
            fixture = matches[0]["timerId"]
            journal["timerId"] = fixture
            journal["initialTitle"] = event["title"]
            save()
            print("NEW_DISPOSABLE_TIMER=", fixture, "TITLE=", event["title"])
            return fixture
        if len(matches) > 1:
            stop("AMBIGUOUS_CREATED_TEST_TIMER")
        if attempt != 44:
            time.sleep(2)
    stop("CREATED_TIMER_NOT_VISIBLE_NO_CREATE_RETRY")


if JOURNAL.exists():
    journal = json.loads(JOURNAL.read_text())
    if journal.get("backendId") != BACKEND or journal.get("channelId") != CHANNEL:
        stop("UNEXPECTED_JOURNAL")
else:
    journal = {"backendId": BACKEND, "channelId": CHANNEL, "stages": {}}
    save()

auth = login()
print("ACCEPTANCE=FRESH_FUTURE_DISPOSABLE_TIMER_ADMIN")
print("PREVIOUS_CREATE=HTTP_403_REJECTED_NO_OPERATION")
print("OLD_RECORDING_TIMER=EXCLUDED")

if "candidate" not in journal:
    journal["candidate"] = choose_candidate()
    save()

if "CREATE" not in journal["stages"]:
    c = journal["candidate"]
    event = c["event"]
    print("CREATE_FIXTURE_EVENT=", event["title"], "AT=", event["startTime"])
    submit("CREATE", "POST", "/epg/timer-intents",
           {"backendId": BACKEND, "channelId": CHANNEL,
            "eventId": event["eventId"]}, c["etag"])
else:
    operation("CREATE")

timer_id = journal.get("timerId") or find_fixture()
if timer_id == EXCLUDED or timer_id in journal["candidate"]["beforeIds"]:
    stop("NOT_A_NEW_DISPOSABLE_FIXTURE")

title = journal["initialTitle"]
updated = title + "-APIv1-HW"
path = "/native-timers/" + urllib.parse.quote(timer_id, safe="") + \
       "/modifications?backendId=default"

if "UPDATE" not in journal["stages"]:
    item, etag = valid_detail(timer_id, title, True)
    update = {
        "kind": "update", "channelId": item["channelId"],
        "title": updated, "directory": item.get("directory", ""),
        "day": item["day"], "weekdays": item["weekdays"],
        "startTime": item["startTime"], "endTime": item["endTime"],
        "priority": 50, "lifetime": 99,
        "enabled": True, "vps": False}
    submit("UPDATE", "POST", path, update, etag)
else:
    operation("UPDATE")

if "TOGGLE_OFF" not in journal["stages"]:
    _, etag = valid_detail(timer_id, updated, True)
    submit("TOGGLE_OFF", "POST", path,
           {"kind": "toggle", "enabled": False}, etag)
else:
    operation("TOGGLE_OFF")
print("TOGGLE_OFF=PASS")

if "TOGGLE_ON" not in journal["stages"]:
    _, etag = valid_detail(timer_id, updated, False)
    submit("TOGGLE_ON", "POST", path,
           {"kind": "toggle", "enabled": True}, etag)
else:
    operation("TOGGLE_ON")
print("TOGGLE_ON=PASS")

if "DELETE" not in journal["stages"]:
    _, etag = valid_detail(timer_id, updated, True)
    submit("DELETE", "DELETE",
           "/native-timers/" + urllib.parse.quote(timer_id, safe="") +
           "?backendId=default", None, etag)
else:
    operation("DELETE")

for attempt in range(20):
    code, _, item = detail(timer_id)
    if code == 404:
        print("DELETE=PASS")
        print("HARDWARE_ACCEPTANCE=PASS CREATE UPDATE TOGGLE_OFF TOGGLE_ON DELETE")
        break
    if attempt != 19:
        time.sleep(2)
else:
    print("DELETE_READBACK_HTTP=", code, "BODY=", item)
    stop("DELETE_NOT_VERIFIED")
