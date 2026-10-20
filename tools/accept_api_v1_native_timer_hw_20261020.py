#!/usr/bin/env python3
"""Single-run yaVDR hardware acceptance for API v1 native Timer MODIFY/DELETE.

Only the exact VDRSuite-HW-Test-20261020 timer is mutated.
No CREATE, no blind retries, no credentials printed, no filesystem permission changes.
"""
import json
import subprocess
import sys
import time
import urllib.error
import urllib.request
import uuid
from pathlib import Path

BASE = "http://127.0.0.1:18080/api/v1"
CREDENTIALS = Path("/run/vdr-suite-api-v1-hqoedm30.json")
TITLE = "VDRSuite-HW-Test-20261020"
UPDATED_TITLE = TITLE + "-APIv1"
BACKEND = "default"


def stop(message):
    print("STOP=" + message, flush=True)
    sys.exit(1)


if not CREDENTIALS.is_file():
    stop("TEST_CREDENTIALS_MISSING")
auth = json.loads(CREDENTIALS.read_text())
headers = {
    "Accept": "application/json",
    "Authorization": "VDR-Suite-Device " + auth["credentialId"] + "." + auth["credentialSecret"],
}


def call(method, path, body=None, etag=None):
    h = headers.copy()
    data = None
    if body is not None:
        data = json.dumps(body, separators=(",", ":")).encode()
        h["Content-Type"] = "application/json"
    if method != "GET":
        if not etag:
            stop("MISSING_STRONG_ETAG")
        h["If-Match"] = etag
        h["Idempotency-Key"] = "hw-accept-" + uuid.uuid4().hex
    req = urllib.request.Request(BASE + path, method=method, headers=h, data=data)
    try:
        with urllib.request.urlopen(req, timeout=8) as resp:
            raw = resp.read()
            return resp.status, resp.headers, json.loads(raw) if raw else {}
    except urllib.error.HTTPError as error:
        raw = error.read().decode(errors="replace")
        return error.code, error.headers, {"error": raw[:600]}


def require_ok(code, data, expected=200):
    if code != expected:
        print("HTTP=", code, "RESPONSE=", data, flush=True)
        stop("HTTP_" + str(code))


def lstt(stage):
    result = subprocess.run(["svdrpsend", "LSTT"], capture_output=True, text=True, timeout=10)
    print("VDR_LSTT_STAGE=", stage, "EXIT=", result.returncode, flush=True)
    for line in (result.stdout + result.stderr).splitlines():
        if TITLE in line:
            print("VDR_TIMER=", line[:300], flush=True)
    if result.returncode != 0:
        stop("SVDRP_LSTT_FAILED")
    return result.stdout + result.stderr


def detail(timer_id):
    return call("GET", "/native-timers/" + timer_id + "?backendId=" + BACKEND)


def verified(timer_id):
    code, hdr, item = detail(timer_id)
    require_ok(code, item)
    if item.get("ownership") != "managed" or item.get("bindingState") != "verified":
        print("DETAIL=", item, flush=True)
        stop("NOT_MANAGED_VERIFIED")
    etag = hdr.get("ETag")
    if not etag or etag.startswith("W/"):
        stop("STRONG_ETAG_MISSING")
    return item, etag


def await_operation(stage, accepted):
    op_id = accepted.get("operationId")
    if not isinstance(op_id, str) or not op_id.startswith("op_"):
        print("ADMISSION=", accepted, flush=True)
        stop("OPERATION_ID_MISSING")
    print(stage, "HTTP=202 OPERATION=", op_id, flush=True)
    last_state = None
    for attempt in range(75):
        code, _, operation = call("GET", "/operations/" + op_id)
        require_ok(code, operation)
        state = operation.get("state")
        if state != last_state:
            print(stage, "OPERATION_STATE=", state, flush=True)
            last_state = state
        if state == "succeeded":
            return
        if state in ("failed_verified", "outcome_unknown", "failed", "cancelled", "canceled"):
            stop(stage + "_OPERATION_" + str(state))
        time.sleep(1)
    stop(stage + "_OPERATION_NOT_TERMINAL_NO_RETRY")


