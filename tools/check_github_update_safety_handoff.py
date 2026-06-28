#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CURRENT_STATUS = ROOT / "docs/development/current-status.md"

# GitHub update safety rules for future assistants:
#
# - Never replace a complete existing file through GitHub update_file from a
#   truncated or partial fetch.
# - For index files and status files, a small link addition must remain a
#   small diff.
# - If a GitHub fetch result is truncated, fetch the missing ranges before
#   updating the file or use a smaller safe edit strategy.
# - After every GitHub file update, inspect the commit diff before treating
#   the change as correct.

REQUIRED_CURRENT_STATUS_RULES = [
    "### Preferred edit path for new chats",
    "Prefer direct GitHub repository updates for existing files",
    "Use local edits first only when the change requires:",
    "a workaround because the GitHub connector blocks a file operation",
]

REQUIRED_GUARDRAIL_RULES = [
    "Never replace a complete existing file through GitHub update_file from a",
    "truncated or partial fetch.",
    "For index files and status files, a small link addition must remain a",
    "small diff.",
    "If a GitHub fetch result is truncated, fetch the missing ranges before",
    "After every GitHub file update, inspect the commit diff",
]


def main() -> int:
    if not CURRENT_STATUS.exists():
        print("GitHub update safety handoff check failed:")
        print("- docs/development/current-status.md is missing")
        return 1

    current_status_text = CURRENT_STATUS.read_text(encoding="utf-8")
    own_text = Path(__file__).read_text(encoding="utf-8")

    missing = []

    for item in REQUIRED_CURRENT_STATUS_RULES:
        if item not in current_status_text:
            missing.append("current-status.md missing rule: " + item)

    for item in REQUIRED_GUARDRAIL_RULES:
        if item not in own_text:
            missing.append("guardrail missing anti-truncation rule: " + item)

    if missing:
        print("GitHub update safety handoff check failed:")
        for item in missing:
            print("- " + item)
        return 1

    print("GitHub update safety handoff check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
