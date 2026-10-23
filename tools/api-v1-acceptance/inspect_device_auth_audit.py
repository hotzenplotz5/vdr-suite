#!/usr/bin/env python3
"""Read-only audit evidence of an EXISTING Device's authenticated Public-v1 GETs.

This does not authenticate AS the device, read a credential, change grants,
pair, rotate, revoke, issue HTTP requests, or modify VDR-Suite persistence.
Accountability authorization is not HTTP or backend operation success.
"""
from __future__ import annotations

import argparse
import datetime as dt
import os
from pathlib import Path
import re
import sqlite3
import subprocess
import sys
import urllib.parse

DEVICE_DEFAULT = "Hisense VIDAA TV"
AUDIT_BOUND = 500
PERMISSIONS = (
    "channels.view", "epg.view", "recordings.view", "timers.view",
    "searchtimers.view", "media.live.play", "media.recording.play",
)
UTC = dt.timezone.utc


class AuditBlocked(Exception):
    pass


def parse_iso(value):
    if not isinstance(value, str) or len(value) > 48:
        return None
    try:
        parsed = dt.datetime.fromisoformat(value.replace("Z", "+00:00"))
        if parsed.tzinfo is None:
            return None
        return parsed.astimezone(UTC)
    except ValueError:
        return None


def daemon_database_path():
    # Only the running daemon's actual environment can choose the database.
    # NEVER print its environment: other entries can be sensitive.
    try:
        process = subprocess.run(
            ["systemctl", "show", "--property=MainPID", "--value",
             "vdr-suite-daemon.service"], capture_output=True,
            text=True, timeout=5, check=True)
        raw = process.stdout.strip()
        if not raw.isdecimal() or not (1 < int(raw) < 1_000_000_000):
            raise AuditBlocked("daemon PID not established")
        env_bytes = Path("/proc/" + raw + "/environ").read_bytes()
        if len(env_bytes) > 1024 * 1024:
            raise AuditBlocked("daemon environment exceeds safety bound")
        env = {}
        for entry in env_bytes.split(b"\x00"):
            for key in (b"VDR_SUITE_SECURITY_DATABASE_PATH",
                        b"VDR_SUITE_DATABASE_PATH"):
                if entry.startswith(key + b"="):
                    env[key] = os.fsdecode(entry[len(key) + 1:])
        candidate = (env.get(b"VDR_SUITE_SECURITY_DATABASE_PATH") or
                     env.get(b"VDR_SUITE_DATABASE_PATH"))
        if not candidate:
            raise AuditBlocked("no explicit SQLite path in daemon process environment")
        return candidate
    except (OSError, ValueError, subprocess.SubprocessError) as e:
        raise AuditBlocked("cannot resolve running daemon database (" +
                           type(e).__name__ + ")") from e


def open_ro(dbpath):
    if (not isinstance(dbpath, str) or not os.path.isabs(dbpath)
            or "\x00" in dbpath or len(dbpath) > 4096):
        raise AuditBlocked("database path must be an exact absolute path")
    path = Path(dbpath)
    if path.is_symlink() or not path.is_file():
        raise AuditBlocked("configured database is missing, not regular or symlinked")
    # SQLite URI mode=ro and query_only prevent database mutations.
    uri = "file:" + urllib.parse.quote(str(path), safe="/") + "?mode=ro"
    try:
        connection = sqlite3.connect(uri, uri=True, timeout=2)
        connection.execute("PRAGMA query_only = ON")
        required = {"security_devices", "accountability_events"}
        found = {r[0] for r in connection.execute(
            "SELECT name FROM sqlite_master WHERE type='table' "
            "AND name IN ('security_devices', 'accountability_events')")}
        if found != required:
            connection.close()
            raise AuditBlocked("SQLite schema lacks canonical security audit tables")
        return connection
    except sqlite3.Error as e:
        raise AuditBlocked("read-only SQLite connection/query failed (" +
                           type(e).__name__ + ")") from e


def inspect(conn, name, minutes=30, *, current=None):
    if not isinstance(name, str) or not (1 <= len(name) <= 128):
        raise AuditBlocked("invalid exact Device display name")
    if not isinstance(minutes, int) or not (1 <= minutes <= 10080):
        raise AuditBlocked("invalid observation interval")
    now = current or dt.datetime.now(UTC)
    start = now - dt.timedelta(minutes=minutes)
    rows = list(conn.execute(
        "SELECT device_id,actor_id,active,revoked_at "
        "FROM security_devices WHERE display_name=? LIMIT 2", (name,)))
    if len(rows) != 1:
        raise AuditBlocked("Device not uniquely found by exact display name")
    device_id, actor_id, active, revoked = rows[0]
    if active != 1 or revoked:
        raise AuditBlocked("selected Device is not active")
    print("DEVICE:", re.sub(r"[^A-Za-z0-9 äöüÄÖÜß._-]", "?", name))
    print("WINDOW_MINUTES:", minutes)
    print("PERSISTENCE: READ_ONLY")
    print("CREDENTIAL_ACCESS: NONE")
    rows = list(conn.execute(
        "SELECT occurred_at, permission, backend_id, event_type,"
        "decision, authentication_state, actor_type, outcome "
        "FROM accountability_events "
        "WHERE device_id=? AND actor_id=? "
        "ORDER BY rowid DESC LIMIT ?", (device_id, actor_id, AUDIT_BOUND)))
    observed = []
    for occurred, permission, scope, typ, decision, auth, actor, outcome in rows:
        instant = parse_iso(occurred)
        if instant is None or instant < start or instant > now + dt.timedelta(minutes=1):
            continue
        if (typ != "authorization.allowed" or decision != "allowed"
                or outcome != "dispatch_authorized"
                or auth != "authenticated" or actor != "service"):
            continue
        if permission not in PERMISSIONS or scope != "default":
            continue
        observed.append((instant, permission))
    counts = {p: sum(x[1] == p for x in observed) for p in PERMISSIONS}
    for p in PERMISSIONS:
        print(p + "@default:", counts[p], "allowed Device authorization event(s)")
    print("LAST_ALLOWED_DEVICE_GATE_UTC:",
          max(x[0] for x in observed).isoformat() if observed else "NOT_OBSERVED")
    print("DEVICE_AUTHENTICATED_GATE:",
          "PASS" if observed else "NOT_VERIFIED")
    print("HTTP_RESPONSE_AND_VDR_OUTCOME: NOT_VERIFIED_BY_AUDIT")
    if len(rows) == AUDIT_BOUND:
        print("AUDIT_WINDOW: PARTIAL (500-event scan bound)")
    return bool(observed)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Audit existing Device auth without credentials")
    parser.add_argument("--device-name", default=DEVICE_DEFAULT)
    parser.add_argument("--minutes", type=int, default=30)
    parser.add_argument("--database", default=None,
                        help="explicit absolute security SQLite database path; "
                             "otherwise read only from running daemon's environment")
    a = parser.parse_args(argv)
    try:
        path = a.database or daemon_database_path()
        db = open_ro(path)
        try:
            return 0 if inspect(db, a.device_name, a.minutes) else 2
        finally:
            db.close()
    except (AuditBlocked, sqlite3.Error) as e:
        print("AUDIT=BLOCKED: " + (str(e) if isinstance(e, AuditBlocked)
                                    else "SQLite query failure"))
        return 2


if __name__ == "__main__":
    sys.exit(main())