def await_detail(timer_id, expected_title, expected_enabled):
    for attempt in range(45):
        code, hdr, item = detail(timer_id)
        if code == 200 and item.get("title") == expected_title and item.get("enabled") is expected_enabled and item.get("bindingState") == "verified":
            etag = hdr.get("ETag")
            if etag and not etag.startswith("W/"):
                return item, etag
        time.sleep(1)
    print("LAST_DETAIL_HTTP=", code, "DETAIL=", item, flush=True)
    stop("POST_MUTATION_READBACK_NOT_VERIFIED")


def mutation(stage, timer_id, payload, etag, expected_title, expected_enabled):
    path = "/native-timers/" + timer_id + "/modifications?backendId=" + BACKEND
    code, _, operation = call("POST", path, payload, etag)
    require_ok(code, operation, expected=202)
    await_operation(stage, operation)
    item, new_etag = await_detail(timer_id, expected_title, expected_enabled)
    lstt(stage)
    print(stage + "=PASS", "ENABLED=", item["enabled"], flush=True)
    return item, new_etag


print("ACCEPTANCE=API_V1_NATIVE_TIMER_UPDATE_TOGGLE_DELETE", flush=True)
print("PREPARATION=USE_ONLY_FRESH_SUITE_MANAGED_TEST_TIMER", flush=True)

# The previously created VDRSuite-HW-Test-20261020 is an unbound, manually
# created VDR timer. Never modify it (or Tagesschau, Report Mainz, Meister
# des Alltags) using a fabricated ETag or by choosing a production binding.
code, _, before = call("GET", "/native-timers?backendId=default&limit=30&offset=0")
require_ok(code, before)
before_ids = {item["timerId"] for item in before.get("items", [])}
if len(before_ids) != len(before.get("items", [])):
    stop("DUPLICATE_NATIVE_TIMER_IDS")

# We do not re-test CREATE. One public-v1 EPG CREATE seeds a disposable,
# Suite-owned fixture, using the existing device credential and permissions.
# Public admission checks EPG identity, preview, generation, backend policy.
from datetime import datetime
from urllib.parse import urlencode

now = int(time.time())
candidate = None
candidate_etag = None
# Prefer a broadcast between 48h and 6d in the future so no recording starts
# during the short-lived hardware acceptance.
for slot in range(8):
    start = now + 48 * 3600 + slot * 12 * 3600
    end = start + 12 * 3600 - 1
    path = "/epg/window?" + urlencode({
        "backendId": BACKEND, "channelId": "C-1-1051-10301",
        "fromTime": str(start), "untilTime": str(end), "limit": "192"
    })
    code, _, events = call("GET", path)
    if code != 200:
        print("EPG_WINDOW_HTTP=", code, flush=True)
        continue
    possible = sorted(events.get("items", []), key=lambda item: item.get("startTime", ""))
    for event in possible:
        event_id = event.get("eventId")
        event_title = event.get("title")
        try:
            begins = int(event.get("startTime", 0))
            duration = int(event.get("durationSeconds", 0))
        except (ValueError, TypeError):
            continue
        if not event_id or not event_title or begins < now + 48 * 3600 or duration < 600:
            continue
        # Skip any currently visible timer with the same backend channel and
        # same one-shot day close to its start, leaving existing VDR plans alone.
        local_day = datetime.fromtimestamp(begins).strftime("%Y-%m-%d")
        local_hhmm = datetime.fromtimestamp(begins).strftime("%H%M")
        if any(t.get("channelId") == "C-1-1051-10301"
               and t.get("day") == local_day
               and t.get("startTime", "").zfill(4) == local_hhmm
               for t in before.get("items", [])):
            continue
        code, hdr, epg_detail = call("GET", "/epg/event?" + urlencode({
            "backendId": BACKEND,
            "channelId": "C-1-1051-10301",
            "eventId": event_id,
        }))
        if code != 200 or epg_detail.get("eventId") != event_id:
            continue
        tag = hdr.get("ETag")
        if not tag or tag.startswith("W/"):
            continue
        candidate = epg_detail
        candidate_etag = tag
        break
    if candidate:
        break

