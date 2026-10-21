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
import sqlite3
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
# outcome_unknown is not terminal: the daemon may reconcile this exact
# command via read-only native readback and close it as succeeded/failed_verified.
TERMINAL_FAILURE = {"failed_verified", "failed",
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
    return get("/native-timers/" + urllib.parse.quote(timer_id, safe=":._-") +
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


def diagnostic_readback(opid):
    # Only inspect the existing operation, stored result and the one owned
    # test timer. No credential, bearer token or command payload is logged.
    try:
        with sqlite3.connect(
                "file:/var/lib/vdr-suite/vdr-suite.db?mode=ro",
                uri=True) as db:
            db.execute("PRAGMA query_only=ON")
            op = db.execute(
                "SELECT state, result_reference, backend_generation "
                "FROM mutation_operations WHERE operation_id=?", (opid,)
            ).fetchone()
            print("UNRESOLVED_OPERATION_DB=", op, flush=True)
            rows = db.execute(
                "SELECT c.state, r.result_category, r.error_category, "
                "r.dispatch_state, r.verification_state, "
                "length(r.result_evidence) "
                "FROM backend_agent_commands c "
                "LEFT JOIN backend_agent_command_results r "
                "ON r.command_id=c.command_id "
                "WHERE c.operation_id=?", (opid,)
            ).fetchall()
            print("UNRESOLVED_COMMAND_RESULTS=", rows, flush=True)
            bound = db.execute(
                "SELECT backend_native_timer_id, observed_title, "
                "observed_directory, observed_enabled, observed_recording, "
                "observed_pending, last_verified_operation_id "
                "FROM native_timer_bindings "
                "WHERE native_timer_binding_id=("
                "SELECT resource_id FROM mutation_operations "
                "WHERE operation_id=?)", (opid,)
            ).fetchone()
            print("UNRESOLVED_BINDING=", bound, flush=True)
    except (sqlite3.Error, OSError) as error:
        print("UNRESOLVED_DB_DIAGNOSTIC=", type(error).__name__, flush=True)
    try:
        with urllib.request.urlopen(
                "http://127.0.0.1:8002/timers.json",
                timeout=8) as response:
            raw = json.load(response)
        listed = raw.get("timers", []) if isinstance(raw, dict) else raw
        timer_id = journal.get("timerId")
        for timer in listed:
            if timer.get("id") == timer_id:
                print("UNRESOLVED_LIVE_TIMER=",
                      {field: timer.get(field) for field in
                       ("id", "filename", "flags", "is_active",
                        "is_recording", "is_pending")}, flush=True)
                break
        else:
            print("UNRESOLVED_LIVE_TIMER=NOT_FOUND", flush=True)
    except (OSError, ValueError, urllib.error.URLError) as error:
        print("UNRESOLVED_LIVE_READ=", type(error).__name__, flush=True)


def same_timer_fingerprint_except_epg_event_id(original, current):
    """Compare all 15 canonical native timer fields except transient eventId."""
    prefix = b"native-timer-observed-state/1|"

    def decode(value):
        if not isinstance(value, str):
            return None
        data = value.encode("utf-8")
        if not data.startswith(prefix) or len(data) > 4096:
            return None
        pos, fields = len(prefix), []
        while pos < len(data) and len(fields) < 15:
            colon = data.find(b":", pos)
            if colon < 0 or not data[pos:colon].isdigit():
                return None
            size = int(data[pos:colon])
            start, end = colon + 1, colon + 1 + size
            if end >= len(data) or data[end:end + 1] != b"|":
                return None
            fields.append(data[start:end])
            pos = end + 1
        return fields if pos == len(data) and len(fields) == 15 else None

    before = decode(original)
    after = decode(current)
    return before is not None and after is not None and (
        before[0] == after[0] and before[2:] == after[2:])


def update_no_effect_verified(opid):
    """Read-only proof that the original UPDATE terminated without effect."""
    try:
        with sqlite3.connect(
                "file:/var/lib/vdr-suite/vdr-suite.db?mode=ro", uri=True) as db:
            db.execute("PRAGMA query_only=ON")
            row = db.execute(
                "SELECT o.state, o.result_reference, "
                "o.expected_resource_fingerprint, b.observed_fingerprint, "
                "b.last_verified_operation_id, b.backend_native_timer_id, "
                "b.observed_title, b.observed_recording, b.observed_pending "
                "FROM mutation_operations o "
                "JOIN native_timer_bindings b ON b.native_timer_binding_id = "
                "o.resource_id WHERE o.operation_id=?", (opid,)
            ).fetchone()
    except (sqlite3.Error, OSError):
        return False
    return bool(
        row and row[0] == "failed_verified" and
        row[1] == "native-timer-modify:unchanged-after-uncertain-execution" and
        same_timer_fingerprint_except_epg_event_id(row[2], row[3]) and
        row[4] != opid and
        row[5] == journal.get("timerId") and
        row[6] == journal.get("initialTitle") and
        row[7] == 0 and row[8] == 0
    )


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
            if state == "outcome_unknown":
                print(stage, "READBACK_RECONCILIATION=PENDING_NO_REDISPATCH",
                      flush=True)
            last = state
        if state == "succeeded":
            return
        if state == "failed_verified" and stage == "UPDATE":
            if update_no_effect_verified(opid):
                journal["updateNoEffectVerified"] = True
                save()
                print("UPDATE=FAILED_VERIFIED_NO_EFFECT_KEEP_ORIGINAL_TITLE",
                      flush=True)
                return
        if state in TERMINAL_FAILURE:
            if state == "failed_verified":
                diagnostic_readback(opid)
            stop(stage + "_" + str(state).upper() + "_NO_RETRY")
        if attempt != 44:
            time.sleep(2)
    diagnostic_readback(opid)
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
if not timer_id or not all(c.isascii() and (c.isalnum() or c in ":._-")
                           for c in timer_id):
    stop("INVALID_NATIVE_TIMER_ID")

title = journal["initialTitle"]
updated = title + "-APIv1-HW"
path = "/native-timers/" + urllib.parse.quote(timer_id, safe=":._-") + \
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

# The earlier hardware fixture already proved a successful UPDATE. When
# *this* uncertain UPDATE has now been verified with no side effect, keep
# the same disposable timer for the independent TOGGLE/DELETE checks.
# Never declare the fresh-fixture UPDATE a success in this case.
toggle_title = title if journal.get("updateNoEffectVerified") else updated

if "TOGGLE_OFF" not in journal["stages"]:
    _, etag = valid_detail(timer_id, toggle_title, True)
    submit("TOGGLE_OFF", "POST", path,
           {"kind": "toggle", "enabled": False}, etag)
else:
    operation("TOGGLE_OFF")
print("TOGGLE_OFF=PASS")

if "TOGGLE_ON" not in journal["stages"]:
    _, etag = valid_detail(timer_id, toggle_title, False)
    submit("TOGGLE_ON", "POST", path,
           {"kind": "toggle", "enabled": True}, etag)
else:
    operation("TOGGLE_ON")
print("TOGGLE_ON=PASS")

if "DELETE" not in journal["stages"]:
    _, etag = valid_detail(timer_id, toggle_title, True)
    submit("DELETE", "DELETE",
           "/native-timers/" + urllib.parse.quote(timer_id, safe=":._-") +
           "?backendId=default", None, etag)
else:
    operation("DELETE")

for attempt in range(20):
    code, _, item = detail(timer_id)
    if code == 404:
        print("DELETE=PASS")
        if journal.get("updateNoEffectVerified"):
            print("HARDWARE_ACCEPTANCE=PASS CREATE TOGGLE_OFF TOGGLE_ON DELETE; "
                  "UPDATE=FAILED_VERIFIED_NO_EFFECT (prior fixture UPDATE=PASS)")
        else:
            print("HARDWARE_ACCEPTANCE=PASS CREATE UPDATE TOGGLE_OFF TOGGLE_ON DELETE")
        break
    if attempt != 19:
        time.sleep(2)
else:
    print("DELETE_READBACK_HTTP=", code, "BODY=", item)
    stop("DELETE_NOT_VERIFIED")
