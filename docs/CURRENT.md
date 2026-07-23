# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Metadata-Backed Genre Browser](architecture/metadata-genre-browser.md)
- [Domain Dependency Map](planning/domain-dependency-map.md)
- [Implementation Dependency Map](planning/implementation-dependency-map.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Completed Architecture Source Audit](development/architecture-source-audit-2026-07-15.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR Index](adr/index.md)
- [Completed Phases](development/completed-phases.md)
- [Completed Phases Latest Marker](development/completed-phases-latest.md)

---

## Current Verified State

Current completed project block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Previous completed major project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Historical umbrella implementation track:

```text
Phase 58 - Frontend and Live Parity
```

Latest completed implementation slice:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

Current feature-branch implementation slice:

```text
Phase 61 - persistent normalized Genre index and metadata-backed Genre browser
Branch: feature/phase61-metadata-genre-browser
Status: implementation under review; repository-local build and real-system acceptance pending
```

Completed architecture contract package:

```text
ADR-0042 through ADR-0049
Target Platform Architecture
Domain Dependency Map
Implementation Dependency Map
```

Current runtime implementation phase:

```text
Phase 61 - Suite Metadata Database and External Provider Integration
```

The Phase 58 umbrella label remains product-history grouping. It does not control the strict future sequence.

---

## Phase 61 Genre Vertical Slice

The feature branch now contains the first complete metadata runtime/read-model path for Genre browsing:

```text
persistent Recording and EPG sources
  -> canonical backend-scoped Genre assignments
  -> indexed SQL counts and pagination
  -> provider-neutral Suite REST
  -> DOM-free Client API
  -> genres frontend module
  -> existing Recordings 2 and EPG detail owners
```

Implemented contracts include:

- canonical aliases plus stable `unknown-*` and `unclassified` identities;
- multiple Genres per Recording or EPG event;
- explicit `active`, `missing`, `unknown`, `stale` and derived `conflict` states;
- restart-persistent backend-scoped target bindings and assignments;
- bounded asynchronous EPG provider enrichment owned by the existing cache worker;
- no provider lookup from Genre HTTP GET or frontend rendering;
- distinct SQL counts, limit/offset result pages and a 48-hour EPG browser window;
- read-only remote-backend browsing without weakening mutation policy;
- PR #99 LiveRemote routing precedence preserved;
- reuse of the existing Recordings 2 card/detail owner and existing EPG detail card;
- unchanged EPG timeline ownership.

This slice is not accepted as completed history until the branch is built and verified on the real VDR-Suite checkout. See [Metadata-Backed Genre Browser](architecture/metadata-genre-browser.md).

---

## Completed Architecture Audit

```text
Architecture Source Audit - 2026-07-15
Status: Completed evidence and decision activity
```

The audit covered VDR Core, epgsearch, Live, RESTfulAPI, Streamdev, TVScraper, scraper2vdr, osd2web, epg2vdr and epgd.

Results are split into:

- [Completed audit evidence](development/architecture-source-audit-2026-07-15.md)
- [Living implementation-gap matrix](planning/architecture-audit-gap-matrix.md)
- [Strict future execution order](planning/roadmap.md)
- [Canonical target diagrams](architecture/target-platform-architecture.md)
- [Domain dependencies](planning/domain-dependency-map.md)
- [Implementation dependencies](planning/implementation-dependency-map.md)

The completed audit and contract package are not completed runtime implementation.

---

