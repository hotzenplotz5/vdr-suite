#!/usr/bin/env python3
from pathlib import Path
import sys

UNIT_PATH = Path("packaging/systemd/vdr-suite-daemon.service")

REQUIRED_LINES = [
    "After=network-online.target vdr.service",
    "Wants=network-online.target vdr.service",
    "Environment=VDR_SUITE_FRONTEND_ROOT=/usr/share/vdr-suite/web/frontend",
    "Environment=VDR_SUITE_DATABASE_PATH=/var/lib/vdr-suite/vdr-suite.db",
    "ExecStart=/usr/sbin/vdr-suite-daemon",
    "Restart=on-failure",
    "KillSignal=SIGTERM",
]

FORBIDDEN_PREFIXES = [
    "Requires=vdr.service",
]


def main() -> int:
    if not UNIT_PATH.exists():
        print(f"missing systemd unit: {UNIT_PATH}", file=sys.stderr)
        return 1

    lines = UNIT_PATH.read_text(encoding="utf-8").splitlines()
    line_set = set(lines)

    failed = False

    for required in REQUIRED_LINES:
        if required not in line_set:
            print(f"missing required systemd unit line: {required}", file=sys.stderr)
            failed = True

    for line in lines:
        for forbidden in FORBIDDEN_PREFIXES:
            if line.strip() == forbidden:
                print(f"forbidden hard dependency in systemd unit: {line}", file=sys.stderr)
                failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
