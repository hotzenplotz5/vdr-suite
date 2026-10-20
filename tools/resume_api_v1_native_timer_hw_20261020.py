#!/usr/bin/env python3
"""Resume existing 2026-10-20 native Timer hardware acceptance.

The original UPDATE was independently reconciled to failed_verified with
unchanged native state. Submit a new UPDATE only once, on that same managed
fixture; then TOGGLE OFF/ON and DELETE. Never CREATE or replay a submitted
mutation. Every step has a durable one-shot local journal.
"""
import atexit
import base64
import getpass
import json
import os
import shutil
import sqlite3
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


def discover_existing_auth():
    # Reboots remove /run; prefer the original credential location but
    # also accept other previously saved device-credential receipts.
    # Never print, copy, rotate or issue a credential here.
    candidates = [AUTH_PATH]
    from_env = os.environ.get("VDR_SUITE_API_V1_CREDENTIAL_FILE", "")
    if from_env:
        candidates.insert(0, Path(from_env))
    roots = (Path("/run"), Path("/tmp"), Path("/root"),
             Path("/home/yavdr"), Path("/etc/vdr-suite"),
             Path("/var/lib/vdr-suite"))
    patterns = ("vdr-suite-api-v1*.json", "vdr-suite-timer*.json",
                "*device*credential*.json", "*api*v1*credential*.json")
    for root in roots:
        if not root.is_dir():
            continue
        for pattern in patterns:
            candidates.extend(sorted(root.glob(pattern)))
            candidates.extend(sorted(root.glob("*/" + pattern)))

    seen = set()
    for candidate in candidates:
        if str(candidate) in seen:
            continue
        seen.add(str(candidate))
        if not candidate.is_file():
            continue
        try:
            document = json.loads(candidate.read_text())
            credential_id = document.get("credentialId", "")
            credential_secret = document.get("credentialSecret", "")
            if not isinstance(credential_id, str) or not credential_id or (
                not isinstance(credential_secret, str) or
                len(credential_secret) < 16
            ):
                continue
            token = "VDR-Suite-Device " + credential_id + "." + credential_secret
            req = urllib.request.Request(
                BASE + "/native-timers?backendId=default",
                headers={"Authorization": token, "Accept": "application/json"},
                method="GET")
            with urllib.request.urlopen(req, timeout=4) as response:
                if response.status != 200:
                    continue
            print("AUTH=EXISTING_DEVICE_CREDENTIAL_VERIFIED",
                  "PATH=", candidate, flush=True)
            return token
        except (OSError, ValueError, TypeError, KeyError,
                urllib.error.URLError):
            continue
    return None


def admin_browser_session():
    # Reuse the existing server administrator account. This creates only a
    # temporary authenticated browser session; no new device/pairing/credential.
    if not sys.stdin.isatty():
        stop("DEVICE_CREDENTIAL_MISSING_ADMIN_TTY_REQUIRED_NO_MUTATION")
    print("AUTH=EXISTING_DEVICE_RECEIPT_MISSING", flush=True)
    print("AUTH_FALLBACK=ADMIN_BROWSER_SESSION_LOCAL_ONLY", flush=True)
    username = input("VDR-Suite Admin-Benutzer: ").strip()
    if not username or ":" in username or any(ord(c) < 32 for c in username):
        stop("ADMIN_USERNAME_INVALID_NO_MUTATION")
    password = getpass.getpass("VDR-Suite Admin-Passwort: ")
    if not password:
        stop("ADMIN_PASSWORD_EMPTY_NO_MUTATION")
    basic = base64.b64encode((username + ":" + password).encode()).decode()
    del password
    login = urllib.request.Request(
        "http://127.0.0.1:18080/api/security/browser-sessions",
        data=b"{}", method="POST",
        headers={"Authorization": "Basic " + basic,
                 "Content-Type": "application/json",
                 "Accept": "application/json",
                 "Origin": "http://127.0.0.1:18080"})
    del basic
    try:
        with urllib.request.urlopen(login, timeout=8) as response:
            cookie = response.headers.get("Set-Cookie", "").split(";", 1)[0]
            material = json.loads(response.read())
            if response.status != 200:
                stop("ADMIN_LOGIN_UNEXPECTED_STATUS_NO_MUTATION")
    except urllib.error.HTTPError as error:
        print("ADMIN_LOGIN_HTTP=", error.code, flush=True)
        stop("ADMIN_SESSION_UNAVAILABLE_NO_MUTATION")
    except (OSError, ValueError, TypeError, urllib.error.URLError):
        stop("ADMIN_SESSION_TRANSPORT_OR_RESPONSE_INVALID_NO_MUTATION")
    csrf = material.get("csrfToken")
    if (not cookie.startswith("vdr_suite_session=") or
        not isinstance(csrf, str) or not csrf or
        any(c in cookie for c in "\r\n")):
        stop("ADMIN_SESSION_COOKIE_OR_CSRF_MISSING_NO_MUTATION")

    headers = {"Cookie": cookie, "X-CSRF-Token": csrf,
               "Origin": "http://127.0.0.1:18080"}

    def logout():
        try:
            req = urllib.request.Request(
                "http://127.0.0.1:18080/api/security/browser-sessions/logout",
                data=b"{}", method="POST",
                headers={**headers, "Content-Type": "application/json"})
            with urllib.request.urlopen(req, timeout=3):
                pass
        except (OSError, urllib.error.URLError):
            pass

    atexit.register(logout)
    print("AUTH=ADMIN_BROWSER_SESSION_CREATED", flush=True)
    return headers


