#!/usr/bin/env python3
"""Resume only the accepted 2026-10-20 native timer UPDATE hardware acceptance.

No CREATE or UPDATE is submitted. Each later mutation has a durable local
one-shot journal: restart may poll the saved operation but never re-POST it.
"""
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request
import uuid

BASE = "http://127.0.0.1:18080/api/v1"
AUTH_PATH = Path("/run/vdr-suite-api-v1-hqoedm30.json")
JOURNAL = Path("/run/vdr-suite-timer-hw-acceptance-resume-20261020.json")
TIMER_ID = "C-1-1051-10301:0:1792447200:2100:2145"
UPDATE_OP = "op_cb041fdd3bc9a5568bd5884a5f39953b"
ORIGINAL = "In aller Freundschaft (1152)"
UPDATED = ORIGINAL + "-APIv1-HW"


def stop(reason):
    print("STOP=" + reason, flush=True)
    sys.exit(1)


if not AUTH_PATH.is_file():
    stop("EXISTING_DEVICE_CREDENTIALS_NOT_FOUND")
creds = json.loads(AUTH_PATH.read_text())
auth = "VDR-Suite-Device " + creds["credentialId"] + "." + creds["credentialSecret"]


def request(method, path, body=None, tag=None, key=None):
    hdr = {"Authorization": auth, "Accept": "application/json"}
    payload = None
    if body is not None:
        payload = json.dumps(body, separators=(",", ":")).encode("utf-8")
        hdr["Content-Type"] = "application/json"
    if method != "GET":
        if not tag or not key:
            stop("MISSING_MUTATION_PRECONDITION")
        hdr["If-Match"] = tag
        hdr["Idempotency-Key"] = key
    req = urllib.request.Request(BASE + path, data=payload, headers=hdr, method=method)
    try:
        with urllib.request.urlopen(req, timeout=5) as resp:
            raw = resp.read()
            return resp.status, dict(resp.headers), json.loads(raw) if raw else {}
    except urllib.error.HTTPError as e:
        return e.code, dict(e.headers), {"error": e.read().decode("utf-8", "replace")[:500]}


def safe_get(path):
    try:
        return request("GET", path)
    except (OSError, TimeoutError, socket.timeout) as e:
        stop("GET_UNAVAILABLE_" + type(e).__name__ + "_NO_MUTATION_RETRY")


def get_operation(operation_id):
    code, _, body = safe_get("/operations/" + operation_id)
    if code != 200:
        print("OPERATION_HTTP=", code, "RESPONSE=", body, flush=True)
        stop("OPERATION_READ_FAILED")
    if body.get("operationId") != operation_id or body.get("backendId") != "default":
        stop("OPERATION_IDENTITY_MISMATCH")
    return body["state"]


def completed(label, operation_id):
    for iteration in range(15):
        state = get_operation(operation_id)
        if iteration == 0 or state != previous:
            print(label, "OPERATION=", operation_id, "STATE=", state, flush=True)
        if state == "succeeded":
            return
        if state in ("outcome_unknown", "failed_verified", "failed_before_dispatch",
                     "failed", "rejected", "cancelled", "canceled"):
            stop(label + "_OPERATION_" + state.upper() + "_NO_RETRY")
        previous = state
        if iteration != 14:
            time.sleep(2)
    stop(label + "_STILL_" + state.upper() + "_RECHECK_LATER")


def read_timer(expected_title, expected_enabled):
    for _ in range(5):
        code, hdr, body = safe_get(
            "/native-timers/" + TIMER_ID + "?backendId=default")
        if code == 200 and body.get("timerId") == TIMER_ID and (
            body.get("title") == expected_title and
            body.get("enabled") is expected_enabled and
            body.get("ownership") == "managed" and
            body.get("bindingState") == "verified" and
            not body.get("recording") and not body.get("pending")
        ):
            tag = hdr.get("ETag")
            if tag and tag.startswith('"') and not tag.startswith('W/'):
                print("TIMER_READBACK=PASS", "TITLE=", expected_title,
                      "ENABLED=", expected_enabled, flush=True)
                return tag
        time.sleep(1)
    print("TIMER_DETAIL_HTTP=", code, "DETAIL=", body, flush=True)
    stop("TEST_TIMER_READBACK_NOT_VERIFIED_NO_RETRY")


