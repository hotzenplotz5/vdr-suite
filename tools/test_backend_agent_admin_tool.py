#!/usr/bin/env python3
"""Black-box contract test for the local Backend Agent administration utility."""

from pathlib import Path
import json
import os
import sqlite3
import subprocess
import sys
import tempfile
import time


def run(arguments: list[str], *, check: bool = False) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        arguments,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=20,
        env={"PATH": os.environ.get("PATH", "/usr/bin:/bin")},
    )
    if check and result.returncode != 0:
        raise AssertionError(
            f"command failed: {arguments!r} code={result.returncode} "
            f"stdout={result.stdout!r} stderr={result.stderr!r}"
        )
    return result


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: test_backend_agent_admin_tool.py ADMIN_BINARY ENROLL_BINARY", file=sys.stderr)
        return 64
    admin = Path(sys.argv[1]).resolve()
    enroll = Path(sys.argv[2]).resolve()
    if not admin.is_file() or not enroll.is_file():
        raise AssertionError("missing Backend Agent utility binary")

    with tempfile.TemporaryDirectory(prefix="vdr-suite-agent-admin-") as directory:
        root = Path(directory)
        database = root / "suite.db"
        package = root / "enrollment"
        run([
            str(enroll), "--database", str(database), "--backend", "default",
            "--output", str(package), "--ttl-seconds", "300",
        ], check=True)

        absent = run([
            str(admin), "--database", str(database), "--backend", "default", "--status",
        ], check=True)
        if json.loads(absent.stdout) != {"present": False}:
            raise AssertionError(f"unexpected absent status: {absent.stdout!r}")

        now = int(time.time())
        connection = sqlite3.connect(database)
        connection.execute("PRAGMA foreign_keys = ON")
        connection.execute(
            "INSERT INTO security_actors (actor_id, actor_type, display_name) VALUES (?, ?, ?)",
            ("actor:test-agent", "system", "Test Backend Agent"),
        )
        connection.execute(
            "INSERT INTO security_devices (device_id, actor_id, display_name) VALUES (?, ?, ?)",
            ("device:test-agent", "actor:test-agent", "Test Backend Agent device"),
        )
        connection.execute(
            "INSERT INTO security_credentials "
            "(credential_id, actor_id, credential_type) VALUES (?, ?, ?)",
            ("credential:test-agent", "actor:test-agent", "agent-basic"),
        )
        connection.execute(
            "INSERT INTO backend_agents "
            "(agent_id, backend_id, actor_id, device_id, credential_id, "
            "credential_generation, agent_instance_id, backend_generation, "
            "protocol_version, software_version, heartbeat_sequence, "
            "capability_revision, last_connected_at, last_heartbeat_at, "
            "lease_expires_at, revoked_at, revocation_reason, incompatible, "
            "created_at, updated_at) "
            "VALUES (?, ?, ?, ?, ?, 1, ?, 1, ?, ?, 3, 1, ?, ?, ?, 0, '', 0, ?, ?)",
            (
                "agt_test", "default", "actor:test-agent", "device:test-agent",
                "credential:test-agent", "instance-test", "vdr-suite-agent/1",
                "vdr-suite-backend-agent/test", now, now, now + 90, now, now,
            ),
        )
        connection.execute(
            "INSERT INTO backend_agent_capabilities "
            "(agent_id, capability_revision, capability_kind, capability_name, capability_value) "
            "VALUES (?, 1, 'mode', 'read-only', 'true')",
            ("agt_test",),
        )
        connection.execute(
            "INSERT INTO backend_agent_capabilities "
            "(agent_id, capability_revision, capability_kind, capability_name, capability_value) "
            "VALUES (?, 1, 'observation', 'backend-health', 'true')",
            ("agt_test",),
        )
        connection.execute(
            "INSERT INTO backend_agent_observation_cursors "
            "(backend_id, observation_domain, agent_id, agent_instance_id, "
            "backend_generation, snapshot_generation, producer_sequence, "
            "resource_revision, payload_identity, captured_at, accepted_at) "
            "VALUES (?, ?, ?, ?, 1, 2, 7, ?, ?, ?, ?)",
            (
                "default", "backend-health", "agt_test", "instance-test",
                "heartbeat-3", "payload-test", now, now,
            ),
        )
        connection.commit()
        connection.close()

        active = run([
            str(admin), "--database", str(database), "--backend", "default", "--status",
        ], check=True)
        status = json.loads(active.stdout)
        expected = {
            "present": True,
            "agentId": "agt_test",
            "backendId": "default",
            "state": "online",
            "backendGeneration": 1,
            "heartbeatSequence": 3,
            "capabilityRevision": 1,
            "readOnly": True,
            "adapters": [],
            "observationDomains": ["backend-health"],
            "backendHealthObservation": {
                "present": True,
                "backendGeneration": 1,
                "snapshotGeneration": 2,
                "producerSequence": 7,
                "resourceRevision": "heartbeat-3",
                "capturedAt": now,
                "acceptedAt": now,
            },
            "channelObservation": {
                "present": False,
                "factCount": 0,
            },
        }
        for key, value in expected.items():
            if status.get(key) != value:
                raise AssertionError(f"status mismatch for {key}: {status!r}")

        revoked = run([
            str(admin), "--database", str(database), "--backend", "default",
            "--revoke", "--reason", "runtime-acceptance-replacement",
        ], check=True)
        if "Backend Agent revoked" not in revoked.stdout:
            raise AssertionError(f"missing revocation confirmation: {revoked.stdout!r}")

        revoked_status = json.loads(run([
            str(admin), "--database", str(database), "--backend", "default", "--status",
        ], check=True).stdout)
        if revoked_status.get("state") != "revoked":
            raise AssertionError(f"revoked status not visible: {revoked_status!r}")

        connection = sqlite3.connect(database)
        agent_row = connection.execute(
            "SELECT revoked_at, revocation_reason FROM backend_agents WHERE agent_id = 'agt_test'"
        ).fetchone()
        device_active = connection.execute(
            "SELECT active FROM security_devices WHERE device_id = 'device:test-agent'"
        ).fetchone()
        credential_active = connection.execute(
            "SELECT active FROM security_credentials WHERE credential_id = 'credential:test-agent'"
        ).fetchone()
        connection.close()
        if not agent_row or agent_row[0] <= 0 or agent_row[1] != "runtime-acceptance-replacement":
            raise AssertionError(f"agent revocation was not persisted: {agent_row!r}")
        if device_active != (0,) or credential_active != (0,):
            raise AssertionError("revoked Agent identity remained active")

        repeated = run([
            str(admin), "--database", str(database), "--backend", "default", "--revoke",
        ])
        if repeated.returncode == 0 or "no active Backend Agent" not in repeated.stderr:
            raise AssertionError("repeated revocation did not fail closed")

    print("test_backend_agent_admin_tool passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
