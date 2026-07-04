# Phase 58.90c: Documentation Consolidation

Date: 2026-07-04

## Goal

Bring the documentation state in line with Phase 58.90a and Phase 58.90b directly in the GitHub repository.

## Stable technical state

```text
2f66168d Phase 58.90b: add stable channel sorter
```

Tag:

```text
v1.58.90b-stable-channel-sorter
```

## Updated documents

- `README.md`
- `docs/CURRENT.md`
- `docs/NEW-CHAT-HANDOFF.md`
- `docs/development/current-status.md`
- `docs/development/completed-phases.md`
- `docs/development/completed-phases-latest.md`
- `docs/planning/phase-map.md`
- `docs/planning/roadmap.md`
- `docs/planning/project-progress.md`

## State model

The latest completed major project block remains:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

The latest completed implementation slice is:

```text
Phase 58.90b - Stable Channel Sorter
```

Phase 58 remains in progress as the current frontend and live-parity block.

## Stable channel sorter scope

- `Kanaele sortieren` frontend module
- drag only on the left handle
- touch and desktop operation
- normal scrolling remains possible
- guarded Channel Move API usage

## Deliberate exclusion

The experimental post-move focus restore is not part of the stable state.

## Expected verification

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```
