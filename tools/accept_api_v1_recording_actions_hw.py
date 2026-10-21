#!/usr/bin/env python3
"""Explicit, single-fixture real VDR public-v1 Recording Actions acceptance.

Human Account browser-session authentication for the local Suite HTTP endpoint.
Recording identity and postconditions are read from local RESTfulAPI; all
mutations go only through the public-v1 Suite endpoints. No POST retries.
"""

import argparse
import base64
import getpass
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

STAMP = "2026-10-20.21.45.1-0.rec"
APPROVED_NATIVE = "/srv/vdr/video/Report_Mainz/" + STAMP
RENAME = "VDR-SUITE-TEST-API-V1-RENAMED"
MOVE = "VDR-SUITE-TEST-API-V1-MOVED"
RENAMED_NATIVE = "/srv/vdr/video/" + RENAME + "/" + STAMP
MOVED_NATIVE = "/srv/vdr/video/" + MOVE + "/" + RENAME + "/" + STAMP


def http_json(base, path, method="GET", payload=None, headers=None):
    data = json.dumps(payload).encode("utf-8") if payload is not None else None
    req_headers = {"Accept": "application/json", **(headers or {})}
    if data is not None:
        req_headers["Content-Type"] = "application/json"
    req = urllib.request.Request(
        base + path, data=data, headers=req_headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            raw = resp.read()
            return resp.status, json.loads(raw) if raw else {}, resp.headers
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")[:700]
        raise RuntimeError(f"{method} {path}: HTTP {exc.code}: {body}") from exc


def ensure_local(base):
    url = urllib.parse.urlsplit(base)
    if url.scheme != "http" or url.hostname not in ("127.0.0.1", "localhost", "::1"):
        raise RuntimeError("This hardware acceptance only supports local loopback HTTP")
    if url.username or url.password or url.query or url.fragment or url.path not in ("", "/"):
        raise RuntimeError("Base URL must be a plain local origin, without credentials/path/query")
    return base.rstrip("/")


class AcceptanceClient:
    def __init__(self, suite_base, vdr_base, username, password):
        self.suite_base = ensure_local(suite_base)
        self.vdr_base = ensure_local(vdr_base)
        auth = base64.b64encode((username + ":" + password).encode("utf-8")).decode("ascii")
        status, body, response_headers = http_json(
            self.suite_base, "/api/security/browser-sessions", "POST", {},
            {"Authorization": "Basic " + auth})
        cookie = response_headers.get("Set-Cookie", "").split(";", 1)[0]
        csrf = body.get("csrfToken")
        if status != 200 or not cookie.startswith("vdr_suite_session=") or not csrf:
            raise RuntimeError("Human session issuance failed")
        # The cookie has Secure; this explicit header is limited to loopback HTTP.
        # Never disable normal cookie security for any browser or remote origin.
        self.headers = {"Cookie": cookie, "X-CSRF-Token": csrf}
        print("HUMAN_AUTH=PASS")

    def action_request(self, endpoint, payload):
        status, body, _ = http_json(self.suite_base, endpoint, "POST", payload, self.headers)
        return status, body

    def snapshot(self):
        status, body, _ = http_json(self.vdr_base, "/recordings.json")
        records = body.get("recordings") if isinstance(body, dict) else None
        if status != 200 or not isinstance(records, list):
            raise RuntimeError("Local RESTfulAPI recording collection unavailable")
        return records

    def find_exact(self, native, required=True):
        hits = [item for item in self.snapshot() if item.get("file_name") == native]
        if len(hits) > 1 or (required and len(hits) != 1):
            raise RuntimeError(f"Unsafe recording identity match: {native!r}, count={len(hits)}")
        return hits[0] if hits else None

    def readback(self, expected, old=None, deleted=False):
        for _ in range(30):
            records = self.snapshot()
            hits = [item for item in records if item.get("file_name") == expected]
            old_hits = ([item for item in records if item.get("file_name") == old]
                        if old else [])
            if len(hits) > 1 or len(old_hits) > 1:
                raise RuntimeError("Ambiguous recording readback; no further mutations")
            if (not hits if deleted else len(hits) == 1) and not old_hits:
                print("READBACK=PASS", "ABSENT" if deleted else expected)
                return None if deleted else hits[0]
            time.sleep(2)
        raise RuntimeError("READBACK=FAIL; never automatically retry mutation")


def confirm(text):
    print(text, flush=True)
    if input("To proceed type exactly EXECUTE: ").strip() != "EXECUTE":
        raise RuntimeError("Operator cancelled")


def run_action(client, source, action, expected_source, expected_target=None, **params):
    if source.get("file_name") != expected_source:
        raise RuntimeError("Recording native identity changed before mutation")
    rid = str(source.get("number") or "")
    if not rid.isdecimal():
        raise RuntimeError("Recording number unavailable")
    # Never trust a cached numerical index; re-discover it by exact native path.
    current = client.find_exact(expected_source)
    if str(current.get("number")) != rid:
        raise RuntimeError("Recording ID changed before mutation; no mutation attempted")
    if expected_target and client.find_exact(expected_target, required=False):
        raise RuntimeError("Target already exists; no mutation attempted")
    payload = {
        "backendId": "default", "recordingId": rid,
        "backendNativeId": expected_source, "action": action,
        "dryRun": True, **params,
    }
    status, validation = client.action_request("/api/v1/recordings/actions/validate", payload)
    if status != 200 or validation.get("valid") is not True:
        raise RuntimeError(f"{action}: public-v1 validation rejected: {validation}")
    print(f"{action} VALIDATE=PASS")

    status, dry = client.action_request("/api/v1/recordings/actions/execute", payload)
    try:
        upstream_preview = json.loads(dry.get("upstreamResponseBody") or "{}")
    except ValueError as exc:
        raise RuntimeError(f"{action}: invalid backend preview JSON") from exc
    if (status != 200 or dry.get("errors") or dry.get("upstreamHttpStatus") != 200
            or upstream_preview.get("executable") is not True):
        raise RuntimeError(f"{action}: non-executable backend preview: {dry}")
    # Suite intentionally normalizes a successful preview to success:false.
    print(f"{action} PREVIEW=EXECUTABLE")

    confirm(f"Fixture: {expected_source}\nAction: {action}\nParameters: {params}")
    payload["dryRun"] = False
    status, executed = client.action_request("/api/v1/recordings/actions/execute", payload)
    if status != 200 or executed.get("success") is not True or executed.get("errors"):
        raise RuntimeError(f"{action}: execution unconfirmed; NEVER retry automatically: {executed}")
    upstream = json.loads(executed.get("upstreamResponseBody") or "{}")
    allowed = {"RENAME": {"renamed", "already-renamed"},
               "MOVE": {"moved", "already-moved"},
               "DELETE": {"trashed", "already-trashed"}}
    if upstream.get("status") not in allowed[action]:
        raise RuntimeError(f"{action}: unexpected upstream status; no retry: {upstream}")
    if expected_target and upstream.get("target_file") != expected_target:
        raise RuntimeError(f"{action}: unexpected target path; no further mutation: {upstream}")
    print(f"{action} EXECUTE=PASS STATUS={upstream.get('status')}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", default="http://127.0.0.1:18080")
    parser.add_argument("--vdr-base", default="http://127.0.0.1:8002")
    parser.add_argument("--user", default="vdr-suite")
    parser.add_argument("--native", required=True)
    parser.add_argument("--execute", action="store_true")
    args = parser.parse_args()
    if not args.execute:
        raise RuntimeError("Real mutations disabled: explicitly specify --execute")
    if args.native != APPROVED_NATIVE:
        raise RuntimeError("Only the explicitly approved Report_Mainz fixture is allowed")
    # Refuse to reuse the disposable recording's new identity for another run.
    ensure_local(args.base)
    ensure_local(args.vdr_base)
    password = getpass.getpass(f"VDR-Suite password for {args.user}: ")
    client = AcceptanceClient(args.base, args.vdr_base, args.user, password)
    password = None
    results = {"RENAME": "NOT_RUN", "MOVE": "NOT_RUN", "DELETE": "NOT_RUN"}
    try:
        original = client.find_exact(APPROVED_NATIVE)
        if client.find_exact(RENAMED_NATIVE, required=False) or client.find_exact(MOVED_NATIVE, required=False):
            raise RuntimeError("Fixture collision; no mutation attempted")
        confirm("Confirm that this exact, completed Report_Mainz recording may be renamed, moved and trashed")
        run_action(client, original, "RENAME", APPROVED_NATIVE, RENAMED_NATIVE,
                   newName=RENAME)
        renamed = client.readback(RENAMED_NATIVE, old=APPROVED_NATIVE)
        results["RENAME"] = "PASS"
        run_action(client, renamed, "MOVE", RENAMED_NATIVE, MOVED_NATIVE,
                   targetPath=MOVE)
        moved = client.readback(MOVED_NATIVE, old=RENAMED_NATIVE)
        results["MOVE"] = "PASS"
        run_action(client, moved, "DELETE", MOVED_NATIVE)
        client.readback(MOVED_NATIVE, deleted=True)
        results["DELETE"] = "PASS"
    finally:
        for step, state in results.items():
            print(f"API_V1_RECORDING_{step}={state}")
        print("API_V1_RECORDING_HW_ACCEPTANCE=" +
              ("PASS" if all(s == "PASS" for s in results.values()) else "FAIL"))


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, ValueError, EOFError) as exc:
        print(f"STOP={exc}", file=sys.stderr)
        sys.exit(1)