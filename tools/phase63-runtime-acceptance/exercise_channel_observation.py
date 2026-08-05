#!/usr/bin/env python3
"""Exercise Channel replay and sequence-gap handling without exposing secrets."""

from __future__ import annotations

import argparse
import base64
import http.client
import json
import os
from pathlib import Path
import sqlite3
import ssl
import stat
import sys
import tempfile
import time
from urllib.parse import urlsplit

MAX_RESPONSE_BYTES = 64 * 1024
ROUTE = "/api/agent/v1/observations/channels"


class AcceptanceError(RuntimeError):
    pass


def parse_protected_identity(path: Path) -> dict[str, str]:
    metadata = path.lstat()
    if stat.S_ISLNK(metadata.st_mode) or not stat.S_ISREG(metadata.st_mode):
        raise AcceptanceError("identity_not_regular")
    if metadata.st_mode & (stat.S_IRWXG | stat.S_IRWXO):
        raise AcceptanceError("identity_permissions_too_open")
    values: dict[str, str] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        if not raw_line or raw_line.startswith("#"):
            continue
        key, separator, value = raw_line.partition("=")
        if not separator or not key or key in values:
            raise AcceptanceError("identity_parse_failed")
        values[key] = value
    if values.get("version") not in {"1", "2", "3"}:
        raise AcceptanceError("identity_version_invalid")
    if not values.get("agent_id") or len(values.get("credential_secret", "")) < 32:
        raise AcceptanceError("identity_material_invalid")
    return values


def parse_control_plane_url(value: str) -> tuple[str, int, str]:
    parsed = urlsplit(value)
    if (
        parsed.scheme != "https"
        or not parsed.hostname
        or parsed.username is not None
        or parsed.password is not None
        or parsed.query
        or parsed.fragment
        or value.endswith("/")
    ):
        raise AcceptanceError("control_plane_url_invalid")
    return parsed.hostname, parsed.port or 443, parsed.path.rstrip("/") + ROUTE


def open_read_only_database(path: Path) -> sqlite3.Connection:
    return sqlite3.connect(f"file:{path}?mode=ro", uri=True, timeout=10)


def fact_from_row(row: tuple[object, ...]) -> dict[str, object]:
    return {
        "channelId": str(row[0]),
        "channelNumber": int(row[1]),
        "name": str(row[2]),
        "provider": str(row[3]),
        "groupName": str(row[4]),
        "radio": bool(row[5]),
        "encrypted": bool(row[6]),
        "enabled": bool(row[7]),
    }


def canonical_fact(fact: dict[str, object]) -> str:
    ordered = {
        "channelId": fact["channelId"],
        "channelNumber": fact["channelNumber"],
        "name": fact["name"],
        "provider": fact["provider"],
        "groupName": fact["groupName"],
        "radio": fact["radio"],
        "encrypted": fact["encrypted"],
        "enabled": fact["enabled"],
    }
    return json.dumps(ordered, ensure_ascii=False, separators=(",", ":"))


def canonical_snapshot(facts: list[dict[str, object]]) -> str:
    ordered = sorted(facts, key=lambda item: str(item["channelId"]))
    return '{"channels":[' + ",".join(canonical_fact(item) for item in ordered) + "]}"


