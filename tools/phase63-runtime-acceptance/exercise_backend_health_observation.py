#!/usr/bin/env python3
"""Exercise replay and sequence-gap handling without exposing Agent secrets."""

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
ROUTE = "/api/agent/v1/observations/backend-health"


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
    if values.get("version") not in {"1", "2"}:
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
    base_path = parsed.path.rstrip("/")
    return parsed.hostname, parsed.port or 443, base_path + ROUTE


def open_read_only_database(path: Path) -> sqlite3.Connection:
    return sqlite3.connect(f"file:{path}?mode=ro", uri=True, timeout=10)


def current_protocol_state(
    database_path: Path,
    backend_id: str,
    expected_agent_id: str,
) -> tuple[dict[str, object], dict[str, object]]:
    connection = open_read_only_database(database_path)
    try:
        agent_row = connection.execute(
            "SELECT agent_id, agent_instance_id, backend_generation, "
            "heartbeat_sequence, lease_expires_at "
            "FROM backend_agents WHERE backend_id = ? AND revoked_at = 0 "
            "ORDER BY updated_at DESC LIMIT 1",
            (backend_id,),
        ).fetchone()
        cursor_row = connection.execute(
            "SELECT agent_id, agent_instance_id, backend_generation, "
            "snapshot_generation, producer_sequence, resource_revision, "
            "captured_at FROM backend_agent_observation_cursors "
            "WHERE backend_id = ? AND observation_domain = 'backend-health'",
            (backend_id,),
        ).fetchone()
        if agent_row is None or cursor_row is None:
            raise AcceptanceError("observation_state_missing")
        if agent_row[0] != expected_agent_id or cursor_row[0] != expected_agent_id:
            raise AcceptanceError("observation_agent_mismatch")
        if tuple(agent_row[1:3]) != tuple(cursor_row[1:3]):
            raise AcceptanceError("observation_lineage_mismatch")
        if agent_row[4] <= int(time.time()):
            raise AcceptanceError("agent_lease_expired")
        receipt_row = connection.execute(
            "SELECT kind, captured_at, resource_revision FROM "
            "backend_agent_observation_receipts WHERE backend_id = ? "
            "AND observation_domain = 'backend-health' AND agent_id = ? "
            "AND agent_instance_id = ? AND backend_generation = ? "
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
        if receipt_row is None:
            raise AcceptanceError("accepted_observation_receipt_missing")
        agent = {
            "agentId": agent_row[0],
            "agentInstanceId": agent_row[1],
            "backendGeneration": int(agent_row[2]),
            "heartbeatSequence": int(agent_row[3]),
        }
        cursor = {
            "snapshotGeneration": int(cursor_row[3]),
            "producerSequence": int(cursor_row[4]),
            "kind": receipt_row[0],
            "capturedAt": int(receipt_row[1]),
            "resourceRevision": receipt_row[2],
        }
        return agent, cursor
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
    body = json.dumps(payload, separators=(",", ":"), sort_keys=True).encode("utf-8")
    try:
        connection.request(
            "POST",
            path,
            body=body,
            headers={
                "Authorization": "Basic " + authorization,
                "Content-Type": "application/json",
                "Accept": "application/json",
                "User-Agent": "vdr-suite-phase63-runtime-acceptance/1",
            },
        )
        response = connection.getresponse()
        raw = response.read(MAX_RESPONSE_BYTES + 1)
        if len(raw) > MAX_RESPONSE_BYTES:
            raise AcceptanceError("observation_response_too_large")
        try:
            decoded = json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise AcceptanceError("observation_response_invalid") from error
        if not isinstance(decoded, dict):
            raise AcceptanceError("observation_response_invalid")
        return response.status, decoded
    finally:
        connection.close()


def cursor_identity(database_path: Path, backend_id: str) -> tuple[object, ...]:
    connection = open_read_only_database(database_path)
    try:
        row = connection.execute(
            "SELECT agent_id, agent_instance_id, backend_generation, "
            "snapshot_generation, producer_sequence, resource_revision, captured_at "
            "FROM backend_agent_observation_cursors WHERE backend_id = ? "
            "AND observation_domain = 'backend-health'",
            (backend_id,),
        ).fetchone()
        if row is None:
            raise AcceptanceError("observation_cursor_missing")
        return tuple(row)
    finally:
        connection.close()


def exercise(args: argparse.Namespace) -> None:
    identity = parse_protected_identity(args.identity)
    hostname, port, request_path = parse_control_plane_url(args.control_plane_url)
    agent, cursor = current_protocol_state(
        args.database, args.backend, identity["agent_id"]
    )
    before = cursor_identity(args.database, args.backend)
    common = {
        "protocolVersion": "vdr-suite-agent/1",
        "backendId": args.backend,
        "agentInstanceId": agent["agentInstanceId"],
        "backendGeneration": agent["backendGeneration"],
        "observationDomain": "backend-health",
        "snapshotGeneration": cursor["snapshotGeneration"],
        "agentState": "online",
        "observedHeartbeatSequence": agent["heartbeatSequence"],
    }
    replay_payload = {
        **common,
        "producerSequence": cursor["producerSequence"],
        "kind": cursor["kind"],
        "capturedAt": cursor["capturedAt"],
        "resourceRevision": cursor["resourceRevision"],
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
        raise AcceptanceError("equivalent_replay_not_acknowledged")
    if cursor_identity(args.database, args.backend) != before:
        raise AcceptanceError("equivalent_replay_advanced_cursor")

    gap_payload = {
        **common,
        "producerSequence": int(cursor["producerSequence"]) + 2,
        "kind": "changeBatch",
        "capturedAt": int(time.time()),
        "resourceRevision": "heartbeat-" + str(agent["heartbeatSequence"]) + "-gap",
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
        raise AcceptanceError("sequence_gap_not_rejected_with_resync")
    if cursor_identity(args.database, args.backend) != before:
        raise AcceptanceError("sequence_gap_advanced_cursor")

    print("BACKEND_HEALTH_OBSERVATION_REPLAY=PASS")
    print("BACKEND_HEALTH_OBSERVATION_GAP_RESYNC=PASS")


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
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "identity"
        path.write_text(
            "version=2\nagent_id=agt_test\ncredential_secret=" + "x" * 32 + "\n",
            encoding="utf-8",
        )
        os.chmod(path, 0o600)
        values = parse_protected_identity(path)
        assert values["agent_id"] == "agt_test"
    print("Phase-63 backend-health acceptance helper self-test passed")


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
        print(f"backend-health acceptance helper failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