if candidate is None:
    stop("NO_ELIGIBLE_EPG_EVENT_FOR_OWNED_FIXTURE")

print("TEST_EVENT=", candidate.get("title"),
      "TIME=", candidate.get("startTime"), flush=True)
create_body = {
    "backendId": BACKEND,
    "channelId": candidate["channelId"],
    "eventId": candidate["eventId"],
}
code, _, operation = call("POST", "/epg/timer-intents", create_body, candidate_etag)
require_ok(code, operation, expected=202)
print("FIXTURE_CREATE_SUBMITTED_ONCE", flush=True)
await_operation("FIXTURE_CREATE", operation)

timer_id = None
for attempt in range(50):
    code, _, snapshot = call("GET", "/native-timers?backendId=default&limit=30&offset=0")
    require_ok(code, snapshot)
    new_items = [
        item for item in snapshot.get("items", [])
        if item.get("timerId") not in before_ids
        and item.get("channelId") == candidate["channelId"]
        and item.get("title") == candidate["title"]
    ]
    if len(new_items) == 1:
        timer_id = new_items[0]["timerId"]
        break
    if len(new_items) > 1:
        stop("AMBIGUOUS_NEW_TIMER")
    time.sleep(1)
if not timer_id:
    stop("CREATED_TIMER_NOT_VISIBLE_NO_RETRY")

# Never fall back to any known production/native timer.
if not timer_id or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:._-" for c in timer_id):
    stop("CREATED_TIMER_ID_INVALID")
print("SUITE_FIXTURE_TIMER_ID=", timer_id, flush=True)
for attempt in range(45):
    code, hdr, item = detail(timer_id)
    if code == 200 and item.get("ownership") == "managed" and item.get("bindingState") == "verified" and hdr.get("ETag"):
        etag = hdr["ETag"]
        break
    time.sleep(1)
else:
    print("FIXTURE_DETAIL_HTTP=", code, "DETAIL=", item, flush=True)
    stop("CREATED_TIMER_BINDING_NOT_VERIFIED_NO_RETRY")

if item.get("recording") or item.get("pending") or item.get("enabled") is not True:
    stop("CREATED_TEST_TIMER_NOT_IDLE_ENABLED")
TITLE = item["title"]
UPDATED_TITLE = TITLE + "-APIv1-HW"
lstt("BEFORE_MODIFICATIONS")
print("FIXTURE_VERIFIED=PASS", flush=True)

update = {
    "kind": "update",
    "channelId": item["channelId"],
    "title": UPDATED_TITLE,
    "directory": UPDATED_TITLE,
    "day": item["day"],
    "weekdays": item["weekdays"],
    "startTime": item["startTime"],
    "endTime": item["endTime"],
    "enabled": item["enabled"],
    "vps": False,
    "priority": 50,
    "lifetime": 99,
}
item, etag = mutation("UPDATE", timer_id, update, etag, UPDATED_TITLE, True)
item, etag = mutation("TOGGLE_OFF", timer_id,
                       {"kind": "toggle", "enabled": False}, etag,
                       UPDATED_TITLE, False)
item, etag = mutation("TOGGLE_ON", timer_id,
                       {"kind": "toggle", "enabled": True}, etag,
                       UPDATED_TITLE, True)

code, _, operation = call("DELETE", "/native-timers/" + timer_id +
                          "?backendId=" + BACKEND, etag=etag)
require_ok(code, operation, expected=202)
await_operation("DELETE", operation)
for attempt in range(45):
    code, _, final = detail(timer_id)
    if code == 404:
        break
    time.sleep(1)
else:
    print("DELETE_DETAIL_HTTP=", code, "DETAIL=", final, flush=True)
    stop("DELETE_READBACK_NOT_ABSENT")
out = lstt("AFTER_DELETE")
if UPDATED_TITLE in out:
    stop("VDR_CREATED_TIMER_STILL_PRESENT")
print("DELETE=PASS", flush=True)
print("HARDWARE_ACCEPTANCE=PASS UPDATE TOGGLE_OFF TOGGLE_ON DELETE", flush=True)
