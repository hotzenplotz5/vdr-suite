#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_LINKS = {
    "README.md": [
        "docs/CURRENT.md",
        "docs/index.md",
        "docs/planning/roadmap.md",
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
        "planning/parity-audit-and-frontend-gap-roadmap.md",
        "adr/index.md",
        "development/completed-phases.md",
    ],
    "docs/NEW-CHAT-HANDOFF.md": [
        "CURRENT.md",
        "planning/roadmap.md",
        "planning/phase-map.md",
        "planning/parity-audit-and-frontend-gap-roadmap.md",
        "adr/index.md",
        "development/completed-phases.md",
    ],
    "docs/development/github-actions-status-handoff.md": [
        "../NEW-CHAT-HANDOFF.md",
    ],
    "docs/planning/roadmap.md": [
        "phase-map.md",
        "parity-audit-and-frontend-gap-roadmap.md",
        "../CURRENT.md",
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
        "Phase 61 - Suite Metadata and Genre Platform",
        "Post-Phase 61 Performance Hardening (B1-B4)",
        "Phase 62 - Identity, RBAC and Accountability Foundation",
    ],
    "docs/NEW-CHAT-HANDOFF.md": [
        "Phase 61 - Suite Metadata and Genre Platform",
        "VDR Remote and Live Overlay hardening (#110)",
        "Backend-scoped Global Search (#111)",
        "Phase 62 - Identity, RBAC and Accountability Foundation",
    ],
    "docs/CURRENT.md": [
        "Phase 61 - Suite Metadata and Genre Platform",
        "VDR Remote and Live Overlay hardening (#110)",
        "Backend-scoped Global Search (#111)",
        "Phase 62 - Identity, RBAC and Accountability Foundation",
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
    return 0


if __name__ == "__main__":
    sys.exit(main())
