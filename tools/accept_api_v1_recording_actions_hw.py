#!/usr/bin/env python3
"""Public-v1 R2/R3 hardware acceptance; only the operator-approved test fixture.

Default mode is read-only inspection + Validate/Dry-run. --execute explicitly
allows one RENAME -> MOVE -> DELETE sequence, with a separate console approval
for each write. No mutation POST is ever retried; unknown outcomes STOP.

Uses the Suite's public rec_ and fld1_ IDs. Never send backend-native IDs/paths
in public request bodies. Local RESTfulAPI is *read-only* corroborating evidence.
"""
from __future__ import annotations

import argparse
import base64
import datetime
import getpass
import json
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

FIXTURE = "VDR-SUITE-TEST-R3"
RENAMED = "VDR-SUITE-TEST-R3-RENAMED"
PUBLIC_ID = re.compile(r"rec_[0-9a-f]{32}\Z")
FOLDER_ID = re.compile(r"fld1_[0-9a-f]{32}\Z")
REC_LEAF = re.compile(r"\d{4}-\d\d-\d\d\.\d\d\.\d\d\.[0-9-]+\.rec\Z")
VALID_BACKEND = re.compile(r"[A-Za-z0-9._-]{1,128}\Z")


class Stop(RuntimeError):
    pass


def local_origin(value: str) -> str:
    parsed = urllib.parse.urlsplit(value)
    if (parsed.scheme != "http" or parsed.hostname not in
            ("127.0.0.1", "localhost", "::1") or
            parsed.username or parsed.password or parsed.query or
            parsed.fragment or parsed.path not in ("", "/")):
        raise Stop("HTTP endpoints must be plain local loopback origins")
    return value.rstrip("/")


def request(origin, path, *, method="GET", body=None, headers=None):
    if not path.startswith("/") or path.startswith("//"):
        raise Stop("Invalid request path")
    encoded = json.dumps(body, separators=(",", ":")).encode() if body is not None else None
    custom = {"Accept": "application/json", **(headers or {})}
    if encoded is not None:
        custom["Content-Type"] = "application/json"
    req = urllib.request.Request(origin + path, data=encoded,
                                 headers=custom, method=method)
    try:
        with urllib.request.urlopen(req, timeout=20) as response:
            status, raw, result_headers = response.status, response.read(2_000_000), response.headers
    except urllib.error.HTTPError as exc:
        status, raw, result_headers = exc.code, exc.read(2_000_000), exc.headers
    except (OSError, TimeoutError) as exc:
        raise Stop("HTTP outcome unknown; no automatic POST retry: " +
                   exc.__class__.__name__) from exc
    try:
        data = json.loads(raw) if raw else {}
    except (ValueError, UnicodeDecodeError) as exc:
        raise Stop("Non-JSON server response; STOP without retry") from exc
    if not isinstance(data, dict):
        raise Stop("Unexpected non-object API response")
    return status, data, result_headers


def expect(condition, message):
    if not condition:
        raise Stop(message)


def check_public_id(value):
    return isinstance(value, str) and PUBLIC_ID.fullmatch(value) is not None


def choose_fixture(native_rows):
    """Never accept an arbitrary older Recording, substring or list index."""
    matches = [r for r in native_rows
               if isinstance(r, dict) and r.get("name") == FIXTURE]
    expect(len(matches) == 1,
           "Required exactly one live RESTfulAPI recording titled " + FIXTURE +
           " (got " + str(len(matches)) + ")")
    item = matches[0]
    path = item.get("file_name")
    expect(isinstance(path, str) and path.startswith("/") and
           ("/" + FIXTURE + "/") in path and
           REC_LEAF.fullmatch(path.rsplit("/", 1)[-1]) is not None,
           "Test recording must have a normal .rec path under its exact fixture title")
    # The operator approved a NEW test fixture, not an earlier recording
    # with a coincidentally matching title.
    stamp = path.rsplit("/", 1)[-1][:16]
    try:
        started = datetime.datetime.strptime(stamp, "%Y-%m-%d.%H.%M")
    except ValueError as exc:
        raise Stop("Could not verify fixture start timestamp") from exc
    now = datetime.datetime.now()
    age = (now - started).total_seconds()
    expect(0 <= age <= 12 * 3600,
           "Fixture must have been recorded in the preceding 12 hours")
    expect(int(item.get("duration") or 0) > 0 and
           int(item.get("filesize_mb") or 0) > 0,
           "Test recording must be finished and have known nonzero duration/size")
    return item


