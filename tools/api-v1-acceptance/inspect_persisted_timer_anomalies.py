#!/usr/bin/env python3
"""Read-only classification of persisted timer observations and unknown outcomes.

No IDs, filenames, titles, request payloads, credentials or raw row data are
emitted. A dispatch deadline is not a reconciliation deadline. No redispatch.
"""
from __future__ import annotations

import argparse
import re
import sqlite3
import sys
import time

from inspect_device_auth_audit import AuditBlocked, daemon_database_path, open_ro

DRIFT_STATES = ("none", "expected_transition", "external_field_change",
                "external_disable", "external_delete",
                "native_identity_changed", "ambiguous")
POLICIES = ("none", "readback_required", "event_confirmation",
            "reconciliation_required")
RESOURCES = ("TimerIntent", "TimerAssignment", "NativeTimer",
             "NativeTimerBinding", "SearchTimer", "Recording",
             "RecordingPlaybackSession")
TABLES = {
    "timer_assignments": {"timer_assignment_id", "backend_id",
                          "backend_generation", "state",
                          "updated_at", "native_timer_binding_id"},
    "native_timer_bindings": {"native_timer_binding_id", "timer_assignment_id",
                              "backend_id", "backend_generation", "ownership",
                              "last_verified_operation_id",
                              "missing_since", "drift_state"},
    "mutation_operations": {"backend_id", "state", "updated_at", "deadline",
                            "resource_type", "verification_policy"},
}
DAY=86400


def _label_counts(rows, labels, prefix):
    output={label:0 for label in labels}
    output["other"]=0
    for label, count in rows:
        if not isinstance(count,int) or count<0:
            raise AuditBlocked("invalid grouped aggregate")
        output[label if label in labels else "other"]+=count
    for label,count in output.items():
        if count:
            print(prefix+label.upper()+":",count)
    return output


