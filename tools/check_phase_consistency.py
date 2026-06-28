#!/usr/bin/env python3
import argparse
import re
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "README": ROOT / "README.md",
    "current-status": ROOT / "docs" / "development" / "current-status.md",
    "project-status-dashboard": ROOT / "docs" / "project-status-dashboard.md",
    "roadmap": ROOT / "docs" / "planning" / "roadmap.md",
    # completed-phases.md is a long historical archive and may be compacted.
    # The explicit latest marker is tracked in completed-phases-latest.md instead.
    "completed-phases-latest": ROOT / "docs" / "development" / "completed-phases-latest.md",
    "development-index": ROOT / "docs" / "development" / "index.md",
}

PHASE = re.compile(r"Phase\s+(\d+(?:\.\d+)?[a-z]?)", re.I)
PHASE_TEXT = re.compile(r"Phase\s+\d+(?:\.\d+)?[a-z]?(?:\s*-\s*[^\n\r|]+)?", re.I)
PHASE_HEADING = re.compile(r"^##\s+Phase\s+(\d+(?:\.\d+)?[a-z]?)\b", re.I)


@dataclass(frozen=True)
class PhaseFinding:
    value: str
    text: str
    line_number: int
    marker: str
    line_text: str


def key(value):
    match = re.match(r"(\d+)(?:\.(\d+))?([a-z]?)", value, re.I)
    if not match:
        return (0, 0, value)
    return (int(match.group(1)), int(match.group(2) or 0), match.group(3).lower())


def label(value):
    return "Phase " + value if value else "no phase found"


def relative(path):
    return path.relative_to(ROOT).as_posix()


def finding_from_line(line, line_number, marker):
    text_match = PHASE_TEXT.search(line)
    if not text_match:
        return None

    value_match = PHASE.search(text_match.group(0))
    if not value_match:
        return None

    return PhaseFinding(
        value=value_match.group(1),
        text=text_match.group(0).strip(),
        line_number=line_number,
        marker=marker,
        line_text=line.strip(),
    )


def latest(text):
    findings = []

    for line_number, line in enumerate(text.splitlines(), start=1):
        finding = finding_from_line(line, line_number, "any phase")
        if finding:
            findings.append(finding)

    return sorted(findings, key=lambda finding: key(finding.value))[-1] if findings else None


def latest_completed_heading(text):
    findings = []

    for line_number, line in enumerate(text.splitlines(), start=1):
        if not PHASE_HEADING.search(line):
            continue

        finding = finding_from_line(line, line_number, "latest completed ## Phase heading")
        if finding:
            findings.append(finding)

    return sorted(findings, key=lambda finding: key(finding.value))[-1] if findings else None


def phase_after(text, marker):
    lines = text.splitlines()

    for index, line in enumerate(lines):
        if marker.lower() not in line.lower():
            continue

        finding = finding_from_line(line, index + 1, marker)
        if finding:
            return finding

        for offset, candidate in enumerate(lines[index + 1:index + 10], start=index + 2):
            finding = finding_from_line(candidate, offset, marker)
            if finding:
                return finding

    return None


def first_phase_after_any(text, markers):
    for marker in markers:
        finding = phase_after(text, marker)
        if finding:
            return finding
    return None


def explicit_latest_completed_marker(text):
    return first_phase_after_any(text, [
        "Latest completed implementation phase",
        "Latest Completed Implementation Phase",
    ])


def completed_phases_marker(text):
    explicit = explicit_latest_completed_marker(text)
    if explicit:
        return explicit

    return latest_completed_heading(text)


def completed_phase(name, text):
    if name == "README":
        return first_phase_after_any(text, [
            "Latest completed implementation phase",
            "Latest Completed Implementation Phase",
        ])
    if name == "current-status":
        return first_phase_after_any(text, [
            "Latest completed implementation phase",
        ])
    if name == "project-status-dashboard":
        return first_phase_after_any(text, [
            "Current Major Phase",
            "Latest completed implementation phase",
            "Latest Completed Milestone",
            "Latest Completed Milestones",
        ])
    if name == "roadmap":
        return first_phase_after_any(text, [
            "Completed implementation state",
        ])
    if name == "development-index":
        return first_phase_after_any(text, [
            "Current completed phase",
        ])
    if name == "completed-phases-latest":
        return explicit_latest_completed_marker(text)
    if name == "completed-phases":
        return completed_phases_marker(text)
    return latest(text)