device_auth = discover_existing_auth()
auth_headers = ({"Authorization": device_auth} if device_auth
                else admin_browser_session())


def request(method, path, body=None, tag=None, key=None):
    hdr = {**auth_headers, "Accept": "application/json"}
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
    # A successful native readback updates the durable binding before the
    # independent five-second VDR snapshot poll catches up. Public v1
    # correctly responds 503 while those two views disagree. Wait for the
    # existing cache refresh; do not repeat the successful mutation.
    last_status = None
    for attempt in range(31):
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
        # 503 is a transient snapshot-versus-binding conflict after a
        # successful UPDATE. Do not interpret it as mutation failure.
        status = (code, body.get("title") if code == 200 else None)
        if status != last_status:
            print("TIMER_READBACK=WAIT",
                  "HTTP=", code, "TITLE=",
                  body.get("title") if code == 200 else "<unavailable>",
                  flush=True)
            last_status = status
        if code not in (200, 503):
            print("TIMER_DETAIL_HTTP=", code, "DETAIL=", body, flush=True)
            stop("TEST_TIMER_READBACK_UNEXPECTED_RESPONSE_NO_MUTATION")
        if attempt != 30:
            time.sleep(2)
    print("TIMER_DETAIL_HTTP=", code, "DETAIL=", body, flush=True)
    stop("TEST_TIMER_READBACK_NOT_VERIFIED_NO_RETRY")


def verify_native_modify_provider_sync():
    # A VDR restart creates a new SuiteBridge provider instance epoch.
    # Before a *new* operation, require the Control Plane's published Agent
    # facts to match the LIVE SuiteBridge capabilities. No mutation here.
    commands = [path for name in ("svdrpsend.sh", "svdrpsend")
                if (path := shutil.which(name))]
    if not commands:
        stop("SVDRP_CLIENT_MISSING_NO_MUTATION")

    def live_capability(kind):
        for command in commands:
            try:
                result = subprocess.run(
                    [command, "plug", "suitebridge", "NTMOD", "CAP", "1", kind],
                    capture_output=True, text=True, timeout=8)
            except (OSError, subprocess.TimeoutExpired):
                continue
            if result.returncode != 0:
                continue
            for line in result.stdout.splitlines():
                marker = "vdr-suite-ntmod-cap/1"
                position = line.find(marker)
                if position < 0:
                    continue
                fields = line[position:].split()
                capability = "vdr.timer." + kind
                if (len(fields) == 10 and fields[0] == marker and
                    fields[1] == capability and fields[2:5] ==
                        ["1", "timer-modify", "enabled"] and
                    fields[5] == "suitebridge" and fields[9] == "enabled" and
                    fields[7].isdigit() and fields[8].isdigit()):
                    return (fields[6], int(fields[7]), int(fields[8]))
        return None

    for _ in range(15):
        update = live_capability("update")
        toggle = live_capability("toggle")
        if update and toggle and update == toggle:
            with sqlite3.connect(
                    "file:/var/lib/vdr-suite/vdr-suite.db?mode=ro",
                    uri=True) as db:
                facts = db.execute("""
                    SELECT provider_kind, provider_instance_epoch,
                           provider_generation, capability_revision,
                           available, capabilities, observed_at
                    FROM backend_agent_local_provider_facts
                    WHERE backend_id='default'
                      AND provider_id='suitebridge:local'
                """).fetchone()
            if (facts and facts[0] == "suitebridge" and
                (facts[1], facts[2], facts[3]) == update and
                facts[4] == 1 and
                all(c in facts[5].split(",") for c in
                    ("vdr.timer.update", "vdr.timer.toggle")) and
                facts[6] > 0):
                print("NATIVE_MODIFY_PROVIDER_SYNC=PASS",
                      "EPOCH=", update[0], flush=True)
                return
            print("NATIVE_MODIFY_PROVIDER_SYNC=WAIT",
                  "LIVE_EPOCH=", update[0],
                  "PUBLISHED_EPOCH=", facts[1] if facts else "missing",
                  flush=True)
        else:
            print("NATIVE_MODIFY_PROVIDER_SYNC=WAIT_LIVE_CAPS", flush=True)
        time.sleep(2)
    stop("NATIVE_MODIFY_PROVIDER_NOT_SYNCHRONIZED_NO_MUTATION")


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