def correlate(public_rows, native, backend):
    """Select only the unique public rec_ matching the exact test source."""
    rows = [r for r in public_rows if isinstance(r, dict) and
            r.get("title") == FIXTURE and r.get("backendId") == backend and
            check_public_id(r.get("recordingId"))]
    start = native.get("event_start_time")
    if isinstance(start, int) and start > 0:
        rows = [r for r in rows if str(r.get("recordedAt")) == str(start)]
    size = native.get("filesize_mb")
    if isinstance(size, int) and size > 0:
        rows = [r for r in rows if r.get("sizeMb") == size]
    expect(len(rows) == 1,
           "Could not match exactly one public rec_ to the approved test recording")
    return rows[0]


def require_readback(before, after_rows, action):
    """Fail closed; compare the leaf and source duration/size before a rebind."""
    original = before["file_name"]
    leaf = original.rsplit("/", 1)[-1]
    expect(not any(r.get("file_name") == original for r in after_rows),
           "Original native path still present after " + action)
    candidates = [r for r in after_rows if
                  r.get("file_name", "").rsplit("/", 1)[-1] == leaf and
                  r.get("file_name") != original and
                  r.get("event_start_time") == before.get("event_start_time") and
                  r.get("duration") == before.get("duration") and
                  r.get("filesize_mb") == before.get("filesize_mb")]
    expect(len(candidates) == 1,
           "Provider readback has no unique matching relocated source")
    return candidates[0]


def prompt_for_action(action, public_id, native_path, folder_name=""):
    print("FIXTURE=", FIXTURE, "PUBLIC_ID=", public_id)
    print("ACTION=", action, "LOCAL_CURRENT_SOURCE=", native_path)
    if folder_name:
        print("DESTINATION_FOLDER=", folder_name)
    expected = action + " " + public_id
    entered = input("One-time write approval, type '" + expected + "': ").strip()
    expect(entered == expected, "Operator canceled; no mutation sent")


