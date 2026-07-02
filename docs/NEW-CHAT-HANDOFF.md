# VDR-Suite New Chat Handoff

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Current State](CURRENT.md)
- [Roadmap](planning/roadmap.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR Index](adr/index.md)
- [GitHub Actions Status Handoff](development/github-actions-status-handoff.md)

---

## Purpose

This file is the short project handoff that a new chat should read first.

It does not replace the existing specialized CI handoff.

When GitHub Actions status matters, use [GitHub Actions Status Handoff](development/github-actions-status-handoff.md).

It is intentionally compact and points to the current source documents instead of repeating the whole phase history.

---

## Required First Reading

A new chat should start with these files in this order:

1. [Current State](CURRENT.md)
2. [Roadmap](planning/roadmap.md)
3. [Phase Map](planning/phase-map.md)
4. [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
5. [ADR Index](adr/index.md)
6. [Completed Phases](development/completed-phases.md)
7. [GitHub Actions Status Handoff](development/github-actions-status-handoff.md) when CI state matters

Detailed phase notes should be opened only when a specific historical detail is needed.

---

## Current Repository Truth

Latest completed major project block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Current implementation focus:

```text
Phase 58 - Frontend and Live Parity
```

Do not move global latest-completed markers from Phase 57 to a Phase 58 subphase unless the full Phase 58 major block is completed and the tracked marker files are updated together.

---

## Phase 57 Result

Phase 57 is complete.

The important result is the multi-site backend administration and permission foundation before deeper frontend and Live parity work.

Verified Phase 57 outcomes:

- backend access modes for read-write and read-only sites
- backend registry JSON permission hints
- recording action backend access handling
- timer action backend access coverage
- SearchTimer backend access coverage
- frontend-visible backend permission state

---

## Phase 58 Direction

Phase 58 uses the parity audit as input for the frontend and Live-parity validation block.

The frontend should expose existing backend capabilities and reveal real gaps instead of guessing missing features.

Current Phase 58 guardrails:

- bounded EPG input must be used for frontend channel-card now/next data
- global RESTfulAPI event query parameters must be preserved
- unbounded `/events.json` full dumps must not become default UI input
- startup snapshots must stay lightweight
- startup may load status, timers, SearchTimer metadata and channels
- startup must not load recordings or full EPG events
- SearchTimerPreviewEpgCache remains preview-scoped
- global snapshot event reads remain snapshot-backed
- reusable persistent EPG data belongs to the backend-scoped EPG database foundation

---

## Phase 58.40 Foundation Result

Phase 58.40 added the backend-scoped persistent EPG database foundation.

Architecture rule:

```text
Every persisted EPG event belongs to exactly one backend.
```

Persistent identity:

```text
backend_id + channel_id + event_id
```

Allowed repository API shape:

```text
upsertEventsForBackend(...)
findNowNextForBackend(...)
findWindowForBackend(...)
deleteExpiredForBackend(...)
countForBackend(...)
```

Disallowed repository API shape:

```text
upsertEvents(...)
findNowNext(...)
findWindow(...)
deleteExpired(...)
count(...)
```

Phase 58.40 is database/repository foundation only. It does not add daemon startup EPG loading, SSE-driven synchronization or frontend route migration.

---

## Current Architecture Boundary

Current simplified architecture chain:

```text
RuntimeConfig
  -> VdrConfig
  -> VdrAdapterFactory
  -> IVdrAdapter
  -> ExternalVdrAdapter / MockVdrAdapter / RestfulApiVdrAdapter
  -> VdrService
  -> REST controllers
```

Boundary rule:

```text
Core modules may not depend on api/rest.
The REST API layer may depend on core modules.
```

---

## Documentation Rules

Use [Current State](CURRENT.md) for current truth.

Use [Roadmap](planning/roadmap.md) for planned direction.

Use [Phase Map](planning/phase-map.md) for compact phase-range coverage.

Use [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md) for RESTfulAPI, Live, epgsearch and VDR-Core parity questions.

Use [Completed Phases](development/completed-phases.md) for milestone history.

Use `docs/development/phase-*` files only as historical implementation records.

The old `docs/roadmap/` directory is a forward-roadmap archive, not the current roadmap.

---

## Required Verification Before Declaring Work Complete

For documentation-only or guardrail-only changes:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

For install-layout changes:

```bash
make test-install-staging
make test-docs
make test-phase
make test-phase-map-coverage
```

For runtime-sensitive changes, also run the applicable runtime, daemon and real VDR acceptance checks.

GitHub Actions must be green before a pushed phase or reset block is considered complete.

CI status command:

```bash
tools/watch_github_ci.py --watch --interval 60 --url --chat
```

---

## Current Known Documentation Cleanup

DOC-RESET-1 created the current-state entry point.

DOC-RESET-2 aligned roadmap, ADR index, completed-phase markers and new-chat handoff links.

The parity audit captures the product gap view before DOC-RESET-3.

DOC-RESET-3 should add stronger documentation tooling checks for:

- current entry points
- ADR duplicate active numbers
- completed-phase marker alignment

---

## Back

- [Back to Current State](CURRENT.md)
- [Back to Documentation Index](index.md)
- [Back to README](../README.md)