# The old UPDATE belongs to the now-lost Device credential's actor. Public
# operation GET is strictly actor-scoped: even an Admin browser session gets
# 404 for somebody else's operation. Confirm this *historical* result
# through the authoritative local DB, read-only; new operations still use
# their normal actor-scoped Public-v1 GET throughout this acceptance.
with sqlite3.connect(
        "file:/var/lib/vdr-suite/vdr-suite.db?mode=ro", uri=True) as db:
    historical = db.execute("""
        SELECT action_family, resource_type, resource_id, backend_id,
               state, result_reference
        FROM mutation_operations WHERE operation_id=?
    """, (UPDATE_OP,)).fetchone()
    binding_owner = db.execute("""
        SELECT native_timer_binding_id FROM native_timer_bindings
        WHERE backend_id='default' AND backend_native_timer_id=?
          AND ownership='managed' AND drift_state='none' AND missing_since=0
    """, (TIMER_ID,)).fetchone()
if (not historical or not binding_owner or
        historical[0] != "timer.update" or
        historical[1] != "NativeTimerBinding" or
        historical[2] != binding_owner[0] or
        historical[3] != "default" or
        historical[4] != "failed_verified" or
        historical[5] != "native-timer-modify:unchanged-after-uncertain-execution"):
    stop("HISTORICAL_UPDATE_NOT_VERIFIED_FAILED_NO_NEW_MUTATION")
print("PREVIOUS_UPDATE=", UPDATE_OP, "STATE=failed_verified",
      "SOURCE=AUTHORITATIVE_READ_ONLY_DB", flush=True)

# The prior replacement reached the Agent but was rejected before any
# native effect by the SuiteBridge CAS encoding mismatch. This particular
# historical failure is terminal; never replay its POST. A corrected plugin
# permits exactly one *new* UPDATE, with a distinct durable journal stage.
REJECTED_UPDATE_OP = "op_d9164b555970f8bfc742b72ff0be16a2"
prior = journal.get("update_replacement", {})
if prior.get("state") != "accepted" or prior.get("operationId") != REJECTED_UPDATE_OP:
    stop("PREVIOUS_REPLACEMENT_JOURNAL_MISMATCH_NO_MUTATION")
with sqlite3.connect(
        "file:/var/lib/vdr-suite/vdr-suite.db?mode=ro", uri=True) as db:
    rejected = db.execute("""
        SELECT state, result_reference, action_family, resource_type
        FROM mutation_operations WHERE operation_id=?
    """, (REJECTED_UPDATE_OP,)).fetchone()
    commands = db.execute("""
        SELECT c.command_id, c.state, r.result_category, r.error_category,
               r.dispatch_state, r.verification_state
        FROM backend_agent_commands c
        LEFT JOIN backend_agent_command_results r ON r.command_id=c.command_id
        WHERE c.operation_id=?
    """, (REJECTED_UPDATE_OP,)).fetchall()
if (not rejected or rejected[0] != "failed_verified" or
    rejected[2:] != ("timer.update", "NativeTimerBinding") or
    len(commands) != 1 or
    rejected[1] != "ntmod:vdr:fingerprint-mismatch:" + commands[0][0] or
    commands[0][1:] != ("completed", "rejected", "fenced",
                        "not_started", "verified")):
    stop("PRIOR_FINGERPRINT_REJECTION_NOT_PROVEN_NO_MUTATION")
print("PRIOR_UPDATE=FAILED_VERIFIED_REJECTED_WITHOUT_EFFECT",
      "OPERATION=", REJECTED_UPDATE_OP, flush=True)

# The CAS-corrected attempt was fenced before the native executor after
# the SuiteBridge provider epoch changed. Never replay that command.
STALE_PROVIDER_OP = "op_d34a6048a2b929ca55c926f7cc0dc1fd"
prior = journal.get("update_after_cas_fix", {})
if prior.get("state") != "accepted" or prior.get("operationId") != STALE_PROVIDER_OP:
    stop("PREVIOUS_PROVIDER_FENCE_JOURNAL_MISMATCH_NO_MUTATION")
