#!/usr/bin/env python3
"""Correlate real persisted TimerAssignment, NativeTimerBinding and Operation state.

Read only, no raw identifiers or sensitive resource fields in stdout, no GET
authentication, no recovery/retry/mutations or misleading Public-v1 PASS claim.
"""
from __future__ import annotations

import argparse
import re
import sqlite3
import sys
import time

from inspect_device_auth_audit import AuditBlocked, daemon_database_path, open_ro

SCOPE = re.compile(r"[A-Za-z0-9_.-]{1,128}\Z")
TABLES = {
    "timer_assignments": {"timer_assignment_id", "native_timer_binding_id",
                          "backend_generation", "state", "backend_id", "updated_at"},
    "native_timer_bindings": {"native_timer_binding_id", "timer_assignment_id",
                              "backend_generation", "ownership", "backend_id",
                              "last_verified_operation_id", "missing_since",
                              "drift_state"},
    "mutation_operations": {"operation_id", "backend_id", "state", "updated_at",
                            "deadline"},
}
QUERY_BUDGET_SECONDS = 6.0
DAY = 86400


def correlate(conn, backend="default", *, now=None):
    if not isinstance(backend, str) or not SCOPE.fullmatch(backend):
        raise AuditBlocked("invalid backend identifier")
    tick = int(time.time()) if now is None else int(now)
    if tick < 1:
        raise AuditBlocked("invalid observation timestamp")
    cutoff = tick - DAY
    start = time.monotonic()
    conn.set_progress_handler(
        lambda: int(time.monotonic() - start > QUERY_BUDGET_SECONDS), 1000)
    try:
        found = {row[0] for row in conn.execute(
            "SELECT name FROM sqlite_master WHERE type='table' "
            "AND name IN ('timer_assignments','native_timer_bindings',"
            "'mutation_operations')")}
        if found != set(TABLES):
            raise AuditBlocked("required canonical timer/operation tables not installed")
        for table, columns in TABLES.items():
            actual = {row[1] for row in conn.execute("PRAGMA table_info(" + table + ")")}
            if not columns.issubset(actual):
                raise AuditBlocked("canonical timer/operation schema not compatible")
        # Bound TimerAssignment contracts require a reciprocally owned and
        # generation-matching native binding (not a VDR runtime readback).
        bound, gaps, unverified = conn.execute("""
            SELECT COUNT(*),
                COALESCE(SUM(CASE WHEN
                    a.native_timer_binding_id=''
                    OR NOT EXISTS (
                        SELECT 1 FROM native_timer_bindings b
                        WHERE b.native_timer_binding_id=a.native_timer_binding_id
                          AND b.backend_id=a.backend_id
                          AND b.backend_generation=a.backend_generation
                          AND b.timer_assignment_id=a.timer_assignment_id
                          AND b.ownership='managed')
                    THEN 1 ELSE 0 END),0),
                COALESCE(SUM(CASE WHEN EXISTS (
                    SELECT 1 FROM native_timer_bindings b
                    WHERE b.native_timer_binding_id=a.native_timer_binding_id
                      AND b.backend_id=a.backend_id
                      AND b.backend_generation=a.backend_generation
                      AND b.timer_assignment_id=a.timer_assignment_id
                      AND b.ownership='managed'
                      AND (b.last_verified_operation_id=''
                           OR b.missing_since<>0 OR b.drift_state<>'none'))
                    THEN 1 ELSE 0 END),0)
            FROM timer_assignments a
            WHERE a.backend_id=? AND a.state='bound'
            """, (backend,)).fetchone()
        managed, reciprocal_gaps = conn.execute("""
            SELECT COUNT(*),
                   COALESCE(SUM(CASE WHEN NOT EXISTS (
                       SELECT 1 FROM timer_assignments a
                       WHERE a.timer_assignment_id=b.timer_assignment_id
                         AND a.backend_id=b.backend_id
                         AND a.backend_generation=b.backend_generation
                         AND a.native_timer_binding_id=b.native_timer_binding_id
                         AND a.state='bound')
                   THEN 1 ELSE 0 END),0)
            FROM native_timer_bindings b
            WHERE b.backend_id=? AND b.ownership='managed'
            """, (backend,)).fetchone()
        provisioning, aged_provisioning = conn.execute("""
            SELECT COUNT(*),
                COALESCE(SUM(CASE WHEN updated_at>0 AND updated_at<=?
                                  THEN 1 ELSE 0 END),0)
            FROM timer_assignments
            WHERE backend_id=? AND state='provisioning'
            """, (cutoff, backend)).fetchone()
        unknown, aged_unknown, overdue_unknown = conn.execute("""
            SELECT COUNT(*),
                COALESCE(SUM(CASE WHEN updated_at>0 AND updated_at<=?
                                  THEN 1 ELSE 0 END),0),
                COALESCE(SUM(CASE WHEN deadline>0 AND deadline<?
                                  THEN 1 ELSE 0 END),0)
            FROM mutation_operations
            WHERE backend_id=? AND state='outcome_unknown'
            """, (cutoff, tick, backend)).fetchone()
        # Operation-id references are not public GET proof: missing historical
        # operations can be retained/archived independently.
        missing_op_reference = conn.execute("""
            SELECT COUNT(*) FROM native_timer_bindings b
            WHERE b.backend_id=? AND b.ownership='managed'
              AND b.last_verified_operation_id<>''
              AND NOT EXISTS (
                  SELECT 1 FROM mutation_operations o
                  WHERE o.operation_id=b.last_verified_operation_id
                    AND o.backend_id=b.backend_id
                    AND o.state='succeeded')
            """, (backend,)).fetchone()[0]
        values = {
            "BOUND_ASSIGNMENTS": bound,
            "BOUND_ASSIGNMENTS_WITHOUT_MATCHING_MANAGED_BINDING": gaps,
            "BOUND_ASSIGNMENTS_WITH_NONCLEAN_BINDING_OBSERVATION": unverified,
            "MANAGED_BINDINGS": managed,
            "MANAGED_BINDINGS_WITHOUT_RECIPROCAL_BOUND_ASSIGNMENT": reciprocal_gaps,
            "PROVISIONING_ASSIGNMENTS": provisioning,
            "PROVISIONING_UPDATED_OVER_24H_AGO": aged_provisioning,
            "OUTCOME_UNKNOWN_OPERATIONS": unknown,
            "OUTCOME_UNKNOWN_UPDATED_OVER_24H_AGO": aged_unknown,
            "OUTCOME_UNKNOWN_PAST_DECLARED_DEADLINE": overdue_unknown,
            "MANAGED_BINDINGS_WITHOUT_SUCCEEDED_VERIFICATION_OPERATION": missing_op_reference,
        }
    except sqlite3.Error as error:
        raise AuditBlocked("read-only consistency query aborted or failed (" +
                           type(error).__name__ + ")") from error
    finally:
        conn.set_progress_handler(None, 0)
    if any(not isinstance(v, int) or v<0 for v in values.values()):
        raise AuditBlocked("invalid nonnegative aggregate count")
    print("BACKEND:", backend)
    print("DATABASE_ACCESS: READ_ONLY")
    print("CHECK: PERSISTED_TIMER_OPERATION_RELATIONSHIPS")
    for key, value in values.items():
        print(key + ":", value)
    print("BOUND_RELATIONSHIP:",
          "CONSISTENT_PERSISTED_LINKS" if gaps == 0 and reciprocal_gaps == 0
          else "NEEDS_INVESTIGATION")
    print("AGE_SCOPE: 24H_FROM_CURRENT_UTC_CLOCK")
    print("OPERATIONS_OUTCOME_UNKNOWN: NEVER_AUTOMATICALLY_REDISPATCH")
    print("PUBLIC_API_GET_AND_NATIVE_VDR_READBACK: NOT_VERIFIED")
    return values


def main(argv=None):
    parser = argparse.ArgumentParser(description="Read-only persisted timer-operation consistency")
    parser.add_argument("--backend", default="default")
    parser.add_argument("--database", default=None,
                        help="explicit absolute SQLite path, otherwise daemon process environment")
    args = parser.parse_args(argv)
    try:
        db = open_ro(args.database or daemon_database_path())
        try:
            correlate(db, args.backend)
        finally:
            db.close()
        return 0
    except AuditBlocked as error:
        print("RESULT=BLOCKED:", str(error))
        return 2


if __name__ == "__main__":
    sys.exit(main())
