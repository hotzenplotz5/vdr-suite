#!/usr/bin/env python3
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "README": ROOT / "README.md",
    "current-entrypoint": ROOT / "docs" / "CURRENT.md",
}

PHASE = re.compile(r"Phase\s+(\d+(?:\.\d+)?[a-z]?)(?:\s*-\s*[^\n\r|]+)?", re.I)

COMPLETED_MARKERS = [
    "Latest completed implementation phase",
    "Current completed project block",
    "Current completed phase",
]

NEXT_MARKERS = [
    "Next major implementation milestone",
    "After that",
    "Next implementation focus",
]


def phase_key(value):
    match = re.match(r"(\d+)(?:\.(\d+))?([a-z]?)", value, re.I)
    if not match:
        return (0, 0, value)
    return (int(match.group(1)), int(match.group(2) or 0), match.group(3).lower())


def label(value):
    return "Phase " + value if value else "no phase found"


def phase_after(text, markers):
    lines = text.splitlines()
    for marker in markers:
        for index, line in enumerate(lines):
            if marker.lower() not in line.lower():
                continue
            window = lines[index:index + 10]
            for offset, candidate in enumerate(window, start=index + 1):
                match = PHASE.search(candidate)
                if match:
                    return {
                        "value": match.group(1),
                        "text": match.group(0).strip(),
                        "line": offset,
                        "marker": marker,
                    }
    return None


def describe(name, finding):
    path = FILES[name].relative_to(ROOT).as_posix()
    if not finding:
        return name + ": no phase marker in " + path
    return (
        name
        + ": "
        + finding["text"]
        + " at "
        + path
        + ":"
        + str(finding["line"])
        + " via '"
        + finding["marker"]
        + "'"
    )


def newest(findings):
    values = [finding["value"] for finding in findings.values() if finding]
    return max(values, key=phase_key, default=None)


def main():
    texts = {name: path.read_text(encoding="utf-8") for name, path in FILES.items()}
    completed = {name: phase_after(text, COMPLETED_MARKERS) for name, text in texts.items()}
    planned = {name: phase_after(text, NEXT_MARKERS) for name, text in texts.items()}

    completed_value = newest(completed)
    planned_value = newest(planned)

    errors = []
    for name, finding in completed.items():
        if not finding or finding["value"] != completed_value:
            errors.append(name + " completed phase differs")
    for name, finding in planned.items():
        if planned_value and finding and finding["value"] != planned_value:
            errors.append(name + " next phase differs")

    if errors:
        print("Phase consistency check failed:")
        for error in errors:
            print("- " + error)
        print("")
        print("Completed phase markers:")
        for name in sorted(completed):
            print("- " + describe(name, completed[name]))
        print("Next phase markers:")
        for name in sorted(planned):
            print("- " + describe(name, planned[name]))
        return 1

    print("Phase consistency check passed.")
    print("Latest completed phase: " + label(completed_value))
    for name in sorted(completed):
        print("- " + describe(name, completed[name]))
    if planned_value:
        print("Next implementation focus: " + label(planned_value))
        for name in sorted(planned):
            print("- " + describe(name, planned[name]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
