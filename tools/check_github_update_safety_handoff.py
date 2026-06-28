#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CURRENT_STATUS = ROOT / "docs/development/current-status.md"

REQUIRED = [
    "### Preferred edit path for new chats",
    "Prefer direct GitHub repository updates for existing files",
    "Use local edits first only when the change requires:",
    "a workaround because the GitHub connector blocks a file operation",
    "Never replace a complete existing file through GitHub update_file from a truncated or partial fetch.",
    "For index files and status files, a small link addition must remain a small diff.",
]


def main() -> int:
    if not CURRENT_STATUS.exists():
        print("GitHub update safety handoff check failed:")
        print("- docs/development/current-status.md is missing")
        return 1

    text = CURRENT_STATUS.read_text(encoding="utf-8")
    missing = [item for item in REQUIRED if item not in text]

    if missing:
        print("GitHub update safety handoff check failed:")
        for item in missing:
            print("- missing rule: " + item)
        return 1

    print("GitHub update safety handoff check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
