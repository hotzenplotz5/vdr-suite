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


print("ACCEPTANCE=API_V1_NATIVE_TIMER_NO_CREATE", flush=True)
code, _, collection = call("GET", "/native-timers?backendId=default&limit=30&offset=0")
require_ok(code, collection)
matches = [item for item in collection.get("items", []) if item.get("title") == TITLE]
if len(matches) != 1:
    print("FOUND_TEST_TIMERS=", len(matches), flush=True)
    stop("TEST_TIMER_NOT_UNIQUE")
timer_id = matches[0]["timerId"]
if not timer_id or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:._-" for c in timer_id):
    stop("TIMER_ID_NOT_CANONICAL")
print("TEST_TIMER_ID=", timer_id, flush=True)
item, etag = verified(timer_id)
if item.get("recording") or item.get("pending"):
    stop("TEST_TIMER_UNSAFE")
lstt("BEFORE")
print("DETAIL_BEFORE=", json.dumps(item, ensure_ascii=False), flush=True)

# Explicit full UPDATE: all required API-v1 fields, limited to this disposable test timer.
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
item, etag = mutation("UPDATE", timer_id, update, etag, UPDATED_TITLE, item["enabled"])
item, etag = mutation("TOGGLE_OFF", timer_id, {"kind": "toggle", "enabled": False}, etag, UPDATED_TITLE, False)
item, etag = mutation("TOGGLE_ON", timer_id, {"kind": "toggle", "enabled": True}, etag, UPDATED_TITLE, True)

code, _, operation = call("DELETE", "/native-timers/" + timer_id + "?backendId=" + BACKEND, etag=etag)
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
if TITLE in out:
    stop("VDR_NATIVE_TIMER_STILL_PRESENT")
print("DELETE=PASS", flush=True)
print("HARDWARE_ACCEPTANCE=PASS UPDATE TOGGLE_OFF TOGGLE_ON DELETE", flush=True)
