#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
ADR_DIR = ROOT / "docs" / "adr"
INDEX = ADR_DIR / "index.md"
ACTIVE_START = "## Active Canonical ADRs"
ACTIVE_END = "---"
EXPECTED_LATEST = "ADR-0037"
EXPECTED_NEXT = "ADR-0038"
EXPECTED_ACTIVE_0037 = "ADR-0037-packaging-install-api-boundary.md"
CONFLICT_0037 = "ADR-0037-suite-metadata-database-and-external-scraper-strategy.md"

ADR_LINK = re.compile(r"\((ADR-\d{4}[^)]+\.md)\)")
ADR_NUMBER = re.compile(r"ADR-(\d{4})")


def section(text, start_marker):
    start = text.find(start_marker)
    if start < 0:
        return ""
    rest = text[start + len(start_marker):]
    end = rest.find(ACTIVE_END)
    if end >= 0:
        return rest[:end]
    return rest


def main():
    errors = []

    if not INDEX.exists():
        print("ADR index check failed:")
        print("- docs/adr/index.md is missing")
        return 1

    text = INDEX.read_text(encoding="utf-8")
    active = section(text, ACTIVE_START)

    if EXPECTED_LATEST not in text:
        errors.append("ADR index misses canonical latest marker " + EXPECTED_LATEST)
    if EXPECTED_NEXT not in text:
        errors.append("ADR index misses next canonical ADR marker " + EXPECTED_NEXT)
    if EXPECTED_ACTIVE_0037 not in active:
        errors.append("active ADR section misses canonical ADR-0037 packaging file")
    if CONFLICT_0037 in active:
        errors.append("conflicting ADR-0037 suite metadata file is listed as active")
    if "Numbering Conflict Retained for Cleanup" not in text:
        errors.append("ADR index misses conflict cleanup section")
    if CONFLICT_0037 not in text:
        errors.append("ADR index does not keep conflicting ADR-0037 visible for cleanup")

    active_links = ADR_LINK.findall(active)
    active_numbers = []
    for link in active_links:
        match = ADR_NUMBER.search(link)
        if match:
            active_numbers.append(match.group(1))
        if not (ADR_DIR / link).exists():
            errors.append("active ADR link target is missing: " + link)

    duplicates = sorted({number for number in active_numbers if active_numbers.count(number) > 1})
    for number in duplicates:
        errors.append("duplicate active ADR number: ADR-" + number)

    if "0037" not in active_numbers:
        errors.append("active ADR list does not contain ADR-0037")
    if "0038" in active_numbers:
        errors.append("ADR-0038 is listed active although it is currently the next available number")

    lowercase_or_numeric = []
    for line in active.splitlines():
        if "(adr-" in line or re.search(r"\(\d{3}-", line):
            lowercase_or_numeric.append(line.strip())
    for line in lowercase_or_numeric:
        errors.append("legacy ADR file listed as active: " + line)

    if errors:
        print("ADR index check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("ADR index check passed.")
    print("Active canonical ADR count: " + str(len(active_numbers)))
    print("Latest canonical ADR: " + EXPECTED_LATEST)
    print("Next canonical ADR: " + EXPECTED_NEXT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
