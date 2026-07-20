#!/usr/bin/env python3
from pathlib import Path
import sys

UNIT_PATH = Path("packaging/systemd/vdr-suite-daemon.service")
DEFAULTS_PATH = Path("packaging/systemd/vdr-suite-daemon.default")

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
    "VDR_SUITE_SUITE_BRIDGE_ENABLED=false",
    "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID=default",
    "VDR_SUITE_SUITE_BRIDGE_HOST=127.0.0.1",
    "VDR_SUITE_SUITE_BRIDGE_PORT=6419",
]

FORBIDDEN_PREFIXES = [
    "Requires=vdr.service",
]


def main() -> int:
    failed = False

    if not UNIT_PATH.exists():
        print(f"missing systemd unit: {UNIT_PATH}", file=sys.stderr)
        failed = True

    if not DEFAULTS_PATH.exists():
        print(f"missing systemd defaults: {DEFAULTS_PATH}", file=sys.stderr)
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

    defaults = DEFAULTS_PATH.read_text(encoding="utf-8").splitlines()
    defaults_set = set(defaults)

    for required in REQUIRED_DEFAULTS:
        if required not in defaults_set:
            print(f"missing required daemon default: {required}", file=sys.stderr)
            failed = True

    if "VDR_SUITE_SUITE_BRIDGE_ENABLED=true" in defaults_set:
        print("Suite Bridge must remain disabled in packaged defaults", file=sys.stderr)
        failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
