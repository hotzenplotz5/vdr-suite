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
    "mutation_operations": ("operation_id","backend_id","state"),
    "backend_agent_command_reservations": ("operation_id","backend_id","command_id"),
    "backend_agent_commands": ("operation_id","backend_id","command_id","state"),
    "backend_agent_command_receipts": ("command_id",),
    "backend_agent_command_results": (
        "command_id","result_category","verification_state","dispatch_state"),
}
BUDGET=6.0


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
