#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
ADR_DIR = ROOT / "docs" / "adr"
INDEX = ADR_DIR / "index.md"
ACTIVE_START = "## Active Canonical ADRs"
ACTIVE_END = "---"
EXPECTED_LATEST = "ADR-0052"
EXPECTED_NEXT = "ADR-0053"
REMOVED_CONFLICT = "ADR-0037-suite-metadata-database-and-external-scraper-strategy.md"
EXPECTED_ACTIVE_FILES = {
    "0037": "ADR-0037-packaging-install-api-boundary.md",
    "0038": "ADR-0038-suite-metadata-database-and-external-provider-strategy.md",
    "0039": "ADR-0039-backend-agent-control-plane-boundary.md",
    "0040": "ADR-0040-backend-lifecycle-generation-lease-health.md",
    "0041": "ADR-0041-authentication-agent-trust-multi-site-transport.md",
    "0042": "ADR-0042-safe-mutation-revision-idempotency-contract.md",
    "0043": "ADR-0043-job-claim-retry-saga-execution-model.md",
    "0044": "ADR-0044-timer-intent-assignment-native-timer-model.md",
    "0045": "ADR-0045-canonical-epg-event-identity-provenance.md",
    "0046": "ADR-0046-streaming-gateway-media-session-boundary.md",
    "0047": "ADR-0047-legacy-osd-compatibility-bridge.md",
    "0048": "ADR-0048-public-api-versioning-error-compatibility-contract.md",
    "0049": "ADR-0049-audit-security-event-model.md",
    "0050": "ADR-0050-domain-repository-sqlite-boundary.md",
    "0051": "ADR-0051-manual-recording-metadata-assignment.md",
    "0052": "ADR-0052-manual-recording-cast-ingestion-search.md",
}

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
    if "Numbering Cleanup" not in text:
        errors.append("ADR index misses numbering cleanup section")
    if REMOVED_CONFLICT in active:
        errors.append("removed conflicting ADR-0037 file is listed as active")
    if (ADR_DIR / REMOVED_CONFLICT).exists():
        errors.append("removed conflicting ADR-0037 file still exists")

    active_links = ADR_LINK.findall(active)
    active_numbers = []
    for link in active_links:
        match = ADR_NUMBER.search(link)
        if match:
            active_numbers.append(match.group(1))
        if not (ADR_DIR / link).exists():
            errors.append("active ADR link target is missing: " + link)

    duplicates = sorted(
        {number for number in active_numbers if active_numbers.count(number) > 1}
    )
    for number in duplicates:
        errors.append("duplicate active ADR number: ADR-" + number)

    for number, filename in EXPECTED_ACTIVE_FILES.items():
        if number not in active_numbers:
            errors.append("active ADR list does not contain ADR-" + number)
        if filename not in active:
            errors.append("active ADR section misses canonical file " + filename)
        if not (ADR_DIR / filename).exists():
            errors.append("canonical ADR file is missing: " + filename)

    if "0053" in active_numbers:
        errors.append("ADR-0053 is listed active although it is the next available number")

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
