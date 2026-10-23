#!/usr/bin/env python3
"""Read-only evidence chain for already persisted outcome_unknown Operations.

Only counts exact-scope Operation -> reservation/Agent command -> receipt/result.
Does not read payloads or expose IDs; never retries dispatch or reconciles state.
"""
from __future__ import annotations

import argparse
import re
import sqlite3
import sys
import time

from inspect_device_auth_audit import AuditBlocked, daemon_database_path, open_ro

REQUIRED = {
    "mutation_operations": ("operation_id","backend_id","state","resource_type","action_family","result_reference"),
    "backend_agent_command_reservations": ("operation_id","backend_id","command_id"),
    "backend_agent_commands": ("operation_id","backend_id","command_id","state","command_type"),
    "backend_agent_command_receipts": ("command_id",),
    "backend_agent_command_results": (
        "command_id","result_category","verification_state","dispatch_state","error_category","result_evidence"),
}
BUDGET=6.0
RESOURCES=("TimerAssignment","NativeTimerBinding","SearchTimer")
COMMAND_STATES=("assigned","received","completed","expired","waiting_reconciliation")
DISPATCH_STATES=("not_started","starting","accepted_by_executor","effect_reported")
VERIFY_STATES=("verified","outcome_unknown")
RESULT_STATES=("rejected","outcome_unknown","succeeded")
ACTION_FAMILIES=(
    "timer.create","timer.update","timer.toggle","timer.delete",
    "searchtimer.create",
    "searchtimer.native-cas.update","searchtimer.native-cas.activate",
    "searchtimer.native-cas.deactivate","searchtimer.native-cas.delete",
)
COMMAND_TYPES=(
    "timer.create","timer.update","timer.toggle","timer.delete",
    "vdr.timer.create","vdr.timer.update","vdr.timer.modify",
    "vdr.timer.toggle","vdr.timer.delete",
)
ERROR_CATEGORIES=("none","fenced","expired","unsupported","executor_unknown")


def safe_class(label, known, *, absent):
    # Never print arbitrary source/resource/diagnostic text from SQLite.
    if label is None:
        return absent
    return label if isinstance(label,str) and label in known else "other"




