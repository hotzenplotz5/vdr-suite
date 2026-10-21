#!/usr/bin/env python3
"""Read-only hardware diagnosis for ONE existing uncertain native timer UPDATE.

No API mutation, daemon restart, credential inspection, or direct DB writes.
"""
import json
import sqlite3
import urllib.request
from pathlib import Path

DB = "file:/var/lib/vdr-suite/vdr-suite.db?mode=ro"
OP = "op_84589d48e763200048a1cd39df88e232"
TIMER = "C-1-1051-10301:0:1792533600:1400:1410"

def show(label, row):
    print(label, dict(row) if row is not None else "<missing>", flush=True)

def evidence_fields(encoded):
    prefix = "native-timer-modify-result-evidence/1|"
    if not encoded.startswith(prefix):
        return "UNEXPECTED_PREFIX"
    pos = len(prefix)
    names = ("outcome", "local_starting_at", "dispatch_started_at",
             "completed_at", "evidence_reference")
    parsed = {}
    for name in names:
        separator = encoded.find(":", pos)
        if separator == -1 or not encoded[pos:separator].isdigit():
            return "INVALID_LENGTH_FIELD:" + name
        length = int(encoded[pos:separator])
        pos = separator + 1
        value = encoded[pos:pos+length]
        if len(value) != length or encoded[pos+length:pos+length+1] != "|":
            return "INVALID_VALUE_FIELD:" + name
        parsed[name] = value
        pos += length + 1
    return parsed if pos == len(encoded) else "TRAILING_EVIDENCE"

conn = sqlite3.connect(DB, uri=True, timeout=5)
conn.row_factory = sqlite3.Row
conn.execute("PRAGMA query_only=ON")

op = conn.execute("""
SELECT operation_id, state, operation_revision, backend_generation,
       expected_revision, expected_resource_fingerprint,
       requested_at, updated_at, deadline, result_reference, resource_id
FROM mutation_operations WHERE operation_id=?
""", (OP,)).fetchone()
if not op:
    raise SystemExit("STOP=OP_NOT_FOUND")
op = dict(op)
binding_id = op.pop("resource_id")
print("OPERATION=", op)
bound = conn.execute("""
SELECT backend_native_timer_id, binding_revision, backend_generation,
       observed_fingerprint, observed_title, observed_directory, observed_day,
       observed_weekdays, observed_start_time, observed_end_time,
       observed_enabled, observed_recording, observed_pending,
       last_observed_at, last_verified_operation_id, drift_state
FROM native_timer_bindings WHERE native_timer_binding_id=?
""", (binding_id,)).fetchone()
show("BINDING=", bound)
if bound is not None:
    print("PREDECESSOR_FINGERPRINT_MATCH=",
          op["expected_resource_fingerprint"] == bound["observed_fingerprint"])
agent = conn.execute("""
SELECT backend_generation, lease_expires_at, revoked_at, incompatible,
       agent_instance_id FROM backend_agents
WHERE backend_id='default' AND revoked_at=0
ORDER BY backend_generation DESC LIMIT 1
""").fetchone()
show("LIVE_AGENT=", agent)
rows = conn.execute("""
SELECT c.command_id, c.state, c.operation_id, c.operation_revision,
       c.backend_generation, c.assigned_at, c.deadline,
       r.dispatch_state, r.result_category, r.error_category,
       r.verification_state, r.completed_at, r.bounded_diagnostics,
       r.result_evidence
FROM backend_agent_commands c
LEFT JOIN backend_agent_command_results r ON r.command_id=c.command_id
WHERE c.operation_id=?
""", (OP,)).fetchall()
for row in rows:
    record = dict(row)
    encoded = record.pop("result_evidence") or ""
    print("COMMAND=", record)
    print("COMMAND_EVIDENCE=", evidence_fields(encoded) if encoded else "ABSENT")
conn.close()

with urllib.request.urlopen(
    "http://127.0.0.1:8002/timers.json", timeout=8) as response:
    content = json.load(response)
items = content.get("timers", []) if isinstance(content, dict) else content
matches = [t for t in items if t.get("id") == TIMER]
print("LIVE_TIMER_MATCH_COUNT=", len(matches))
for timer in matches:
    print("LIVE_TIMER=", {k: timer.get(k) for k in
          ("id", "filename", "channel", "event_id", "day", "weekdays",
           "start", "stop", "priority", "lifetime", "flags",
           "is_active", "is_recording", "is_pending")})
print("DIAGNOSIS=READ_ONLY")