class Client:
    def __init__(self, suite, vdr, username, password):
        self.suite, self.vdr = local_origin(suite), local_origin(vdr)
        authorization = base64.b64encode(
            (username + ":" + password).encode()).decode()
        status, body, headers = request(
            self.suite, "/api/security/browser-sessions",
            method="POST", body={},
            headers={"Authorization": "Basic " + authorization})
        cookie = headers.get("Set-Cookie", "").split(";", 1)[0]
        token = body.get("csrfToken")
        expect(status == 200 and cookie.startswith("vdr_suite_session=") and
               isinstance(token, str) and bool(token),
               "Browser login/CSRF failed")
        # Explicit cookie header only on local loopback. Never bypass HTTPS
        # Secure cookies for a remote origin.
        self.headers = {"Cookie": cookie, "X-CSRF-Token": token}
        print("HUMAN_AUTH=PASS")

    def native(self):
        status, body, _ = request(self.vdr, "/recordings.json")
        records = body.get("recordings")
        expect(status == 200 and isinstance(records, list),
               "Local VDR Recording inventory unavailable")
        expect(all(isinstance(r, dict) for r in records),
               "Malformed VDR inventory")
        return records

    def public(self, backend):
        seen, items, cursor = set(), [], ""
        for _ in range(100):
            qs = {"backendId": backend, "limit": "100"}
            if cursor:
                qs["cursor"] = cursor
            path = "/api/v1/recordings?" + urllib.parse.urlencode(qs)
            status, body, _ = request(self.suite, path, headers=self.headers)
            expect(status == 200 and isinstance(body.get("items"), list) and
                   isinstance(body.get("page"), dict),
                   "Public v1 Recording list unavailable or unauthorized")
            items.extend(body["items"])
            expect(len(items) <= 10000, "Recording collection safety bound exceeded")
            next_cursor = body["page"].get("nextCursor")
            if not next_cursor:
                return items
            expect(isinstance(next_cursor, str) and next_cursor not in seen,
                   "Public Recording cursor repeated")
            seen.add(next_cursor)
            cursor = next_cursor
        raise Stop("Recording list pagination safety bound exceeded")

    def folders_at_root(self, backend):
        results = []
        for offset in range(0, 10000, 100):
            path = "/api/v1/recordings?" + urllib.parse.urlencode(
                {"backendId": backend, "view": "folders",
                 "offset": offset, "limit": 100})
            status, body, _ = request(self.suite, path, headers=self.headers)
            expect(status == 200 and isinstance(body.get("items"), list),
                   "Public folder browse unavailable")
            results.extend(r for r in body["items"]
                           if isinstance(r, dict) and r.get("kind") == "folder")
            total = body.get("page", {}).get("totalCount")
            if isinstance(total, int) and offset + 100 >= total:
                return results
            if not body["items"]:
                return results
        raise Stop("Root folder collection exceeds safety bound")

    def folder(self, backend, folder_id):
        expect(FOLDER_ID.fullmatch(folder_id) is not None,
               "MOVE requires one real fld1_ ID")
        seen = [r for r in self.folders_at_root(backend)
                if r.get("folderId") == folder_id]
        expect(len(seen) == 1, "Target folder ID must appear exactly once in root browse")
        folder_name = seen[0].get("name")
        expect(isinstance(folder_name, str) and bool(folder_name),
               "Target folder lacks an unambiguous name")
        expect(FIXTURE not in folder_name or folder_name.endswith("-MOVED"),
               "Ambiguous target folder identity")
        return folder_name

    def action(self, endpoint, payload, *, etag=None):
        custom = dict(self.headers)
        if etag is not None:
            custom["If-Match"] = etag
        return request(self.suite,
                       "/api/v1/recordings/actions/" + endpoint,
                       method="POST", body=payload, headers=custom)

    def preflight(self, backend, public_id, action, *,
                  new_name=None, folder_id=None):
        body = {"backendId": backend, "recordingId": public_id,
                "action": action, "dryRun": True}
        if action == "RENAME":
            body["newName"] = new_name
        if action == "MOVE":
            body["targetFolderId"] = folder_id
        status, valid, headers = self.action("validate", body)
        expect(status == 200 and valid.get("valid") is True and
               valid.get("recordingId") == public_id,
               action + ": validation failed, no write attempted")
        tag = headers.get("ETag")
        expect(isinstance(tag, str) and re.fullmatch(r'"vsr-[0-9a-f]+"', tag),
               action + ": authoritative SafeMutation ETag missing")
        status, dry, _ = self.action("execute", body)
        expect(status == 200 and dry.get("dryRun") is True and
               dry.get("executed") is False and
               dry.get("validation", {}).get("valid") is True,
               action + ": dry-run failed, no write attempted")
        print(action + "_VALIDATE=PASS")
        print(action + "_DRY_RUN=PASS")
        return body, tag

    def write_once(self, body, tag):
        action = body["action"]
        submitted = dict(body, dryRun=False)
        if action == "DELETE":
            submitted["confirmDelete"] = True
        # Do not catch-and-resubmit errors, even after a timeout.
        status, result, _ = self.action("execute", submitted, etag=tag)
        expect(status == 200 and result.get("success") is True and
               result.get("executed") is True and
               result.get("readbackVerified") is True and
               result.get("recordingId") == body["recordingId"] and
               result.get("action") == action,
               action + ": unknown/unconfirmed outcome; STOP, NEVER RETRY")
        print(action + "_EXECUTE=PASS")
        return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", default="http://127.0.0.1:18080")
    parser.add_argument("--vdr-base", default="http://127.0.0.1:8002")
    parser.add_argument("--user", default="vdr-suite")
    parser.add_argument("--backend", default="default")
    parser.add_argument("--folder-id",
                        help="Existing backend-scoped fld1_ ID shown by public folder browse")
    parser.add_argument("--list-folders", action="store_true",
                        help="Read-only listing of existing root fld1_ destinations")
    parser.add_argument("--execute", action="store_true",
                        help="Enable ONE confirmed RENAME -> MOVE -> DELETE sequence")
    args = parser.parse_args()
    expect(VALID_BACKEND.fullmatch(args.backend) is not None,
           "Invalid backend scope")
    if args.folder_id:
        expect(FOLDER_ID.fullmatch(args.folder_id) is not None,
               "Invalid fld1_ target ID")
    if args.execute:
        expect(bool(args.folder_id),
               "Real MOVE needs an existing --folder-id; no writes attempted")
    suite, vdr = local_origin(args.base), local_origin(args.vdr_base)
    password = getpass.getpass("VDR-Suite Human Account password: ")
    client = Client(suite, vdr, args.user, password)
    password = None
    native_rows = client.native()
    native = choose_fixture(native_rows)
    expect(not any(r.get("name") == RENAMED for r in native_rows),
           "Potential RENAME collision with an existing Recording")
    public = correlate(client.public(args.backend), native, args.backend)
    public_id = public["recordingId"]
    print("APPROVED_FIXTURE=" + FIXTURE)
    print("PUBLIC_RECORDING_ID=" + public_id)
    print("SOURCE_DURATION_SECONDS=" + str(native["duration"]))
    print("SOURCE_SIZE_MB=" + str(native["filesize_mb"]))
    print("SOURCE_NATIVE_READBACK=" + native["file_name"])
    folder_name = (client.folder(args.backend, args.folder_id)
                   if args.folder_id else None)
    if folder_name:
        print("TARGET_FOLDER=" + folder_name + " (" + args.folder_id + ")")
    print("CHECK_RECORDING_FINISHED_AND_NOT_ACTIVE=OPERATOR_REQUIRED")
    if args.list_folders:
        print("AVAILABLE_ROOT_FOLDERS:")
        for item in client.folders_at_root(args.backend):
            print("  FOLDER_ID=" + str(item.get("folderId")) +
                  "  NAME=" + str(item.get("name")))
        print("LIVE_MUTATIONS=NOT_RUN")
        return

    # Default run is strictly non-mutating. All applicable actions preflight
    # on the original fixture; no destructive endpoint is reached.
    if not args.execute:
        client.preflight(args.backend, public_id, "RENAME", new_name=RENAMED)
        if args.folder_id:
            client.preflight(args.backend, public_id, "MOVE",
                             folder_id=args.folder_id)
        client.preflight(args.backend, public_id, "DELETE")
        print("R2_RENAME_DELETE_PREFLIGHT=PASS")
        print("R3_MOVE_PREFLIGHT=" + ("PASS" if args.folder_id else "NOT_RUN"))
        print("LIVE_MUTATIONS=NOT_RUN")
        return

    print("WARNING: this run will rename, move and TRASH the exact test recording")
    prompt_for_action("BEGIN", public_id, native["file_name"], folder_name)
    results = {"RENAME": "NOT_RUN", "MOVE": "NOT_RUN",
               "DELETE": "NOT_RUN"}
    try:
        original_path = native["file_name"]
        for action in ("RENAME", "MOVE", "DELETE"):
            # The public identity must remain stable after RENAME / MOVE.
            rows = [r for r in client.public(args.backend)
                    if r.get("recordingId") == public_id]
            expect(len(rows) == 1,
                   action + ": public rec_ missing or ambiguous")
            current = client.native()
            before = [r for r in current
                      if r.get("file_name") == native["file_name"]]
            expect(len(before) == 1, action + ": native source changed")
            if action == "RENAME":
                body, tag = client.preflight(
                    args.backend, public_id, action, new_name=RENAMED)
            elif action == "MOVE":
                body, tag = client.preflight(
                    args.backend, public_id, action, folder_id=args.folder_id)
            else:
                body, tag = client.preflight(args.backend, public_id, action)
            print(action + "_ETAG_PRESENT=PASS")
            prompt_for_action(action, public_id, native["file_name"], folder_name)
            client.write_once(body, tag)  # exactly one mutation POST
            after_native = client.native()
            if action == "DELETE":
                expect(not any(r.get("file_name") == native["file_name"]
                               for r in after_native),
                       "DELETE readback still shows the source; STOP")
                expect(not any(r.get("recordingId") == public_id
                               for r in client.public(args.backend)),
                       "Public rec_ remains after DELETE; STOP")
            else:
                next_native = require_readback(native, after_native, action)
                if action == "RENAME":
                    expect(RENAMED in next_native["file_name"],
                           "RENAME target does not match operator-approved name")
                else:
                    expect(("/" + folder_name + "/") in next_native["file_name"],
                           "MOVE target does not include selected logical folder")
                matches = [r for r in client.public(args.backend)
                           if r.get("recordingId") == public_id]
                expect(len(matches) == 1, "Persistent public rec_ was lost")
                native = next_native
            results[action] = "PASS"
            print(action + "_READBACK=PASS")
    finally:
        for action, result in results.items():
            print("API_V1_RECORDING_" + action + "=" + result)
        print("API_V1_RECORDING_HW_ACCEPTANCE=" +
              ("PASS" if all(x == "PASS" for x in results.values())
               else "FAIL"))
        print("AUTOMATIC_RETRY=DISABLED")


if __name__ == "__main__":
    try:
        main()
    except (Stop, ValueError, KeyError, EOFError, KeyboardInterrupt) as exc:
        print("STOP=" + str(exc), file=sys.stderr)
        sys.exit(1)
