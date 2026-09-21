#!/usr/bin/env python3
"""Report phase and milestone markers in documentation files.

This helper is intentionally read-only. It scans the docs tree for phase,
milestone and status wording so stale roadmap/status documents can be found
before manual documentation updates.
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

MARKER_PATTERN = re.compile(
    r"(Phase\s+\d+(?:\.\d+)?[a-z]?|"
    r"Milestone|"
    r"Current\s+(?:Major\s+)?Phase|"
    r"Current\s+Focus|"
    r"Latest\s+Completed\s+Milestone|"
    r"Next\s+implementation\s+step|"
    r"Completed\s+implementation\s+state|"
    r"Roadmap\s+Progress|"
    r"Complete\b|"
    r"planned\b)",
    re.IGNORECASE,
)

PHASE_PATTERN = re.compile(r"Phase\s+(\d+(?:\.\d+)?[a-z]?)", re.IGNORECASE)


def phase_sort_key(value: str) -> tuple[int, float, str]:
    match = re.match(r"(\d+)(?:\.(\d+))?([a-z]?)", value, re.IGNORECASE)
    if not match:
        return (0, 0.0, value)

    major = int(match.group(1))
    minor = float(match.group(2) or 0)
    suffix = match.group(3) or ""
    return (major, minor, suffix)


def main() -> int:
    if not DOCS.exists():
        print("docs directory not found")
        return 1

    phase_values: set[str] = set()
    files_with_markers = 0

    for path in sorted(DOCS.rglob("*.md")):
        rel = path.relative_to(ROOT)
        matches: list[tuple[int, str]] = []

        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if MARKER_PATTERN.search(line):
                matches.append((number, line.rstrip()))

            for phase_match in PHASE_PATTERN.finditer(line):
                phase_values.add(phase_match.group(1))

        if not matches:
            continue

        files_with_markers += 1
        print(f"\n## {rel}")
        for number, line in matches:
            print(f"{number}: {line}")

    print("\n## Summary")
    print(f"Files with markers: {files_with_markers}")

    if phase_values:
        ordered = sorted(phase_values, key=phase_sort_key)
        print("Detected phases:")
        for phase in ordered:
            print(f"- Phase {phase}")
        print(f"Latest detected phase: Phase {ordered[-1]}")
    else:
        print("Detected phases: none")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
