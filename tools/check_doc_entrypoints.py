#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_LINKS = {
    "README.md": [
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/planning/roadmap.md",
        "docs/adr/index.md",
    ],
    "docs/index.md": [
        "CURRENT.md",
        "NEW-CHAT-HANDOFF.md",
        "planning/roadmap.md",
        "planning/parity-audit-and-frontend-gap-roadmap.md",
    ],
    "docs/CURRENT.md": [
        "NEW-CHAT-HANDOFF.md",
        "planning/roadmap.md",
        "planning/phase-map.md",
        "planning/golden-user-journeys.md",
        "adr/index.md",
        "development/current-status.md",
    ],
    "docs/NEW-CHAT-HANDOFF.md": [
        "CURRENT.md",
        "planning/roadmap.md",
        "planning/golden-user-journeys.md",
        "adr/index.md",
        "development/completed-phases.md",
    ],
    "docs/development/github-actions-status-handoff.md": [
        "../NEW-CHAT-HANDOFF.md",
    ],
    "docs/planning/roadmap.md": [
        "phase-map.md",
        "golden-user-journeys.md",
        "../CURRENT.md",
    ],
    "docs/planning/phase-map.md": [
        "../CURRENT.md",
        "roadmap.md",
        "golden-user-journeys.md",
    ],
    "docs/planning/index.md": [
        "roadmap.md",
        "phase-map.md",
        "parity-audit-and-frontend-gap-roadmap.md",
        "../CURRENT.md",
        "../NEW-CHAT-HANDOFF.md",
    ],
}

REQUIRED_TEXT = {
    "README.md": [
        "sole repository authority for volatile operational status",
        "A chat discussion is not a binding VDR-Suite project decision",
    ],
    "docs/CURRENT.md": [
        "## Operational status authority",
        "Phase 63 - Backend Agent and Secure Multi-Site Runtime",
        "Phase 64 - Timer Intent and Multi-Backend Orchestration",
        "Phase 65 - Streaming Gateway and Media Sessions",
        "PR #190 - Add disabled SuiteBridge Timer delete transport",
        "No Phase-64 successor implementation is currently authorized",
    ],
    "docs/NEW-CHAT-HANDOFF.md": [
        "## Current implementation boundary",
        "no `#191` are currently authorized",
        "broad polished Timer UI",
        "Phase 65 Streaming",
    ],
    "docs/planning/roadmap.md": [
        "planning hold after the PR-#190 checkpoint; Phase 64 is not complete",
        "broad polished Timer UI",
        "Phase 65 — Streaming Gateway and Media Sessions",
    ],
}


def read(rel):
    path = ROOT / rel
    if not path.exists():
        raise FileNotFoundError(rel)
    return path.read_text(encoding="utf-8")


def main():
    errors = []

    for rel, links in REQUIRED_LINKS.items():
        try:
            text = read(rel)
        except FileNotFoundError:
            errors.append(rel + " is missing")
            continue
        for link in links:
            if link not in text:
                errors.append(rel + " misses link/text: " + link)

    for rel, markers in REQUIRED_TEXT.items():
        try:
            text = read(rel)
        except FileNotFoundError:
            errors.append(rel + " is missing")
            continue
        for marker in markers:
            if marker not in text:
                errors.append(rel + " misses marker: " + marker)

    if errors:
        print("Documentation entrypoint check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Documentation entrypoint check passed.")
    print("Volatile project status authority: docs/CURRENT.md")
    return 0


if __name__ == "__main__":
    sys.exit(main())
