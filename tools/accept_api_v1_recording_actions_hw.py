#!/usr/bin/env python3
"""Explicit, single-fixture real VDR API-v1 recording actions acceptance."""
import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request

MARKER = "VDR-SUITE-TEST"
RENAME = "VDR-SUITE-TEST-API-V1-RENAMED"
MOVE = "VDR-SUITE-TEST-API-V1-MOVED"

def request(base, path, method="GET", payload=None):
    headers = {"Accept": "application/json"}
    auth = os.environ.get("VDR_SUITE_TEST_AUTHORIZATION", "").strip()
    if not auth:
        raise RuntimeError("VDR_SUITE_TEST_AUTHORIZATION is required (full Authorization header value)")
    headers["Authorization"] = auth
    csrf = os.environ.get("VDR_SUITE_TEST_CSRF", "").strip()
    if csrf:
        headers["X-CSRF-Token"] = csrf
    data = None
    if payload is not None:
        headers["Content-Type"] = "application/json"
        data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(base + path, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            raw = resp.read()
            return resp.status, json.loads(raw) if raw else {}
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")[:1200]
        raise RuntimeError(f"{method} {path}: HTTP {exc.code}: {body}") from exc

def entries(obj):
    if isinstance(obj, dict):
        if isinstance(obj.get("backendNativeId"), str) and obj.get("backendNativeId"):
            yield obj
        for value in obj.values():
            yield from entries(value)
    elif isinstance(obj, list):
        for value in obj:
            yield from entries(value)

def snapshot(base):
    status, body = request(base, "/api/vdr/recordings")
    if status != 200:
        raise RuntimeError(f"recording list status {status}")
    return list(entries(body))

def identity(item):
    return str(item.get("backendNativeId") or ""), str(item.get("recordingId") or item.get("id") or "")

def find_exact(items, native):
    found = [x for x in items if identity(x)[0] == native]
    if len(found) != 1:
        raise RuntimeError(f"expected exactly one recording with native identity {native!r}, found {len(found)}")
    return found[0]

def confirm(text):
    print(text)
    if input("To proceed type exactly EXECUTE: ").strip() != "EXECUTE":
        raise RuntimeError("operator cancelled")

def run_action(base, backend, item, action, **params):
    native, rid = identity(item)
    if MARKER not in native or not native.endswith(".rec") or not rid:
        raise RuntimeError("STOP: missing marker/native .rec identity/recordingId")
    payload = {"backendId": backend, "recordingId": rid, "backendNativeId": native,
               "action": action, "dryRun": True, **params}
    status, validation = request(base, "/api/v1/recordings/actions/validate", "POST", payload)
    print(f"{action} VALIDATE HTTP={status} RESULT={json.dumps(validation, ensure_ascii=False)[:900]}")
    if status != 200 or validation.get("valid") is not True:
        raise RuntimeError(f"{action}: validation did not approve")
    status, dry = request(base, "/api/v1/recordings/actions/execute", "POST", payload)
    print(f"{action} DRY_RUN HTTP={status} RESULT={json.dumps(dry, ensure_ascii=False)[:900]}")
    if status != 200 or dry.get("errors"):
        raise RuntimeError(f"{action}: dry-run rejected")
    confirm(f"Fixture: {native}\nAction: {action} Params: {params}")
    payload["dryRun"] = False
    status, result = request(base, "/api/v1/recordings/actions/execute", "POST", payload)
    print(f"{action} EXECUTE HTTP={status} RESULT={json.dumps(result, ensure_ascii=False)[:1200]}")
    if status != 200 or result.get("success") is not True:
        raise RuntimeError(f"{action}: execute not confirmed; NEVER automatically retry")
    return native

def wait_for(base, predicate, description):
    for _ in range(30):
        items = snapshot(base)
        found = predicate(items)
        if found is not None:
            print(f"READBACK=PASS {description}")
            return found
        time.sleep(2)
    raise RuntimeError(f"READBACK=FAIL {description}; no automatic POST retry")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--base", default="http://127.0.0.1:18080")
    p.add_argument("--backend", default="default")
    p.add_argument("--native", required=True, help="Existing dedicated test recording's absolute backendNativeId")
    p.add_argument("--execute", action="store_true", help="Required for mutations; each action separately confirmed")
    args = p.parse_args()
    if not args.execute:
        raise RuntimeError("Real mutations disabled: explicitly specify --execute")
    native = args.native
    if MARKER not in native or not native.endswith(".rec"):
        raise RuntimeError("Fixture native path must contain VDR-SUITE-TEST and end in .rec")
    base = args.base.rstrip("/")
    item = find_exact(snapshot(base), native)
    print("FIXTURE=", json.dumps(item, ensure_ascii=False)[:1400])
    confirm("Confirm this is a disposable, completed dedicated TEST recording; NO real recordings")
    results = {"RENAME": "NOT_RUN", "MOVE": "NOT_RUN", "DELETE": "NOT_RUN"}
    try:
        old = run_action(base, args.backend, item, "RENAME", newName=RENAME)
        def renamed(items):
            matches = [x for x in items if MARKER in identity(x)[0] and RENAME.replace("-", "_") in identity(x)[0] and identity(x)[0] != old]
            return matches[0] if len(matches) == 1 else None
        item = wait_for(base, renamed, "RENAME")
        results["RENAME"] = "PASS"
        old = run_action(base, args.backend, item, "MOVE", targetPath=MOVE)
        def moved(items):
            matches = [x for x in items if MARKER in identity(x)[0] and MOVE.replace("-", "_") in identity(x)[0] and identity(x)[0] != old]
            return matches[0] if len(matches) == 1 else None
        item = wait_for(base, moved, "MOVE")
        results["MOVE"] = "PASS"
        old = run_action(base, args.backend, item, "DELETE")
        wait_for(base, lambda items: True if not [x for x in items if identity(x)[0] == old] else None, "DELETE")
        results["DELETE"] = "PASS"
    finally:
        for name, value in results.items():
            print(f"API_V1_RECORDING_{name}={value}")
        print("API_V1_RECORDING_HW_ACCEPTANCE=" +
              ("PASS" if all(x == "PASS" for x in results.values()) else "FAIL"))

if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, ValueError) as exc:
        print(f"STOP={exc}", file=sys.stderr)
        sys.exit(1)
