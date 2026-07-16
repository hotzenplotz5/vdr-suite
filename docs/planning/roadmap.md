# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Phase Map](phase-map.md)
- [ADR Index](../adr/index.md)
- [Completed Phases](../development/completed-phases.md)
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)
- [Recording Metadata Roadmap](tvscraper-recording-metadata-roadmap.md)

---

## Purpose

This file defines the **forward execution order** of VDR-Suite.

It is not the chronological phase archive. Completed history belongs in [Completed Phases](../development/completed-phases.md), and compact phase-range coverage belongs in [Phase Map](phase-map.md).

The roadmap has one rule:

> Work is read from top to bottom. A later numbered phase does not begin before the required decisions and exit criteria of the earlier phase are complete.

---

## Current Verified Position

```text
Completed major project block
Phase 57 - Multi-Site Backend Administration and Permissions

Current umbrella implementation track
Phase 58 - Frontend and Live Parity

Latest completed implementation slice
Phase 60.14k - Recording Detail UX Polish

Next planned implementation slice
Phase 60.15 - Recording Metadata and Poster Preparation
```

The Phase 58 umbrella label describes the broad product track. It is **not** used to order future work. The actual execution order continues from Phase 60.15 onward.

Accepted architecture baseline:

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
```

---

# Strict Execution Order

## Step 1 - Complete the Architecture Contract Package

Status: Next repository work.

Before new runtime implementation begins, complete:

```text
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
ADR-0046 - Streaming Gateway and Media Session Boundary
ADR-0047 - Legacy OSD Compatibility Bridge
ADR-0048 - Public API Versioning, Error and Compatibility Contract
ADR-0049 - Audit and Security Event Model
```

Then update:

- architecture diagrams;
- the domain dependency map;
- the implementation dependency map;
- affected earlier ADR cross-references.

Exit criteria:

- stable identity vocabulary exists for backends, recordings, events, timers, metadata and assets;
- mutation requests define revision, idempotency, verification and audit behavior;
- asynchronous work defines claim, retry, cancellation and compensation behavior;
- client API, Agent API, streaming and Legacy OSD boundaries are distinct;
- no future runtime phase depends on an undefined trust, identity or consistency model.

---

## Step 2 - Phase 60.15: Recording Metadata and Poster Preparation

Status: Planned immediately after Step 1.

Goal:

- Prepare the existing Recording API and frontend for metadata and artwork without implementing the full metadata platform yet.

Scope:

- separate technical VDR data from normalized and provider-derived metadata;
- introduce provider-neutral artwork references;
- add poster and artwork placeholders that also work without provider data;
- keep lazy Recording folder loading and Recording detail navigation unchanged;
- keep external lookup latency outside synchronous list rendering;
- prevent direct frontend coupling to TVScraper, scraper2vdr or provider databases.

Exit criteria:

- the Recording API can represent technical, normalized and provider-derived fields separately;
- artwork uses a suite-owned asset identity or a clearly temporary placeholder contract;
- the frontend remains fully usable without enriched metadata;
- existing Recording browser regression coverage remains green.

---

## Step 3 - Phase 61: Suite Metadata Database and External Providers

Status: Planned after Phase 60.15.

Goal:

- Build the suite-owned normalized metadata platform defined by ADR-0038.

Implementation order inside Phase 61:

1. metadata entity and assignment identities;
2. database schema and migrations;
3. provider, provenance, evidence and confidence contracts;
4. artwork asset storage and delivery;
5. Recording enrichment read model;
6. sidecar, imported and plugin-backed provider adapters;
7. asynchronous refresh, retry and invalidation jobs;
8. frontend enrichment beyond the Phase 60.15 placeholders;
9. migration and operational hardening.

Exit criteria:

- no external provider database is authoritative for VDR-Suite;
- provider failures do not break Recording browsing;
- metadata and artwork are backend-neutral;
- stale and missing provider data have explicit states;
- enrichment work is observable, retryable and auditable.

---

## Step 4 - Phase 62: Identity, RBAC and Audit Foundation

Status: Planned after Phase 61.

Goal:

- Replace the current broad backend read-only/read-write hints with production-grade user, service and Agent authorization foundations.

Scope:

- users, service accounts and Agent identities;
- roles and permission grants;
- backend- and action-scoped authorization;
- server-side enforcement;
- secure sessions and credential lifecycle;
- mutation audit records and security events;
- preservation of the existing read-only backend baseline.

Exit criteria:

- different users can have different rights on the same backend;
- the second-house read-only use case is enforced server-side;
- every real mutation has an actor and audit outcome;
- Agent credentials can be enrolled, rotated and revoked.

---

## Step 5 - Phase 63: Backend Agent and Secure Multi-Site Runtime

Status: Planned after Phase 62.

Goal:

- Implement the Control Plane and Backend Agent boundary for remote VDR sites.

Scope:

- outbound authenticated Agent connections;
- persistent BackendId and native identity mapping;
- backend generation, heartbeat, lease and health;
- capability and snapshot publication;
- fenced command dispatch;
- offline, reconnecting and degraded states;
- no public exposure of RESTfulAPI, SVDRP, Streamdev or plugin-internal ports.

Exit criteria:

- a remote read-only backend works through the Agent boundary;
- stale Agent generations cannot complete current commands;
- lease expiry changes backend availability deterministically;
- remote writes remain disabled until all authorization and safe-mutation gates pass.

---

## Step 6 - Phase 64: Timer Intent and Multi-Backend Orchestration

Status: Planned after Phase 63.

Goal:

- Separate user and automation intent from backend-native timer execution.

Scope:

- TimerIntent;
- TimerAssignment;
- NativeTimer binding;
- scheduler and reconciler;
- backend capability and channel availability checks;
- deduplication and conflict handling;
- reassignment and failure recovery;
- epgsearch, SearchTimer and other automation providers producing intents instead of independent native writes.

Exit criteria:

- every native timer can be traced to an intent and assignment;
- only one backend owns an active assignment;
- backend failure does not silently create duplicate timers;
- native execution is read back and reconciled.

---

## Step 7 - Phase 65: Streaming Gateway and Media Sessions

Status: Planned after Phase 64.

Goal:

- Provide authenticated live and Recording playback without exposing internal Streamdev endpoints.

Scope:

- short-lived stream sessions;
- backend and resource authorization;
- gateway routing;
- expiry, limits and audit;
- range, seek and disconnect behavior;
- Streamdev as an internal provider only.

---

## Step 8 - Phase 66: Legacy OSD Compatibility Bridge

Status: Planned after Phase 65.

Goal:

- Provide controlled access to plugin functionality that does not yet have a native VDR-Suite domain UI.

Scope:

- isolated OSD sessions;
- `osd.view` and `osd.control` permissions;
- multiple viewers and one controller lease;
- sequence and resynchronization behavior;
- rate-limited key input;
- no free shell-command channel;
- no use as the primary Web or TV frontend architecture.

---

## Step 9 - Phase 67: Public API and Client Compatibility Hardening

Status: Planned after Phase 66.

Goal:

- Stabilize the platform contract for Web, Windows, Android, iOS and TV clients.

Scope:

- `/api/v1` contract;
- common error envelope;
- request and correlation IDs;
- pagination, revisions and ETags;
- deprecation and compatibility policy;
- capability negotiation;
- final client-independent API documentation.

---

## Step 10 - Phase 68: Recommendation and Content Knowledge Graph

Status: Later vision.

Goal:

- Build explainable recommendation and graph features only after metadata, identity, multi-site and API foundations are mature.

Prerequisites:

- stable metadata identities;
- mature provider provenance;
- people and character relationships;
- cross-backend Recording metadata;
- reliable audit and event history;
- stable public API contracts.

Phase 68 is not part of the immediate implementation sequence.

---

# Global Completion Gates

## Identity Gate

A persisted or synchronized resource requires:

- a stable suite identity;
- an explicit backend-native binding where applicable;
- revision and ownership semantics.

## Mutation Gate

A real mutation requires:

- authentication and authorization;
- capability validation;
- stale-state protection;
- idempotency behavior;
- result verification;
- audit-visible outcome.

## Asynchronous Work Gate

Slow provider, filesystem, network or cross-site work belongs in the job model. It must not run inside synchronous list rendering or long-held VDR locks.

## Source Audit Gate

The broad plugin audit is complete. Additional source audits are performed only for a concrete feature, adapter or risk question.

---

## Maintenance Rules

- [Phase Map](phase-map.md) is the compact phase-number source of truth.
- This roadmap defines the forward work order only.
- [Completed Phases](../development/completed-phases.md) owns chronological history.
- A phase number is not reused or renumbered after implementation.
- Accepted ADRs define direction but do not imply runtime completion.
- Later phases do not begin merely because they can be developed independently; the dependency order above is authoritative.

Verification:

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