## Accepted Architecture Package

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
ADR-0046 - Streaming Gateway and Media Session Boundary
ADR-0047 - Legacy OSD Compatibility Bridge
ADR-0048 - Public API Versioning, Error and Compatibility Contract
ADR-0049 - Audit and Security Event Model
```

The accepted package establishes:

- Control Plane, Backend Agent and VDR/plugin ownership;
- backend identity, generation, lease, health and trust;
- common mutation revision, idempotency, dispatch and verification semantics;
- durable operations, jobs, attempts, retries, sagas and reconciliation;
- TimerIntent, TimerAssignment and NativeTimerBinding separation;
- ProgramEvent, BackendEventRef, observations and provenance;
- MediaSession, MediaRoute, ProviderStreamLease, MediaAccessGrant and PlaybackConnection;
- LegacyOsdSession, viewer binding, ordered frame/delta delivery and controller lease;
- `/api/v1`, structured errors, request context, ETags, pagination and compatibility;
- append-only AccountabilityEvent records and security-event classification.

The [Target Platform Architecture](architecture/target-platform-architecture.md) is the canonical diagram set. The [Domain Dependency Map](planning/domain-dependency-map.md) and [Implementation Dependency Map](planning/implementation-dependency-map.md) make prerequisite direction explicit.

---

## Runtime Status Boundary

The accepted contracts do not mark the following as implemented:

- universal revisions and durable idempotency;
- production worker claims, retries and sagas;
- actor identity, RBAC and append-only accountability persistence;
- Backend Agent enrollment, transport, generation, lease and reconnect runtime;
- complete canonical ProgramEvent/entity resolution beyond the Genre read-model slice;
- complete metadata artwork storage, derivative, backup and recovery runtime;
- TimerIntent persistence, scheduler, reconciler and failover;
- Streaming Gateway, media routes and access grants;
- Legacy OSD capture, sequencing, viewer fan-out or remote control;
- `/api/v1` route migration, ETags, common errors or cursor collections;
- Agent evidence buffering, audit outbox, retention, protected queries or exports.

Current native Timer actions, SearchTimer proposals, `VdrEvent` read models, warm EPG cache, persistent Genre read model, historical Stream Provider direction, existing OSD/remote adapter references, unversioned `ApiRouter`, DOM-free Web Client API, runtime logs and diagnostics remain strong foundations rather than the completed future platform.

---

## Immediate Repository Work

Validate the Phase 61 Genre vertical slice on the real checkout:

```text
repository-local focused and fast tests
daemon build and restart
SQLite persistence and restart behavior
Recording and EPG Genre counts on the default backend
read-only remote-backend isolation
Recordings 2 and EPG detail return navigation
unchanged EPG timeline and PR #99 live remote/overlay behavior
```

After explicit live acceptance, continue Phase 61 with:

```text
broader MetadataEntity and MetadataAssignment resolution
provider, provenance, evidence and confidence operations
artwork asset storage and derivative policy
backend-aware provider registry
migration, backup, observability and recovery hardening
```

Phase 61 must preserve:

- Recording browsing without metadata providers;
- lazy folder loading and cached fallback;
- backend scope and provider failure isolation;
- frontend module ownership;
- provider-neutral public contracts;
- migration, backup and recovery coverage.

---

## Strict Future Sequence

```text
1. Phase 61 - Suite Metadata Platform
2. Phase 62 - Identity, RBAC and Audit
3. Phase 63 - Backend Agent and Multi-Site Runtime
4. Phase 64 - Timer Intent and Orchestration
5. Phase 65 - Streaming Gateway
6. Phase 66 - Legacy OSD Bridge
7. Phase 67 - Public API and Client Hardening
8. Phase 68 - Recommendation and Knowledge Graph
```

The architecture package is the accepted prerequisite baseline for this sequence. It remains authoritative even though its later runtime components are incomplete.

---

## Documentation Reading Rule

Before proposing frontend, Live-parity, RESTfulAPI, epgsearch, metadata, multi-site or architecture work, inspect:

- `docs/CURRENT.md`
- `docs/planning/roadmap.md`
- `docs/planning/phase-map.md`
- `docs/architecture/target-platform-architecture.md`
- `docs/architecture/metadata-genre-browser.md`
- `docs/planning/domain-dependency-map.md`
- `docs/planning/implementation-dependency-map.md`
- `docs/planning/architecture-audit-gap-matrix.md`
- `docs/development/architecture-source-audit-2026-07-15.md`
- `docs/development/completed-phases.md`
- `docs/planning/parity-audit-and-frontend-gap-roadmap.md`
- `docs/development/client-api-frontend-module-boundary-plan.md`
- `docs/architecture/restfulapi-integration.md`
- `docs/development/epgsearch-capability-matrix.md`
- `docs/adr/index.md`

---

## Boundary Rules

- Completed Phases records finished implementation only.
- Feature-branch status is not completed history until repository-local and live acceptance gates pass.
- The completed source audit records evidence and conclusions only.
- The Architecture Audit Gap Matrix records open, partial and implemented gaps.
- The target diagrams define accepted ownership, not current runtime completion.
- The Domain Dependency Map defines conceptual prerequisite direction.
- The Implementation Dependency Map and roadmap own future execution order.
- The older parity matrix owns product and ecosystem parity questions, not architecture sequencing.
- Additional plugin source audits require a concrete feature, adapter, migration or risk question.

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
