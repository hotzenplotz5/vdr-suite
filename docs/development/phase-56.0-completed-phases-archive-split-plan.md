# Phase 56.0 - Completed Phases Archive Split Plan

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)

---

## Purpose

Phase 56 starts the developer documentation and library boundary milestone.

The first documentation risk to reduce is `docs/development/completed-phases.md`.

That file currently acts as both:

- the authoritative completed-phase marker for phase consistency checks
- the full chronological archive for many historical phases

This makes future GitHub-first documentation updates riskier than necessary because a small status change can require touching a very large historical file.

Phase 56.0 defines the split plan before any historical content is moved.

---

## Problem Statement

`completed-phases.md` has grown into a mixed-purpose file.

Current responsibilities:

- navigation to current project state
- milestone overview
- latest completed implementation marker
- chronological detailed phase history
- historical traceability for many independent implementation tracks

Risk:

- large connector fetches can be truncated
- full-file replacements become fragile
- small completion updates can conflict with unrelated historical content
- future archive changes can accidentally disturb the phase-consistency marker

---

## Target Structure

Keep `completed-phases.md` small and authoritative.

Planned structure:

```text

docs/development/completed-phases.md
docs/development/completed-phases/
  phase-46.md
  phase-47.md
  phase-48.md
  phase-49.md
  phase-50.md
  phase-51.md
  phase-52.md
  phase-53.md
  phase-54.md
  phase-55.md
```

`completed-phases.md` should retain:

- navigation
- purpose
- explicit latest completed implementation marker
- completed milestone overview
- archive index links
- maintenance rules
- back links

Archive files should contain the detailed chronological phase history grouped by phase range.

---

## Required Marker Contract

The phase-consistency checker must not depend on the newest `## Phase ...` heading inside a large historical archive.

Instead, `completed-phases.md` should expose the same explicit marker style used by README, current status and the development index:

```text
Latest completed implementation phase:

```text
Phase 55.6 - Recording operations audit and safety policy
```
```

After the marker exists, `tools/check_phase_consistency.py` can read `completed-phases.md` through that marker instead of scanning all phase headings.

---

## Migration Order

Use small reviewable commits.

1. Add this plan.
2. Add the explicit latest-completed marker to `completed-phases.md` while leaving the historical content untouched.
3. Update `tools/check_phase_consistency.py` so `completed-phases.md` uses the explicit marker.
4. Add the `docs/development/completed-phases/` archive directory and move one phase-range group at a time.
5. Link every archive file from `completed-phases.md` and `docs/development/index.md`.
6. Run documentation and phase checks after every migration step.
7. Only after successful migration, keep `completed-phases.md` as the compact entry point.

---

## Safety Rules

- Do not rewrite the large historical file through a truncated GitHub fetch.
- Do not move all historical ranges in one unreviewable commit.
- Do not remove historical phase content until the destination archive file is linked and reachable.
- Do not change latest-completed semantics during the archive split.
- Do not mix library boundary implementation with historical archive migration.

---

## Verification

Required after each step:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

When guardrails are touched, also run:

```bash
make test-github-update-safety-handoff
```

---

## Phase Boundary

Phase 56.0 is documentation architecture only.

It does not:

- move historical content yet
- change runtime behavior
- change API behavior
- change backend mutation behavior
- change package layout
- change library boundaries

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