def next_phase(name, text):
    if name == "README":
        return first_phase_after_any(text, [
            "Next major implementation milestone",
            "Next Major Implementation Milestone",
            "Current Implementation Focus",
        ])
    if name == "current-status":
        return first_phase_after_any(text, [
            "Next Technical Focus",
            "Next major implementation milestone",
        ])
    if name == "project-status-dashboard":
        return first_phase_after_any(text, [
            "Next Major Implementation Milestone",
            "Current Focus",
        ])
    if name == "roadmap":
        return first_phase_after_any(text, [
            "Next major implementation milestone",
            "Next implementation step",
        ])
    if name == "development-index":
        return first_phase_after_any(text, [
            "Next implementation focus",
        ])
    return None


def read_files():
    texts = {}
    for name, path in FILES.items():
        if not path.exists():
            raise FileNotFoundError(str(path))
        texts[name] = path.read_text(encoding="utf-8")
    return texts


def newest_value(findings):
    values = [finding.value for finding in findings.values() if finding]
    return max(values, key=key, default=None)


def describe(name, finding):
    path = relative(FILES[name])

    if not finding:
        return name + ": no phase found in " + path

    return (
        name
        + ": "
        + finding.text
        + " at "
        + path
        + ":"
        + str(finding.line_number)
        + " via '"
        + finding.marker
        + "'"
    )


def print_report(completed, planned, newest, expected_next):
    print("Phase consistency report:")
    print("Expected completed phase: " + label(newest))
    if expected_next:
        print("Expected next implementation focus: " + label(expected_next))

    print("")
    print("Completed phase markers:")
    for name in sorted(completed):
        print("- " + describe(name, completed[name]))

    print("")
    print("Next phase markers:")
    for name in sorted(planned):
        if planned[name]:
            print("- " + describe(name, planned[name]))
        else:
            print("- " + name + ": no next phase marker")


def collect_errors(completed, planned, newest, expected_next):
    errors = []

    if newest is None:
        errors.append(("documentation", "completed", None, newest))
    else:
        for name, finding in completed.items():
            if not finding or finding.value != newest:
                errors.append((name, "completed", finding, newest))

    if expected_next is not None:
        for name, finding in planned.items():
            if finding and finding.value != expected_next:
                errors.append((name, "next", finding, expected_next))

    return errors


def print_errors(errors):
    print("Phase consistency check failed:")

    for name, kind, finding, expected in errors:
        if name == "documentation" and finding is None:
            print("- No completed phase found in documentation")
            continue

        path = relative(FILES[name])
        actual = label(finding.value) if finding else "no phase found"
        message = (
            "- "
            + name
            + " "
            + kind
            + " phase differs: "
            + actual
            + " != "
            + label(expected)
            + " in "
            + path
        )

        if finding:
            message += ":" + str(finding.line_number) + " via '" + finding.marker + "'"

        print(message)

        if finding:
            print("  line: " + finding.line_text)

    print("")
    print("Run with --report to show all detected phase markers.")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Check that high-level documentation phase markers stay consistent."
    )
    parser.add_argument(
        "--report",
        action="store_true",
        help="Print all detected phase markers and their source lines.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    texts = read_files()
    completed = {name: completed_phase(name, text) for name, text in texts.items()}
    planned = {name: next_phase(name, text) for name, text in texts.items()}

    newest = newest_value(completed)
    expected_next = newest_value(planned)
    errors = collect_errors(completed, planned, newest, expected_next)

    if args.report:
        print_report(completed, planned, newest, expected_next)
        print("")

    if errors:
        print_errors(errors)
        return 1

    print("Phase consistency check passed.")
    print("Latest completed phase: " + label(newest))
    for name in sorted(completed):
        print("- " + describe(name, completed[name]))

    if expected_next:
        print("Next implementation focus: " + label(expected_next))
        for name in sorted(planned):
            if planned[name]:
                print("- " + describe(name, planned[name]))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
