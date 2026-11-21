#!/usr/bin/env python3
"""Canonical parser for the numbered runtime-phase status in docs/CURRENT.md.

`docs/CURRENT.md` is the sole repository authority for volatile phase status.
Generic documentation/status guards import this module instead of hard-coding a
specific latest/next phase pair. Phase-specific historical guards remain free to
protect their immutable contracts without becoming current-status authorities.
"""

from dataclasses import dataclass
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
CURRENT = ROOT / "docs" / "CURRENT.md"

LATEST_LABEL = "Latest completed numbered runtime phase:"
ACTIVE_LABEL = "Current active numbered runtime phase:"
NEXT_LABEL = "Next strict numbered runtime phase:"

_PHASE_RE = re.compile(r"^Phase\s+(\d+)\s+-\s+(.+)$")


@dataclass(frozen=True)
class PhaseStatus:
    latest_completed: str
    current_active: str
    next_phase: str
    latest_completed_number: int
    next_phase_number: int

    @property
    def current_active_is_none(self) -> bool:
        return self.current_active.startswith("none - ")

    @property
    def latest_roadmap_heading(self) -> str:
        return self.latest_completed.replace(" - ", " — ", 1)

    @property
    def next_roadmap_heading(self) -> str:
        return self.next_phase.replace(" - ", " — ", 1)

    @property
    def latest_closeout_rel(self) -> str:
        return f"docs/development/phase-{self.latest_completed_number}-closeout.md"

    @property
    def latest_closeout_path(self) -> Path:
        return ROOT / self.latest_closeout_rel

    @property
    def next_not_started_marker(self) -> str:
        if not self.current_active_is_none:
            return ""
        return self.current_active.removeprefix("none - ")


def _value_after_label(text: str, label: str) -> str:
    lines = text.splitlines()
    positions = [index for index, line in enumerate(lines) if line.strip() == label]
    if len(positions) != 1:
        raise ValueError(
            f"docs/CURRENT.md must contain exactly one {label!r}; found {len(positions)}"
        )

    for line in lines[positions[0] + 1 :]:
        value = line.strip()
        if not value or value == "```":
            continue
        return value

    raise ValueError(f"docs/CURRENT.md has no value after {label!r}")


def _parse_number(value: str, label: str) -> int:
    match = _PHASE_RE.fullmatch(value)
    if not match:
        raise ValueError(
            f"{label} must use canonical 'Phase N - Title' spelling; got {value!r}"
        )
    return int(match.group(1))


def parse_phase_status(text: str) -> PhaseStatus:
    latest = _value_after_label(text, LATEST_LABEL)
    active = _value_after_label(text, ACTIVE_LABEL)
    next_phase = _value_after_label(text, NEXT_LABEL)

    latest_number = _parse_number(latest, LATEST_LABEL)
    next_number = _parse_number(next_phase, NEXT_LABEL)

    if next_number <= latest_number:
        raise ValueError(
            "next numbered runtime phase must be newer than latest completed phase"
        )

    if active.startswith("none - "):
        expected = f"Phase {next_number}"
        if expected not in active or "not started" not in active.casefold():
            raise ValueError(
                "inactive phase status must identify the next phase and state that it has not started"
            )
    else:
        active_number = _parse_number(active, ACTIVE_LABEL)
        if active_number != next_number:
            raise ValueError(
                "when a numbered runtime phase is active it must match the next strict phase"
            )

    return PhaseStatus(
        latest_completed=latest,
        current_active=active,
        next_phase=next_phase,
        latest_completed_number=latest_number,
        next_phase_number=next_number,
    )


def load_phase_status(path: Path = CURRENT) -> PhaseStatus:
    if not path.is_file():
        raise ValueError(f"phase status authority is missing: {path.relative_to(ROOT)}")
    return parse_phase_status(path.read_text(encoding="utf-8"))


def replace_current_status_values(
    text: str,
    *,
    completed: str,
    active: str,
    next_phase: str,
) -> str:
    """Replace only the three canonical status values in CURRENT.md text."""

    replacements = {
        LATEST_LABEL: completed,
        ACTIVE_LABEL: active,
        NEXT_LABEL: next_phase,
    }
    lines = text.splitlines()

    for label, value in replacements.items():
        positions = [index for index, line in enumerate(lines) if line.strip() == label]
        if len(positions) != 1:
            raise ValueError(
                f"docs/CURRENT.md must contain exactly one {label!r}; found {len(positions)}"
            )
        label_index = positions[0]
        for value_index in range(label_index + 1, len(lines)):
            candidate = lines[value_index].strip()
            if not candidate or candidate == "```":
                continue
            lines[value_index] = value
            break
        else:
            raise ValueError(f"docs/CURRENT.md has no value after {label!r}")

    updated = "\n".join(lines) + ("\n" if text.endswith("\n") else "")
    parse_phase_status(updated)
    return updated
