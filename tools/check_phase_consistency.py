#!/usr/bin/env python3
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATUS = ROOT / "docs" / "development" / "current-status.md"

EXPECTED_PHASE_MAJOR = 13
BASELINE_COMMIT = "ede298e"


def run_git(args):
    return subprocess.check_output(
        ["git"] + args,
        cwd=ROOT,
        text=True
    )


def main():
    errors = []

    status_text = STATUS.read_text(encoding="utf-8")

    if f"Phase {EXPECTED_PHASE_MAJOR}" not in status_text:
        errors.append(
            f"current-status.md does not mention expected Phase {EXPECTED_PHASE_MAJOR}"
        )

    log = run_git(["log", "--oneline", f"{BASELINE_COMMIT}..HEAD"])

    phase_commits = []
    for line in log.splitlines():
        match = re.search(r"\bPhase\s+(\d+)(?:\.\d+)?\b", line)
        if match:
            phase_commits.append((line, int(match.group(1))))

    for line, major in phase_commits:
        if major < EXPECTED_PHASE_MAJOR:
            errors.append(
                f"Commit references old phase {major}: {line}"
            )

    if errors:
        print("Phase consistency check failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        f"Phase consistency check passed for Phase {EXPECTED_PHASE_MAJOR} "
        f"after baseline {BASELINE_COMMIT}."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