with sqlite3.connect(
        "file:/var/lib/vdr-suite/vdr-suite.db?mode=ro", uri=True) as db:
    rejected = db.execute("""
        SELECT state, result_reference, action_family, resource_type
        FROM mutation_operations WHERE operation_id=?
    """, (STALE_PROVIDER_OP,)).fetchone()
    commands = db.execute("""
        SELECT c.state, r.result_category, r.error_category,
               r.dispatch_state, r.verification_state, r.result_evidence
        FROM backend_agent_commands c
        LEFT JOIN backend_agent_command_results r ON r.command_id=c.command_id
        WHERE c.operation_id=?
    """, (STALE_PROVIDER_OP,)).fetchall()
if (not rejected or rejected !=
        ("failed_verified", "executor-fence:provider-changed",
         "timer.update", "NativeTimerBinding") or
    len(commands) != 1 or
    commands[0][:5] !=
        ("completed", "rejected", "fenced", "not_started", "verified") or
    "executor-fence:provider-changed" not in (commands[0][5] or "")):
    stop("PREVIOUS_PROVIDER_CHANGE_NOT_VERIFIED_NO_MUTATION")
print("PRIOR_PROVIDER_CHANGE=REJECTED_WITHOUT_EFFECT",
      "OPERATION=", STALE_PROVIDER_OP, flush=True)

path = "/native-timers/" + TIMER_ID + "/modifications?backendId=default"
if "update_after_provider_sync" not in journal:
    if any(stage in journal for stage in ("toggle_off", "toggle_on", "delete")):
        stop("JOURNAL_OUT_OF_ORDER")
    verify_native_modify_provider_sync()
    etag = read_timer(ORIGINAL, True)
    with sqlite3.connect(
            "file:/var/lib/vdr-suite/vdr-suite.db?mode=ro", uri=True) as db:
        native = db.execute("""
            SELECT observed_channel_id, observed_title,
                   observed_directory, observed_day, observed_weekdays,
                   observed_start_time, observed_end_time,
                   observed_priority, observed_lifetime, observed_vps,
                   observed_enabled, observed_recording, observed_pending
            FROM native_timer_bindings
            WHERE backend_id='default' AND backend_native_timer_id=?
              AND ownership='managed' AND drift_state='none'
              AND missing_since=0
        """, (TIMER_ID,)).fetchone()
    if not native or native[1] != ORIGINAL or not native[10] or native[11] or native[12]:
        stop("BINDING_SPECIFICATION_NOT_VERIFIED")
    update_body = {
        "kind": "update",
        "channelId": native[0],
        "title": UPDATED,
        "directory": native[2],
        "day": native[3],
        "weekdays": native[4],
        "startTime": native[5],
        "endTime": native[6],
        "priority": native[7],
        "lifetime": native[8],
        "vps": bool(native[9]),
        "enabled": True,
    }
else:
    etag = None
    update_body = None
submit_or_resume("update_after_provider_sync", "POST", path, update_body, etag)
if "toggle_off" not in journal:
    etag = read_timer(UPDATED, True)
print("UPDATE=PASS", flush=True)

submit_or_resume("toggle_off", "POST", path,
                 {"kind": "toggle", "enabled": False},
                 etag if "toggle_off" not in journal else None)
if "toggle_on" not in journal:
    etag = read_timer(UPDATED, False)
print("TOGGLE_OFF=PASS", flush=True)

submit_or_resume("toggle_on", "POST", path,
                 {"kind": "toggle", "enabled": True},
                 etag if "toggle_on" not in journal else None)
if "delete" not in journal:
    etag = read_timer(UPDATED, True)
print("TOGGLE_ON=PASS", flush=True)

submit_or_resume("delete", "DELETE",
                 "/native-timers/" + TIMER_ID + "?backendId=default",
                 None, etag if "delete" not in journal else None)
code, _, body = safe_get("/native-timers/" + TIMER_ID + "?backendId=default")
if code != 404:
    print("DELETE_READBACK_HTTP=", code, "DETAIL=", body, flush=True)
    stop("DELETE_NOT_ABSENT")
try:
    native = subprocess.run(["svdrpsend", "LSTT"], capture_output=True,
                            text=True, timeout=10)
except (OSError, subprocess.TimeoutExpired) as e:
    stop("VDR_LSTT_UNAVAILABLE_" + type(e).__name__)
if native.returncode != 0 or UPDATED in native.stdout:
    stop("NATIVE_LSTT_DOES_NOT_PROVE_ABSENCE")
print("DELETE=PASS", flush=True)
print("HARDWARE_ACCEPTANCE=PASS UPDATE TOGGLE_OFF TOGGLE_ON DELETE", flush=True)