def current_protocol_state(
    database_path: Path,
    backend_id: str,
    expected_agent_id: str,
) -> tuple[dict[str, object], dict[str, object], list[dict[str, object]]]:
    connection = open_read_only_database(database_path)
    try:
        agent_row = connection.execute(
            "SELECT agent_id, agent_instance_id, backend_generation, "
            "heartbeat_sequence, lease_expires_at FROM backend_agents "
            "WHERE backend_id = ? AND revoked_at = 0 "
            "ORDER BY updated_at DESC LIMIT 1",
            (backend_id,),
        ).fetchone()
        cursor_row = connection.execute(
            "SELECT agent_id, agent_instance_id, backend_generation, "
            "snapshot_generation, producer_sequence, resource_revision, "
            "payload_identity, captured_at FROM backend_agent_observation_cursors "
            "WHERE backend_id = ? AND observation_domain = 'channels'",
            (backend_id,),
        ).fetchone()
        if agent_row is None or cursor_row is None:
            raise AcceptanceError("channel_observation_state_missing")
        if agent_row[0] != expected_agent_id or cursor_row[0] != expected_agent_id:
            raise AcceptanceError("channel_observation_agent_mismatch")
        if tuple(agent_row[1:3]) != tuple(cursor_row[1:3]):
            raise AcceptanceError("channel_observation_lineage_mismatch")
        if int(agent_row[4]) <= int(time.time()):
            raise AcceptanceError("agent_lease_expired")
        receipt_row = connection.execute(
            "SELECT kind, captured_at, resource_revision, payload_identity, "
            "canonical_payload FROM backend_agent_observation_receipts "
            "WHERE backend_id = ? AND observation_domain = 'channels' "
            "AND agent_id = ? AND agent_instance_id = ? AND backend_generation = ? "
            "AND snapshot_generation = ? AND producer_sequence = ? "
            "AND outcome = 'accepted' ORDER BY receipt_id DESC LIMIT 1",
            (
                backend_id,
                expected_agent_id,
                cursor_row[1],
                cursor_row[2],
                cursor_row[3],
                cursor_row[4],
            ),
        ).fetchone()
        if receipt_row is None or receipt_row[0] != "completeSnapshot":
            raise AcceptanceError("accepted_channel_snapshot_receipt_missing")
        fact_rows = connection.execute(
            "SELECT channel_id, channel_number, name, provider, group_name, "
            "radio, encrypted, enabled FROM backend_agent_channel_facts "
            "WHERE backend_id = ? ORDER BY channel_id",
            (backend_id,),
        ).fetchall()
        if not fact_rows:
            raise AcceptanceError("channel_facts_missing")
        facts = [fact_from_row(tuple(row)) for row in fact_rows]
        if canonical_snapshot(facts) != str(receipt_row[4]):
            raise AcceptanceError("channel_fact_receipt_mismatch")
        if (
            int(receipt_row[1]) != int(cursor_row[7])
            or str(receipt_row[2]) != str(cursor_row[5])
            or str(receipt_row[3]) != str(cursor_row[6])
        ):
            raise AcceptanceError("channel_cursor_receipt_mismatch")
        agent = {
            "agentInstanceId": str(agent_row[1]),
            "backendGeneration": int(agent_row[2]),
            "heartbeatSequence": int(agent_row[3]),
        }
        cursor = {
            "snapshotGeneration": int(cursor_row[3]),
            "producerSequence": int(cursor_row[4]),
            "resourceRevision": str(cursor_row[5]),
            "capturedAt": int(cursor_row[7]),
        }
        return agent, cursor, facts
    finally:
        connection.close()


def cursor_and_fact_identity(database_path: Path, backend_id: str) -> tuple[object, ...]:
    connection = open_read_only_database(database_path)
    try:
        cursor = connection.execute(
            "SELECT agent_id, agent_instance_id, backend_generation, "
            "snapshot_generation, producer_sequence, resource_revision, "
            "payload_identity, captured_at FROM backend_agent_observation_cursors "
            "WHERE backend_id = ? AND observation_domain = 'channels'",
            (backend_id,),
        ).fetchone()
        facts = connection.execute(
            "SELECT channel_id, channel_number, name, provider, group_name, "
            "radio, encrypted, enabled, agent_id, agent_instance_id, "
            "backend_generation, snapshot_generation, producer_sequence, "
            "captured_at, resource_revision FROM backend_agent_channel_facts "
            "WHERE backend_id = ? ORDER BY channel_id",
            (backend_id,),
        ).fetchall()
        if cursor is None or not facts:
            raise AcceptanceError("channel_state_missing")
        return tuple(cursor) + tuple(tuple(row) for row in facts)
    finally:
        connection.close()


def post_json(
    hostname: str,
    port: int,
    path: str,
    ca_certificate: str,
    agent_id: str,
    credential_secret: str,
    payload: dict[str, object],
) -> tuple[int, dict[str, object]]:
    context = ssl.create_default_context(cafile=ca_certificate or None)
    connection = http.client.HTTPSConnection(hostname, port, context=context, timeout=15)
    authorization = base64.b64encode(
        f"{agent_id}:{credential_secret}".encode("utf-8")
    ).decode("ascii")
    body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    try:
        connection.request(
            "POST",
            path,
            body=body,
            headers={
                "Authorization": "Basic " + authorization,
                "Content-Type": "application/json",
                "Accept": "application/json",
                "User-Agent": "vdr-suite-phase63-channel-acceptance/1",
            },
        )
        response = connection.getresponse()
        raw = response.read(MAX_RESPONSE_BYTES + 1)
        if len(raw) > MAX_RESPONSE_BYTES:
            raise AcceptanceError("channel_observation_response_too_large")
        try:
            decoded = json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise AcceptanceError("channel_observation_response_invalid") from error
        if not isinstance(decoded, dict):
            raise AcceptanceError("channel_observation_response_invalid")
        return response.status, decoded
    finally:
        connection.close()