def persist(journal):
    tmp = JOURNAL.with_name(JOURNAL.name + ".tmp")
    fd = os.open(str(tmp), os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    with os.fdopen(fd, "w") as stream:
        json.dump(journal, stream, sort_keys=True)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(tmp, JOURNAL)


if JOURNAL.is_file():
    journal = json.loads(JOURNAL.read_text())
    if journal.get("timerId") != TIMER_ID or journal.get("updateOperation") != UPDATE_OP:
        stop("EXISTING_JOURNAL_NOT_FOR_THIS_TEST_TIMER")
else:
    journal = {"timerId": TIMER_ID, "updateOperation": UPDATE_OP}
    persist(journal)


def submit_or_resume(stage, method, path, body, etag):
    record = journal.get(stage)
    if record is not None:
        if record.get("state") != "accepted" or not record.get("operationId"):
            stop(stage + "_POST_RESULT_UNKNOWN_DO_NOT_RETRY")
        operation_id = record["operationId"]
        print(stage, "REUSE_EXISTING_OPERATION=", operation_id, flush=True)
    else:
        # Persist the possibly-submitted state before network I/O.
        journal[stage] = {"state": "result_unknown"}
        persist(journal)
        key = "hw-resume-" + uuid.uuid4().hex
        try:
            code, _, result = request(method, path, body, etag, key)
        except (OSError, TimeoutError, socket.timeout) as e:
            print(stage, "POST_TRANSPORT_ERROR=", type(e).__name__, flush=True)
            stop(stage + "_POST_RESULT_UNKNOWN_DO_NOT_RETRY")
        if code != 202:
            print(stage, "POST_HTTP=", code, "RESPONSE=", result, flush=True)
            stop(stage + "_POST_NOT_ACCEPTED")
        operation_id = result.get("operationId", "")
        if not operation_id.startswith("op_"):
            stop(stage + "_RECEIPT_MISSING_DO_NOT_RETRY")
        journal[stage] = {"state": "accepted", "operationId": operation_id}
        persist(journal)
        print(stage, "HTTP=202 OPERATION=", operation_id, flush=True)
    completed(stage, operation_id)


print("RESUME=EXISTING_TEST_TIMER_ONLY", flush=True)
print("UPDATE_ALREADY_SUBMITTED=", UPDATE_OP, flush=True)
completed("UPDATE", UPDATE_OP)
etag = read_timer(UPDATED, True) if "toggle_off" not in journal else None

path = "/native-timers/" + TIMER_ID + "/modifications?backendId=default"
submit_or_resume("toggle_off", "POST", path,
                 {"kind": "toggle", "enabled": False}, etag)
if "toggle_on" not in journal:
    etag = read_timer(UPDATED, False)
print("TOGGLE_OFF=PASS", flush=True)

submit_or_resume("toggle_on", "POST", path,
                 {"kind": "toggle", "enabled": True}, etag)
if "delete" not in journal:
    etag = read_timer(UPDATED, True)
print("TOGGLE_ON=PASS", flush=True)

submit_or_resume("delete", "DELETE",
                 "/native-timers/" + TIMER_ID + "?backendId=default",
                 None, etag)
code, _, body = safe_get("/native-timers/" + TIMER_ID + "?backendId=default")
if code != 404:
    print("DELETE_READBACK_HTTP=", code, "DETAIL=", body, flush=True)
    stop("DELETE_NOT_ABSENT")
try:
    native = subprocess.run(["svdrpsend", "LSTT"], capture_output=True,
                            text=True, timeout=10)
except (OSError, subprocess.TimeoutExpired) as e:
    stop("VDR_LSTT_UNAVAILABLE_" + type(e).__name__)
if native.returncode != 0 or UPDATED in native.stdout or ORIGINAL + "-APIv1-HW" in native.stdout:
    stop("NATIVE_LSTT_DOES_NOT_PROVE_ABSENCE")
print("DELETE=PASS", flush=True)
print("HARDWARE_ACCEPTANCE=PASS UPDATE TOGGLE_OFF TOGGLE_ON DELETE", flush=True)
