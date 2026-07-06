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
4. [Current Project Status](development/current-status.md)
5. [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
6. [Client API and Frontend Module Boundary Plan](development/client-api-frontend-module-boundary-plan.md)
7. [RESTfulAPI Integration Architecture](architecture/restfulapi-integration.md)
8. [EPGSearch Capability Matrix](development/epgsearch-capability-matrix.md)
9. [ADR Index](adr/index.md)
10. [Completed Phases](development/completed-phases.md)
11. [GitHub Actions Status Handoff](development/github-actions-status-handoff.md) when CI state matters

Detailed phase notes should be opened only when a specific historical detail is needed.

---

## Current Repository Truth

Latest completed major project block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Latest completed implementation slice:

```text
Phase 59.09b - Web Frontend Direct API Fetch Guard
```

Previous completed major project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Current implementation focus:

```text
Phase 58 - Frontend and Live Parity
```

Keep the latest completed major project block at Phase 57 until the full frontend and Live-parity block is completed. Track completed Phase 59 frontend and parity slices separately; the latest stable slice is Phase 59.09b.

---

## Phase 59.07a Result

Phase 59.07a refreshes the Live, RESTfulAPI, epgsearch and Web Client API parity inventory.

Verified outcomes:

- Roadmap and current-status documents point to the current parity audit slice
- stale Live-parity entries are downgraded from missing/check where backend-neutral source support exists
- EPGSearch capability matrix records newer discovery, timer-conflict, regex, fuzzy and native fuzzy foundations
- RESTfulAPI integration documentation records later SearchTimer discovery and timer-conflict paths
- Web Client API parity is tracked as its own dimension before more frontend work
- `docs/CURRENT.md` and this handoff define the required reading order for future chats

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

## Phase 56 Result

Phase 56 is complete.

The important result is the install and boundary contract before real packaging.

Verified Phase 56 outcomes:

- source groups are split by responsibility
- the transitional recording-action aggregate is removed
- core/API boundary is documented
- REST API remains the application-facing boundary
- `make install DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr` stages installable files
- daemon, CLI, documentation, data directory and manpages are staged
- no public C++ ABI is promised
- no `vdr-suite-dev` package is introduced
- Debian packaging metadata is still deferred

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
- channel sorting must use guarded move APIs and preserve normal touch scrolling

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

Use [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md) for RESTfulAPI, Live, epgsearch, VDR-Core and Web Client API parity questions.

Use [Client API and Frontend Module Boundary Plan](development/client-api-frontend-module-boundary-plan.md) before proposing frontend data-loading, UI extraction or multi-client API work.

Use [RESTfulAPI Integration Architecture](architecture/restfulapi-integration.md) before proposing direct RESTfulAPI usage.

Use [EPGSearch Capability Matrix](development/epgsearch-capability-matrix.md) before proposing SearchTimer, EPG search, discovery catalog or conflict work.

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
```
---

## Back

- [Back to Documentation Index](index.md)
- [Back to Current State](CURRENT.md)
- [Back to README](../README.md)