def exercise(args: argparse.Namespace) -> None:
    identity = parse_protected_identity(args.identity)
    hostname, port, request_path = parse_control_plane_url(args.control_plane_url)
    agent, cursor, facts = current_protocol_state(
        args.database, args.backend, identity["agent_id"]
    )
    before = cursor_and_fact_identity(args.database, args.backend)
    common = {
        "protocolVersion": "vdr-suite-agent/1",
        "backendId": args.backend,
        "agentInstanceId": agent["agentInstanceId"],
        "backendGeneration": agent["backendGeneration"],
        "observationDomain": "channels",
        "snapshotGeneration": cursor["snapshotGeneration"],
        "observedHeartbeatSequence": agent["heartbeatSequence"],
    }
    replay_payload = {
        **common,
        "producerSequence": cursor["producerSequence"],
        "kind": "completeSnapshot",
        "capturedAt": cursor["capturedAt"],
        "resourceRevision": cursor["resourceRevision"],
        "payload": {"channels": facts},
    }
    replay_status, replay = post_json(
        hostname,
        port,
        request_path,
        args.ca_certificate_path,
        identity["agent_id"],
        identity["credential_secret"],
        replay_payload,
    )
    if (
        replay_status != 200
        or replay.get("outcome") != "replayed"
        or replay.get("snapshotGeneration") != cursor["snapshotGeneration"]
        or replay.get("producerSequence") != cursor["producerSequence"]
    ):
        raise AcceptanceError("equivalent_channel_replay_not_acknowledged")
    if cursor_and_fact_identity(args.database, args.backend) != before:
        raise AcceptanceError("equivalent_channel_replay_changed_state")

    gap_payload = {
        **common,
        "producerSequence": int(cursor["producerSequence"]) + 2,
        "kind": "changeBatch",
        "capturedAt": int(time.time()),
        "resourceRevision": "channel-gap-" + str(agent["heartbeatSequence"]),
        "payload": {"upserts": [facts[0]], "removedChannelIds": []},
    }
    gap_status, gap = post_json(
        hostname,
        port,
        request_path,
        args.ca_certificate_path,
        identity["agent_id"],
        identity["credential_secret"],
        gap_payload,
    )
    error = gap.get("error")
    error_code = error.get("code") if isinstance(error, dict) else None
    if gap_status != 409 or error_code != "observation_resync_required":
        raise AcceptanceError("channel_sequence_gap_not_rejected_with_resync")
    if cursor_and_fact_identity(args.database, args.backend) != before:
        raise AcceptanceError("channel_sequence_gap_changed_state")

    print("CHANNEL_OBSERVATION_REPLAY=PASS")
    print("CHANNEL_OBSERVATION_GAP_RESYNC=PASS")
    print("CHANNEL_OBSERVATION_FACTS_UNCHANGED=PASS")


