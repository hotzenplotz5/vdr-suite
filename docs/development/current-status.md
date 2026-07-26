# VDR-Suite Current Project Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current State](../CURRENT.md)
- [Phase Map](../planning/phase-map.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md)

---

## Purpose

This document tracks the current verified technical state of VDR-Suite. It summarizes accepted runtime work and points to detailed evidence rather than repeating the complete chronological archive.

---

## Current Verified State

```text
Latest completed runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening block:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next runtime implementation phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

`main` contains PR #100 and the performance PRs #102 through #108. The previous documentation state that described Phase 61 as an unmerged feature branch is obsolete.

---

## Latest Verified Implementation Result

Phase 61 completed the first accepted Suite-owned metadata/Genre vertical slice:

- persistent backend-scoped target bindings and assignments;
- Recording and EPG Genre materialization;
- provider evidence and derived browse-class persistence;
- explicit assignment states and conflict handling;
- indexed backend-scoped counts and pagination;
- provider-neutral REST and Web Client API routes;
- Genre frontend navigation using existing Recording and EPG detail owners;
- bounded asynchronous provider enrichment;
- restart persistence and live acceptance.

The detailed implementation and acceptance record is in [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md).

---

## Latest Verified Operational Result

The post-Phase 61 B1-B4 hardening block completed:

- EPG candidate-query fast path with equivalent results;
- atomic EPG Genre evidence persistence;
- no-op Recording Genre synchronization;
- integer EPG window index with query-plan guard;
- no-op EPG conflict upserts;
- 15-minute throttling of completed ETYPES snapshot cycles.

Live verification on the real yaVDR system proved:

- identical EPG events do not rewrite timestamps;
- changed events still update;
- unchanged Recording Genre refreshes avoid writer transactions;
- a completed ETYPES cycle stays idle for at least the configured interval;
- the next cycle restarts with offset zero and continues applying classified pages.

---

## Stable Runtime Scope

The current repository provides stable foundations for:

- VDR status, channels, EPG, recordings and timers;
- lazy Recording cache and browser;
- guarded Recording actions;
- SearchTimer workflows and preview;
- persistent metadata, people, artwork and Genre read paths;
- TVScraper EPG detail enrichment;
- backend-neutral remote actions and live overlay;
- backend access modes and read-only enforcement;
- frontend Client API and modular ownership;
- packaging, staging and daemon installation.

This is a strong platform foundation, but not complete ecosystem parity.

---

## Important Remaining Gaps

The next implementation work remains:

1. actor identities and secure sessions;
2. scoped roles and permissions;
3. centralized server-side authorization;
4. append-only accountability persistence and outbox;
5. secure Backend Agent runtime;
6. Timer intent/orchestration;
7. Streaming Gateway;
8. legacy OSD compatibility bridge;
9. stable `/api/v1` and compatibility contracts;
10. later recommendation/knowledge-graph work.

Detailed feature parity against VDR Core, Live, epgsearch and RESTfulAPI is maintained in [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md).

---

## Immediate Implementation Focus

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

The first Phase 62 checkpoint should add actor identity, authorization and accountability value objects and persistence without yet introducing remote Agent writes. Existing server-enforced read-only backend policy must remain intact during migration.

---

## Documentation Safety

The authoritative current-state files are:

- `docs/CURRENT.md`
- `docs/NEW-CHAT-HANDOFF.md`
- `docs/development/current-status.md`
- `docs/development/completed-phases-latest.md`
- `docs/planning/phase-map.md`
- `docs/planning/roadmap.md`

They must use the same completed and next-phase markers.

Verification:

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

---

## Back

- [Back to Development Index](index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)