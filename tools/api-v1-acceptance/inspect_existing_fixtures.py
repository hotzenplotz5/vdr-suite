#!/usr/bin/env python3
"""Identify *existing* native-timer/operation fixture presence without writes.

Reads only aggregate table state counts from the actual daemon's SQLite DB.
No object IDs, account/actor IDs, titles, paths, payloads, or secrets printed.
SQLite read-only URI + query_only, short query timeout, no HTTP/Device actions.
"""
from __future__ import annotations

import argparse
import re
import sqlite3
import sys
import time

from inspect_device_auth_audit import AuditBlocked, daemon_database_path, open_ro

SAFE_SCOPE = re.compile(r"[A-Za-z0-9_.-]{1,128}\Z")
SPECS = (
    ("TIMER_ASSIGNMENTS", "timer_assignments", "state", ("backend_id", "state")),
    ("MUTATION_OPERATIONS", "mutation_operations", "state", ("backend_id", "state")),
    ("NATIVE_TIMER_BINDINGS", "native_timer_bindings", "ownership",
     ("backend_id", "ownership")),
)
QUERY_BUDGET_SECONDS = 6.0


def inventory(conn, backend="default"):
    if not isinstance(backend, str) or not SAFE_SCOPE.fullmatch(backend):
        raise AuditBlocked("backend scope is not an allowed opaque identifier")
    start = time.monotonic()

    def abort_slow():
        return int(time.monotonic() - start > QUERY_BUDGET_SECONDS)

    conn.set_progress_handler(abort_slow, 1000)
    try:
        tables = {row[0] for row in conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table' "
            "AND name IN ('timer_assignments','mutation_operations','native_timer_bindings')"
        )}
        print("DATABASE_ACCESS: READ_ONLY")
        print("BACKEND: " + backend)
        print("AUTH_OR_CREDENTIAL_ACTIONS: NONE")
        for label, table, group, columns in SPECS:
            if table not in tables:
                print(label + ": SCHEMA_NOT_INSTALLED")
                continue
            actual = {row[1] for row in conn.execute("PRAGMA table_info(" + table + ")")}
            if not set(columns).issubset(actual):
                print(label + ": SCHEMA_NOT_COMPATIBLE")
                continue
            # Both table and column names are fixed in source code, not input.
            rows = conn.execute(
                "SELECT " + group + ",COUNT(*) FROM " + table +
                " WHERE backend_id=? GROUP BY " + group, (backend,))
            counts = []
            for status, count in rows:
                if (not isinstance(status,str) or
                        not re.fullmatch(r"[a-z][a-z0-9_-]{0,63}",status) or
                        not isinstance(count,int) or count < 0):
                    raise AuditBlocked("unexpected enum state in read-only fixture inventory")
                counts.append((status,count))
            print(label + ": " + str(sum(n for _,n in counts)) +
                  " existing row(s) for selected backend")
            for state, number in sorted(counts):
                print("  " + state + "=" + str(number))
        print("EVIDENCE_SCOPE: DATABASE_FIXTURE_PRESENCE_ONLY")
        print("HTTP_AUTHORIZATION_AND_NATIVE_READBACK: NOT_VERIFIED")
    except sqlite3.Error as e:
        raise AuditBlocked("read-only fixture SQL failed or exceeded safety budget (" +
                           type(e).__name__ + ")") from e
    finally:
        conn.set_progress_handler(None, 0)


def main(argv=None):
    parser=argparse.ArgumentParser(
        description="Count existing operation/timer fixtures without modifying yaVDR")
    parser.add_argument("--backend",default="default")
    parser.add_argument("--database",default=None,
                        help="optional explicit absolute SQLite path")
    args=parser.parse_args(argv)
    try:
        db=open_ro(args.database or daemon_database_path())
        try:
            inventory(db,args.backend)
        finally:
            db.close()
        return 0
    except AuditBlocked as e:
        print("RESULT=BLOCKED: " + str(e))
        return 2


if __name__=="__main__":
    sys.exit(main())