def inspect(conn, backend="default"):
    if not isinstance(backend,str) or not re.fullmatch(r"[A-Za-z0-9_.-]{1,128}",backend):
        raise AuditBlocked("invalid backend scope")
    mark=time.monotonic()
    conn.set_progress_handler(lambda:int(time.monotonic()-mark>BUDGET),1000)
    try:
        for table,fields in REQUIRED.items():
            present={row[1] for row in conn.execute(
                "PRAGMA table_info("+table+")")}
            if not set(fields).issubset(present):
                raise AuditBlocked("canonical command evidence table missing or incompatible")
        # DISTINCT by operation, not by number of command attempts. A missing
        # receipt/result is lack of audit evidence, not proof of non-dispatch.
        results=conn.execute("""
            SELECT COUNT(*),
              COALESCE(SUM(CASE WHEN EXISTS (
                SELECT 1 FROM backend_agent_command_reservations r
                WHERE r.backend_id=o.backend_id AND r.operation_id=o.operation_id
              ) THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN EXISTS (
                SELECT 1 FROM backend_agent_commands c
                WHERE c.backend_id=o.backend_id AND c.operation_id=o.operation_id
              ) THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN EXISTS (
                SELECT 1 FROM backend_agent_commands c
                WHERE c.backend_id=o.backend_id AND c.operation_id=o.operation_id
                  AND c.state='waiting_reconciliation'
              ) THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN EXISTS (
                SELECT 1 FROM backend_agent_commands c
                JOIN backend_agent_command_receipts r ON r.command_id=c.command_id
                WHERE c.backend_id=o.backend_id AND c.operation_id=o.operation_id
              ) THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN EXISTS (
                SELECT 1 FROM backend_agent_commands c
                JOIN backend_agent_command_results r ON r.command_id=c.command_id
                WHERE c.backend_id=o.backend_id AND c.operation_id=o.operation_id
              ) THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN EXISTS (
                SELECT 1 FROM backend_agent_commands c
                JOIN backend_agent_command_results r ON r.command_id=c.command_id
                WHERE c.backend_id=o.backend_id AND c.operation_id=o.operation_id
                  AND r.result_category='outcome_unknown'
                  AND r.verification_state='outcome_unknown'
              ) THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN EXISTS (
                SELECT 1 FROM backend_agent_commands c
                JOIN backend_agent_command_results r ON r.command_id=c.command_id
                WHERE c.backend_id=o.backend_id AND c.operation_id=o.operation_id
                  AND r.dispatch_state='accepted_by_executor'
              ) THEN 1 ELSE 0 END),0)
            FROM mutation_operations o
            WHERE o.backend_id=? AND o.state='outcome_unknown'
        """,(backend,)).fetchone()
        if not results or len(results)!=8 or any(
            not isinstance(x,int) or x<0 for x in results):
            raise AuditBlocked("invalid bounded command evidence aggregate")
        # Group *only* existing unknown Operation chains, including chains
        # without an Agent Command or without a Result. Fixed allowlisted
        # labels prevent resource names / untrusted result strings from
        # escaping to terminal or public issue logs. DISTINCT prevents a
        # duplicated command/attempt from inflating an operation count.
        groups=list(conn.execute("""
            SELECT
              CASE
                WHEN o.resource_type IN
                   ('TimerAssignment','NativeTimerBinding','SearchTimer')
                THEN o.resource_type ELSE 'other' END,
              CASE
                WHEN c.command_id IS NULL THEN 'no_command'
                WHEN c.state IN ('assigned','received','completed',
                    'expired','waiting_reconciliation') THEN c.state
                ELSE 'other' END,
              CASE
                WHEN x.command_id IS NULL THEN 'no_result'
                WHEN x.dispatch_state IN ('not_started','starting',
                    'accepted_by_executor','effect_reported')
                  THEN x.dispatch_state ELSE 'other' END,
              CASE
                WHEN x.command_id IS NULL THEN 'no_result'
                WHEN x.verification_state IN ('verified','outcome_unknown')
                  THEN x.verification_state ELSE 'other' END,
              CASE
                WHEN x.command_id IS NULL THEN 'no_result'
                WHEN x.result_category IN
                    ('rejected','outcome_unknown','succeeded')
                  THEN x.result_category ELSE 'other' END,
              CASE
                WHEN o.action_family IN
                   ('timer.create','timer.update','timer.toggle','timer.delete',
                    'searchtimer.create','searchtimer.native-cas.update',
                    'searchtimer.native-cas.activate',
                    'searchtimer.native-cas.deactivate',
                    'searchtimer.native-cas.delete')
                THEN o.action_family ELSE 'other' END,
              CASE
                WHEN c.command_id IS NULL THEN 'no_command'
                WHEN c.command_type IN
                   ('timer.create','timer.update','timer.toggle','timer.delete',
                    'vdr.timer.create','vdr.timer.update','vdr.timer.modify',
                    'vdr.timer.toggle','vdr.timer.delete')
                THEN c.command_type ELSE 'other' END,
              CASE
                WHEN x.command_id IS NULL THEN 'no_result'
                WHEN x.error_category IN
                   ('none','fenced','expired','unsupported','executor_unknown')
                THEN x.error_category ELSE 'other' END,
              COUNT(DISTINCT o.operation_id)
            FROM mutation_operations o
            LEFT JOIN backend_agent_commands c
              ON c.backend_id=o.backend_id AND c.operation_id=o.operation_id
            LEFT JOIN backend_agent_command_results x
              ON x.command_id=c.command_id
            WHERE o.backend_id=? AND o.state='outcome_unknown'
            GROUP BY 1,2,3,4,5,6,7,8 ORDER BY 1,2,3,4,5,6,7,8
        """,(backend,)))
        if (len(groups)>int(results[0])*10+1 or any(
              len(row)!=9 or not isinstance(row[8],int) or row[8]<0
              for row in groups)):
            raise AuditBlocked("invalid bounded command group evidence")
        # Diagnose ONLY eligibility evidence for the narrow source fix:
        # an exact old quarantine marker and a typed nonempty Agent result
        # attached to the very same original toggle Operation. It is not a
        # proof that the evidence is cryptographically/canonically valid or
        # that the original bound timer is still unchanged.
        toggle = conn.execute("""
            SELECT COUNT(*),
              COALESCE(SUM(CASE WHEN
                o.result_reference='native-timer-modify:missing-result-evidence'
                THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN EXISTS (
                 SELECT 1 FROM backend_agent_commands c
                 JOIN backend_agent_command_results r
                    ON r.command_id=c.command_id
                 WHERE c.backend_id=o.backend_id
                   AND c.operation_id=o.operation_id
                   AND c.command_type='vdr.timer.toggle'
                   AND r.result_evidence<>''
                   AND r.dispatch_state='not_started'
                   AND r.verification_state='verified'
                   AND r.result_category='rejected'
              ) THEN 1 ELSE 0 END),0),
              COALESCE(SUM(CASE WHEN
                o.result_reference='native-timer-modify:missing-result-evidence'
                AND EXISTS (
                  SELECT 1 FROM backend_agent_commands c
                  JOIN backend_agent_command_results r
                    ON r.command_id=c.command_id
                  WHERE c.backend_id=o.backend_id
                    AND c.operation_id=o.operation_id
                    AND c.command_type='vdr.timer.toggle'
                    AND r.result_evidence<>''
                    AND r.dispatch_state='not_started'
                    AND r.verification_state='verified'
                    AND r.result_category='rejected'
                ) THEN 1 ELSE 0 END),0)
            FROM mutation_operations o
            WHERE o.backend_id=? AND o.state='outcome_unknown'
              AND o.resource_type='NativeTimerBinding'
              AND o.action_family='timer.toggle'
        """,(backend,)).fetchone()
        if not toggle or len(toggle)!=4 or any(
            not isinstance(x,int) or x<0 for x in toggle):
            raise AuditBlocked("invalid read-only quarantine evidence count")
    except sqlite3.Error as error:
        raise AuditBlocked("read-only command evidence query failed or exceeded budget ("
                           +type(error).__name__+")") from error
    finally:
        conn.set_progress_handler(None,0)
    names=(
        "OUTCOME_UNKNOWN_OPERATIONS",
        "WITH_COMMAND_RESERVATION",
        "WITH_BACKEND_AGENT_COMMAND",
        "WITH_WAITING_RECONCILIATION_COMMAND",
        "WITH_COMMAND_RECEIPT",
        "WITH_COMMAND_RESULT",
        "WITH_UNKNOWN_COMMAND_RESULT",
        "WITH_ACCEPTED_BY_EXECUTOR_RESULT",
    )
    print("BACKEND:",backend)
    print("DATABASE_ACCESS: READ_ONLY")
    for name,value in zip(names,results):
        print(name+":",value)
    print("=== OUTCOME_UNKNOWN_COMMAND_CHAIN_BY_RESOURCE ===")
    for resource, command, dispatch, verification, result, action, cmd_type, error, count in groups:
        # SQL CASE values are hardcoded above, but retain a second check
        # against unexpected types/source differences before publication.
        fields=(
            safe_class(resource,RESOURCES,absent="other"),
            safe_class(command,COMMAND_STATES,absent="no_command")
              if command!="no_command" else "no_command",
            safe_class(dispatch,DISPATCH_STATES,absent="no_result")
              if dispatch!="no_result" else "no_result",
            safe_class(verification,VERIFY_STATES,absent="no_result")
              if verification!="no_result" else "no_result",
            safe_class(result,RESULT_STATES,absent="no_result")
              if result!="no_result" else "no_result",
            safe_class(action,ACTION_FAMILIES,absent="other"),
            safe_class(cmd_type,COMMAND_TYPES,absent="no_command")
              if cmd_type!="no_command" else "no_command",
            safe_class(error,ERROR_CATEGORIES,absent="no_result")
              if error!="no_result" else "no_result",
        )
        print("CHAIN resource={} command={} dispatch={} verification={} result={} action={} command_type={} error={} operation_count={}".format(*fields,count))
    print("=== NATIVE_TOGGLE_VERIFIED_REJECTION_CANDIDACY ===")
    for name,count in zip((
        "UNKNOWN_NATIVE_TOGGLE_OPERATIONS",
        "WITH_EXACT_EVIDENCE_FREE_QUARANTINE_MARKER",
        "WITH_NONEMPTY_TYPED_AGENT_NO_EFFECT_RESULT",
        "WITH_BOTH_SAME_OPERATION",
    ),toggle):
        print(name+":",count)
    print("NONEMPTY_EVIDENCE_IS_NOT_VALIDATED_AGENT_EVIDENCE: TRUE")
    print("UNCHANGED_BINDING_NOT_PROVEN_BY_THIS_QUERY: TRUE")
    print("RECOVERY_OR_STATE_CHANGE_PERFORMED: FALSE")
    print("CHAIN_GROUP_COUNTS_MAY_OVERLAP_IF_MULTIPLE_COMMANDS: TRUE")
    print("COUNT_UNIT: DISTINCT_OPERATION")
    print("MISSING_COMMAND_EVIDENCE_IS_NOT_PROOF_OF_NO_DISPATCH: TRUE")
    print("RESULT_EVIDENCE_IS_NOT_NATIVE_READBACK: TRUE")
    print("RETRY_OR_RECONCILIATION_EXECUTED: FALSE")
    return dict(zip(names,results))


def main(argv=None):
    parser=argparse.ArgumentParser(
        description="Count existing uncertain mutation command evidence without writes")
    parser.add_argument("--backend",default="default")
    parser.add_argument("--database",default=None)
    args=parser.parse_args(argv)
    try:
        db=open_ro(args.database or daemon_database_path())
        try:
            inspect(db,args.backend)
        finally:
            db.close()
        return 0
    except AuditBlocked as error:
        print("RESULT=BLOCKED: "+str(error))
        return 2


if __name__=="__main__":
    sys.exit(main())