def self_test() -> None:
    assert parse_control_plane_url("https://example.test/vdr-suite") == (
        "example.test",
        443,
        "/vdr-suite" + ROUTE,
    )
    for value in (
        "http://example.test/vdr-suite",
        "https://user@example.test/vdr-suite",
        "https://example.test/vdr-suite/",
        "https://example.test/vdr-suite?secret=1",
    ):
        try:
            parse_control_plane_url(value)
        except AcceptanceError:
            pass
        else:
            raise AssertionError(value)
    facts = [
        {
            "channelId": "S19.2E-1-1051-10301",
            "channelNumber": 1,
            "name": "Example",
            "provider": "Provider",
            "groupName": "TV",
            "radio": False,
            "encrypted": False,
            "enabled": True,
        }
    ]
    assert canonical_snapshot(facts).startswith('{"channels":[{"channelId":')
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        path = root / "identity"
        path.write_text(
            "version=3\nagent_id=agt_test\ncredential_secret=" + "x" * 32 + "\n",
            encoding="utf-8",
        )
        os.chmod(path, 0o600)
        assert parse_protected_identity(path)["agent_id"] == "agt_test"
        database = root / "state.db"
        connection = sqlite3.connect(database)
        connection.executescript(
            """
            CREATE TABLE backend_agents (
                agent_id TEXT, agent_instance_id TEXT, backend_id TEXT,
                backend_generation INTEGER, heartbeat_sequence INTEGER,
                lease_expires_at INTEGER, revoked_at INTEGER, updated_at INTEGER
            );
            CREATE TABLE backend_agent_observation_cursors (
                backend_id TEXT, observation_domain TEXT, agent_id TEXT,
                agent_instance_id TEXT, backend_generation INTEGER,
                snapshot_generation INTEGER, producer_sequence INTEGER,
                resource_revision TEXT, payload_identity TEXT, captured_at INTEGER
            );
            CREATE TABLE backend_agent_observation_receipts (
                receipt_id INTEGER PRIMARY KEY, backend_id TEXT,
                observation_domain TEXT, agent_id TEXT, agent_instance_id TEXT,
                backend_generation INTEGER, snapshot_generation INTEGER,
                producer_sequence INTEGER, kind TEXT, captured_at INTEGER,
                resource_revision TEXT, payload_identity TEXT, canonical_payload TEXT,
                outcome TEXT
            );
            CREATE TABLE backend_agent_channel_facts (
                backend_id TEXT, channel_id TEXT, channel_number INTEGER, name TEXT,
                provider TEXT, group_name TEXT, radio INTEGER, encrypted INTEGER,
                enabled INTEGER, agent_id TEXT, agent_instance_id TEXT,
                backend_generation INTEGER, snapshot_generation INTEGER,
                producer_sequence INTEGER, captured_at INTEGER, resource_revision TEXT
            );
            """
        )
        canonical = canonical_snapshot(facts)
        now = int(time.time())
        connection.execute(
            "INSERT INTO backend_agents VALUES (?,?,?,?,?,?,?,?)",
            ("agt_test", "agi_test", "default", 4, 9, now + 60, 0, now),
        )
        connection.execute(
            "INSERT INTO backend_agent_observation_cursors VALUES (?,?,?,?,?,?,?,?,?,?)",
            ("default", "channels", "agt_test", "agi_test", 4, 7, 1,
             "fnv1a64-test", "payload-test", now),
        )
        connection.execute(
            "INSERT INTO backend_agent_observation_receipts VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            (1, "default", "channels", "agt_test", "agi_test", 4, 7, 1,
             "completeSnapshot", now, "fnv1a64-test", "payload-test", canonical,
             "accepted"),
        )
        fact = facts[0]
        connection.execute(
            "INSERT INTO backend_agent_channel_facts VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
            ("default", fact["channelId"], fact["channelNumber"], fact["name"],
             fact["provider"], fact["groupName"], int(fact["radio"]),
             int(fact["encrypted"]), int(fact["enabled"]), "agt_test",
             "agi_test", 4, 7, 1, now, "fnv1a64-test"),
        )
        connection.commit()
        connection.close()
        agent, cursor, loaded = current_protocol_state(database, "default", "agt_test")
        assert agent["heartbeatSequence"] == 9
        assert cursor["snapshotGeneration"] == 7
        assert loaded == facts
        assert cursor_and_fact_identity(database, "default")
    print("Phase-63 Channel acceptance helper self-test passed")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--database", type=Path)
    parser.add_argument("--identity", type=Path)
    parser.add_argument("--backend", default="default")
    parser.add_argument("--control-plane-url", default="")
    parser.add_argument("--ca-certificate-path", default="")
    args = parser.parse_args()
    if not args.self_test and (
        args.database is None
        or args.identity is None
        or not args.control_plane_url
        or not args.backend
    ):
        parser.error("database, identity, backend and control-plane-url are required")
    return args


def main() -> int:
    args = parse_arguments()
    try:
        if args.self_test:
            self_test()
        else:
            exercise(args)
    except (AcceptanceError, OSError, sqlite3.Error, ssl.SSLError) as error:
        print(f"channel acceptance helper failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
