#!/usr/bin/env python3
from pathlib import Path
import sys

UNIT_PATH = Path("packaging/systemd/vdr-suite-daemon.service")
DEFAULTS_PATH = Path("packaging/systemd/vdr-suite-daemon.default")
AGENT_UNIT_PATH = Path("packaging/systemd/vdr-suite-backend-agent.service")
AGENT_CONFIG_PATH = Path("packaging/systemd/backend-agent.conf")

REQUIRED_LINES = [
    "After=network-online.target vdr.service",
    "Wants=network-online.target vdr.service",
    "EnvironmentFile=-/etc/default/vdr-suite-daemon",
    "Environment=VDR_SUITE_FRONTEND_ROOT=/usr/share/vdr-suite/web/frontend",
    "Environment=VDR_SUITE_DATABASE_PATH=/var/lib/vdr-suite/vdr-suite.db",
    "ExecStart=/usr/sbin/vdr-suite-daemon",
    "Restart=on-failure",
    "KillSignal=SIGTERM",
]

REQUIRED_DEFAULTS = [
    "VDR_SUITE_SUITE_BRIDGE_ENABLED=true",
    "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID=default",
    "VDR_SUITE_SUITE_BRIDGE_HOST=127.0.0.1",
    "VDR_SUITE_SUITE_BRIDGE_PORT=6419",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED=true",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER=tvmaze",
    "VDR_SUITE_TVMAZE_CONNECT_TIMEOUT_MS=2000",
    "VDR_SUITE_TVMAZE_TOTAL_TIMEOUT_MS=8000",
    "VDR_SUITE_TVMAZE_MAX_RETRIES=1",
]

FORBIDDEN_DEFAULTS = [
    "VDR_SUITE_SUITE_BRIDGE_ENABLED=false",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED=false",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER=none",
]

FORBIDDEN_PREFIXES = [
    "Requires=vdr.service",
]

REQUIRED_AGENT_LINES = [
    "After=network-online.target",
    "Wants=network-online.target",
    "ConditionPathExists=/etc/vdr-suite/backend-agent.conf",
    "User=vdr",
    "Group=vdr",
    "ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf",
    "Restart=on-failure",
    "NoNewPrivileges=true",
    "UMask=0077",
    "ProtectSystem=strict",
    "ProtectHome=true",
    "CapabilityBoundingSet=",
    "RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6",
    "ReadWritePaths=/var/lib/vdr-suite/backend-agent",
]

REQUIRED_AGENT_CONFIG = [
    "CONTROL_PLANE_URL=https://control-plane.example.invalid",
    "BACKEND_ID=default",
    "IDENTITY_PATH=/var/lib/vdr-suite/backend-agent/identity",
    "ENROLLMENT_PATH=/var/lib/vdr-suite/backend-agent/enrollment",
    "OBSERVATION_DOMAINS=backend-health",
]


def active_lines(path: Path) -> set[str]:
    return {
        line.strip()
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    }


def main() -> int:
    failed = False

    if not UNIT_PATH.exists():
        print(f"missing systemd unit: {UNIT_PATH}", file=sys.stderr)
        failed = True

    if not DEFAULTS_PATH.exists():
        print(f"missing systemd defaults: {DEFAULTS_PATH}", file=sys.stderr)
        failed = True

    for path, label in [
        (AGENT_UNIT_PATH, "Backend Agent systemd unit"),
        (AGENT_CONFIG_PATH, "Backend Agent configuration"),
    ]:
        if not path.exists():
            print(f"missing {label}: {path}", file=sys.stderr)
            failed = True

    if failed:
        return 1

    lines = UNIT_PATH.read_text(encoding="utf-8").splitlines()
    line_set = set(lines)

    for required in REQUIRED_LINES:
        if required not in line_set:
            print(f"missing required systemd unit line: {required}", file=sys.stderr)
            failed = True

    for line in lines:
        for forbidden in FORBIDDEN_PREFIXES:
            if line.strip() == forbidden:
                print(f"forbidden hard dependency in systemd unit: {line}", file=sys.stderr)
                failed = True

    defaults_set = active_lines(DEFAULTS_PATH)

    for required in REQUIRED_DEFAULTS:
        if required not in defaults_set:
            print(f"missing required active daemon default: {required}", file=sys.stderr)
            failed = True

    for forbidden in FORBIDDEN_DEFAULTS:
        if forbidden in defaults_set:
            print(f"forbidden obsolete active daemon default: {forbidden}", file=sys.stderr)
            failed = True

    if any(
        line.startswith("VDR_SUITE_TMDB_READ_ACCESS_TOKEN=")
        for line in defaults_set
    ):
        print("packaged defaults must not define a TMDB token", file=sys.stderr)
        failed = True

    agent_lines = AGENT_UNIT_PATH.read_text(encoding="utf-8").splitlines()
    for required in REQUIRED_AGENT_LINES:
        if required not in agent_lines:
            print(f"missing required Backend Agent unit line: {required}", file=sys.stderr)
            failed = True
    for line in agent_lines:
        normalized = line.strip().lower()
        if normalized.startswith("environment=") or normalized.startswith("environmentfile="):
            print("Backend Agent unit must not source credentials from the environment", file=sys.stderr)
            failed = True
        if "--token" in normalized or "--password" in normalized or "--secret" in normalized:
            print("Backend Agent credentials must not be process arguments", file=sys.stderr)
            failed = True

    agent_config = active_lines(AGENT_CONFIG_PATH)
    for required in REQUIRED_AGENT_CONFIG:
        if required not in agent_config:
            print(f"missing required Backend Agent configuration: {required}", file=sys.stderr)
            failed = True
    for line in agent_config:
        name = line.split("=", 1)[0].strip().lower()
        if any(secret in name for secret in [
            "token", "password", "credential_secret", "authorization", "cookie", "csrf"
        ]):
            print(f"packaged Backend Agent configuration contains secret field: {name}", file=sys.stderr)
            failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