def classify(conn, backend="default", *, now=None):
    if not isinstance(backend,str) or not re.fullmatch(
            r"[A-Za-z0-9_.-]{1,128}",backend):
        raise AuditBlocked("invalid backend")
    tick=int(time.time()) if now is None else int(now)
    if tick <= 0:
        raise AuditBlocked("invalid current time")
    start=time.monotonic()
    conn.set_progress_handler(lambda:int(time.monotonic()-start>6.0),1000)
    try:
        for table,required in TABLES.items():
            actual={row[1] for row in conn.execute(
                "PRAGMA table_info("+table+")")}
            if not required.issubset(actual):
                raise AuditBlocked("required canonical schema unavailable")
        # Only matched bound/managed pairs; the prior correlation check
        # separately verifies that all bound/managed links are reciprocal.
        where=(
            " FROM timer_assignments a "
            "JOIN native_timer_bindings b "
            "ON b.native_timer_binding_id=a.native_timer_binding_id "
            "AND b.timer_assignment_id=a.timer_assignment_id "
            "AND b.backend_id=a.backend_id "
            "AND b.backend_generation=a.backend_generation "
            "WHERE a.backend_id=? AND a.state='bound' "
            "AND b.ownership='managed'"
        )
        drift=list(conn.execute(
            "SELECT b.drift_state,COUNT(*)"+where+
            " GROUP BY b.drift_state",(backend,)))
        factors=conn.execute(
            "SELECT COUNT(*), "
            "COALESCE(SUM(CASE WHEN b.missing_since<>0 THEN 1 ELSE 0 END),0), "
            "COALESCE(SUM(CASE WHEN b.last_verified_operation_id='' "
            "THEN 1 ELSE 0 END),0), "
            "COALESCE(SUM(CASE WHEN b.drift_state<>'none' "
            "THEN 1 ELSE 0 END),0)"+where,(backend,)).fetchone()
        pending=conn.execute("""
            SELECT COUNT(*),
              COALESCE(SUM(CASE WHEN a.updated_at>0
                    AND a.updated_at<=? THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN a.native_timer_binding_id<>''
                    THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN EXISTS (
                SELECT 1 FROM native_timer_bindings b
                WHERE b.timer_assignment_id=a.timer_assignment_id
                  AND b.backend_id=a.backend_id
                  AND b.backend_generation=a.backend_generation
                  AND b.ownership='managed')
                    THEN 1 ELSE 0 END),0)
            FROM timer_assignments a
            WHERE a.backend_id=? AND a.state='provisioning'
        """,(tick-7*DAY,backend)).fetchone()
        unknown=conn.execute("""
            SELECT COUNT(*),
              COALESCE(SUM(CASE WHEN updated_at>0 AND updated_at<=?
                    THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN deadline=0 THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN deadline>0 AND deadline<?
                    THEN 1 ELSE 0 END),0)
            FROM mutation_operations
            WHERE backend_id=? AND state='outcome_unknown'
        """,(tick-7*DAY,tick,backend)).fetchone()
        policy_rows=list(conn.execute("""
            SELECT verification_policy,COUNT(*) FROM mutation_operations
            WHERE backend_id=? AND state='outcome_unknown'
            GROUP BY verification_policy
        """,(backend,)))
        resource_rows=list(conn.execute("""
            SELECT resource_type,COUNT(*) FROM mutation_operations
            WHERE backend_id=? AND state='outcome_unknown'
            GROUP BY resource_type
        """,(backend,)))
    except sqlite3.Error as error:
        raise AuditBlocked("aggregate SQL unavailable or exceeded 6s budget ("+
                           type(error).__name__+")") from error
    finally:
        conn.set_progress_handler(None,0)
    for value in (*factors,*pending,*unknown):
        if not isinstance(value,int) or value<0:
            raise AuditBlocked("unexpected aggregate value")
    print("BACKEND:",backend)
    print("DATABASE_ACCESS: READ_ONLY")
    print("BOUND_MATCHED_MANAGED_BINDINGS:",factors[0])
    print("BOUND_BINDINGS_MISSING_SINCE_SET:",factors[1])
    print("BOUND_BINDINGS_EMPTY_VERIFICATION_REFERENCE:",factors[2])
    print("BOUND_BINDINGS_NON_NONE_DRIFT:",factors[3])
    drift_summary=_label_counts(drift,DRIFT_STATES,"BOUND_DRIFT_")
    print("PROVISIONING_ASSIGNMENTS:",pending[0])
    print("PROVISIONING_UPDATED_OVER_7D_AGO:",pending[1])
    print("PROVISIONING_BINDING_ID_PRESENT:",pending[2])
    print("PROVISIONING_WITH_MATCHED_MANAGED_BINDING:",pending[3])
    print("OUTCOME_UNKNOWN_OPERATIONS:",unknown[0])
    print("OUTCOME_UNKNOWN_UPDATED_OVER_7D_AGO:",unknown[1])
    print("OUTCOME_UNKNOWN_WITHOUT_DISPATCH_DEADLINE:",unknown[2])
    print("OUTCOME_UNKNOWN_PAST_DISPATCH_DEADLINE:",unknown[3])
    policies=_label_counts(policy_rows,POLICIES,"OUTCOME_UNKNOWN_POLICY_")
    resources=_label_counts(resource_rows,RESOURCES,"OUTCOME_UNKNOWN_RESOURCE_")
    print("NOTE: Flags may overlap; counts are not additive.")
    print("DISPATCH_DEADLINE_IS_NOT_RECONCILIATION_DEADLINE: TRUE")
    print("RETRY_OR_RECONCILIATION_EXECUTED: FALSE")
    print("HTTP_AND_NATIVE_VDR_READBACK: NOT_VERIFIED")
    return {"bound_factors":factors,"drift":drift_summary,
            "provisioning":pending,"unknown":unknown,
            "policies":policies,"resources":resources}


def main(argv=None):
    parser=argparse.ArgumentParser(
        description="Read-only aggregate diagnostics for existing timer state")
    parser.add_argument("--backend",default="default")
    parser.add_argument("--database",default=None)
    args=parser.parse_args(argv)
    try:
        db=open_ro(args.database or daemon_database_path())
        try:
            classify(db,args.backend)
        finally:
            db.close()
        return 0
    except AuditBlocked as error:
        print("RESULT=BLOCKED: "+str(error))
        return 2


if __name__=="__main__":
    sys.exit(main())
