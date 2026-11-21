# Architecture Audit Gap Matrix

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Planning Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Completed Architecture Source Audit](../development/architecture-source-audit-2026-07-15.md)
- [ADR Index](../adr/index.md)

---

## Purpose

This is the living implementation-gap register produced from the completed 2026-07-15 architecture source audit.

It answers:

```text
What is already implemented?
What exists only as a foundation?
What is missing?
Which ADR defines the target?
In which roadmap phase is the gap closed?
```

It does not replace:

- [Completed Phases](../development/completed-phases.md), which records finished implementation;
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md), which focuses on product and ecosystem parity;
- [Roadmap](roadmap.md), which defines strict execution order.

---

## Status Legend

| Status | Meaning |
| --- | --- |
| Implemented | The required foundation exists in the current repository and is covered by implementation or tests. |
| Strong foundation | Most core mechanics exist, but the final cross-domain contract is incomplete. |
| Partial | Some domain or runtime behavior exists, but important required semantics are missing. |
| Missing | No complete implementation boundary exists yet. |
| Decision accepted | Architecture is decided, but runtime implementation has not started or is incomplete. |
| Deferred | Intentionally postponed until prerequisite phases are complete. |

---

## Executive Gap Summary

Current strengths:

- stable backend IDs and backend registry;
- backend-scoped access mode and server-enforced read-only behavior;
- backend-scoped snapshots and change-feed sequencing;
- guarded Recording preview, validation and execution foundations;
- native timer action foundation;
- SearchTimer domain, preview and real-backend validation;
- lazy Recording loading and frontend module boundaries;
- metadata, person and provider foundations;
- strong documentation and regression guardrails.

Largest remaining production gaps:

- user, service and Agent identity with real scoped RBAC;
- Backend Agent runtime, generation, heartbeat, lease and health;
- universal revisions, idempotency and stale-state protection;
- durable job claim, retry, cancellation and compensation semantics;
- TimerIntent, assignment, scheduler and reconciler;
- canonical programme-event identity and provenance;
- suite-owned metadata and artwork services;
- authenticated streaming sessions;
- isolated Legacy OSD bridge;
- versioned public API and audit/security event model.

---

## Detailed Gap Register

| ID | Architecture capability | Current status | Current evidence or limitation | Target decision | Roadmap destination |
| --- | --- | --- | --- | --- | --- |
| G-01 | Control Plane and Backend Agent boundary | Decision accepted | VDR-Suite currently has direct backend adapters and a backend registry, but no separate remote Agent runtime protocol. | ADR-0039 | Phase 63 |
| G-02 | Stable BackendId, native identities, generation and lease | Partial | Stable `backendId` exists. Native identity mapping, runtime generation, heartbeat, lease expiry and fencing are missing. | ADR-0040 | Phase 63 |
| G-03 | Concrete capability contract and degradation model | Strong foundation | Capability sets and reports exist, but capability revision, origin, temporary unavailability and channel-scoped capability semantics remain incomplete. | ADR-0012 plus ADR-0048 | Phase 63 and Phase 67 |
| G-04 | Backend-scoped RBAC and read-only policy | Partial | Server-enforced read-only access mode exists. Per-user roles, grants, scopes and service or Agent actors do not. | ADR-0013, ADR-0041, ADR-0049 | Phase 62 |
| G-05 | Immutable backend snapshots | Implemented | Backend-scoped snapshot objects and cache services exist. Additional generation and revision metadata is still required. | ADR-0016 and ADR-0018 | Phase 63 hardening |
| G-06 | Snapshot revision, event sequence and full resync vocabulary | Partial | Change-feed sequence and snapshot generation exist. Backend generation, resource revision and cursor semantics are not yet consistently separated. | ADR-0016, ADR-0018, ADR-0048 | Step 1 contracts, Phase 63 and Phase 67 |
| G-07 | Common mutation preview, validation, execution and verification contract | Strong foundation | Recording actions already have preview, validation, policy and execution. The same contract is not universal across all mutation domains. | ADR-0042 | Step 1, then domain implementation slices |
| G-08 | Idempotency and optimistic concurrency | Missing | Current action requests do not consistently carry `expectedRevision`, `backendGeneration` or an idempotency key. | ADR-0042 | Required before new real mutation paths and Phase 63 writes |
| G-09 | Native lock and pointer isolation | Partial | Adapter boundaries exist, but this remains a mandatory review rule for plugin integrations and native mutation paths. | ADR-0007 plus architecture invariant | Continuous; enforced in every adapter phase |
| G-10 | Stable suite RecordingId and native Recording binding | Partial | Backend ID, native recording data and fingerprints exist, but a durable suite-owned Recording identity and revisioned native binding are incomplete. | ADR-0014 and ADR-0042 | Phase 60.15 and Phase 61, mutation hardening before Phase 63 |
| G-11 | Trash, restore and purge lifecycle | Partial | Guarded recording actions exist. A single canonical trash/restore/purge lifecycle with idempotent state transitions is not complete. | ADR-0024 and ADR-0042 | Domain hardening before remote writes |
| G-12 | Durable job, retry and saga model | Partial | A local job record and Recording action payload exist. Claim lease, attempts, retry schedule, verification, cancellation and compensation are missing. | ADR-0043 | Step 1; used by Phase 61 and Phase 63 |
| G-13 | TimerIntent, TimerAssignment and NativeTimer separation | Missing | Native timer actions and SearchTimer workflows exist, but user intent and backend assignment are not independent durable objects. | ADR-0044 | Phase 64 |
| G-14 | Capability-aware scheduler and reconciler | Missing | No central assignment, ownership, re-evaluation or reconciliation runtime exists across backends. | ADR-0044 | Phase 64 |
| G-15 | BackendEventRef and canonical ProgramEvent | Partial | Backend event data exists, but canonical programme identity is not separated consistently from backend event identity. | ADR-0045 | Step 1, Phase 61 and Phase 64 |
| G-16 | EPG provenance and merge policy | Partial | EPG and metadata foundations exist. Provider-event provenance, merge rules and field-level evidence are not a complete shared model. | ADR-0045 and ADR-0038 | Phase 61 |
| G-17 | Suite-owned metadata entities and artwork assets | Partial | Metadata, people and provider foundations exist. Normalized entity assignment, provenance and suite-owned artwork delivery are incomplete. | ADR-0038 | Phase 60.15 and Phase 61 |
| G-18 | Unified automation-provider boundary | Partial | SearchTimer and epgsearch integration foundations exist. Providers still need to produce central TimerIntents rather than independently own native timer mutation. | ADR-0029 and ADR-0044 | Phase 64 |
| G-19 | Streaming Gateway and authenticated media sessions | Missing | Live transport and Streamdev integration concepts exist, but no public session gateway isolates internal media endpoints. | ADR-0046 | Phase 65 |
| G-20 | Legacy OSD bridge and controller lease | Missing | OSD is intentionally not the primary UI. No hardened bridge with viewer/controller roles, sequencing and resync exists. | ADR-0047 | Phase 66 |
| G-21 | Central database is not a client or Agent protocol | Decision accepted | Existing architecture uses service and adapter boundaries. The new metadata and Agent designs must preserve this invariant. | ADR-0038 and ADR-0039 | Phase 61 and Phase 63 |
| G-22 | Agent authentication, protected transport and credential rotation | Missing | Backend access mode exists, but no Agent enrollment, certificate or credential lifecycle and protected site transport runtime exists. | ADR-0041 | Phase 62 and Phase 63 |
| G-23 | Explicit multi-site trust boundary | Partial | Multi-backend and read-only policy foundations exist. Remote sites still need authenticated Agent sessions and deterministic trust and revocation. | ADR-0039 through ADR-0041 | Phase 62 and Phase 63 |
| G-24 | Audit and security event model | Missing | Diagnostics and logs exist, but mutations do not yet have a universal actor, request, decision and outcome audit record. | ADR-0049 | Phase 62 |
| G-25 | Public API version, error and compatibility contract | Partial | REST controllers and a frontend client wrapper exist. A stable `/api/v1`, common errors, ETags, deprecation and compatibility policy do not. | ADR-0048 | Phase 67 |
| G-26 | Plugin adapter capability degradation | Partial | Adapter and capability foundations exist. Per-plugin-version degradation, unsafe-operation disablement and temporary capability state need a common contract. | ADR-0007, ADR-0012, ADR-0048 | Phase 63 and Phase 67 |
| G-27 | epgd and epg2vdr migration or provider strategy | Deferred | Concepts are audited, but no direct shared-database integration is approved. Migration and provider adapters require Phase 61 identities and provenance first. | ADR-0038 and ADR-0045 | Phase 61 after core schema foundation |
| G-28 | Shared and remote storage semantics | Partial | Recording paths and backend ownership exist. Shared filesystems, remote storage identity, move semantics and ownership across sites remain undefined. | ADR-0014, ADR-0042 and future targeted storage decision | After Phase 63; before cross-site storage mutations |
| G-29 | Offline Agent synchronization and reconciliation | Partial | Snapshot resync foundations exist. Durable offline Agent queues, reconnect reconciliation and generation-aware command outcomes do not. | ADR-0040 and ADR-0043 | Phase 63 |
| G-30 | Timer failover and redundant recording policy | Missing | Multi-backend timers exist only as backend-native operations. Failover, reassignment and deliberate redundancy require TimerIntent and scheduler semantics. | ADR-0044 | Phase 64 |

---

## Priority View

### P0 - Required before production remote writes

- G-01 Control Plane and Backend Agent boundary
- G-02 generation, lease and fencing
- G-04 real RBAC
- G-07 common mutation contract
- G-08 idempotency and optimistic concurrency
- G-12 durable jobs and sagas
- G-22 Agent authentication and transport
- G-23 multi-site trust
- G-24 audit and security events
- G-29 offline reconciliation

### P1 - Required for the next product platform

- G-10 stable Recording identity
- G-13 and G-14 timer intent and orchestration
- G-15 and G-16 event identity and provenance
- G-17 metadata and artwork platform
- G-18 automation-provider unification
- G-19 streaming gateway
- G-25 public API contract
- G-26 capability degradation

### P2 - Compatibility and later platform expansion

- G-20 Legacy OSD bridge
- G-27 epgd and epg2vdr migration adapters
- G-28 shared and remote storage
- G-30 timer failover and deliberate redundancy

---

## Relationship to the Strict Roadmap

The authoritative order is:

```text
Step 1  ADR-0042 through ADR-0049 and diagrams
Step 2  Phase 60.15 Recording metadata preparation
Step 3  Phase 61 Suite metadata platform
Step 4  Phase 62 Identity, RBAC and audit
Step 5  Phase 63 Backend Agent and multi-site runtime
Step 6  Phase 64 Timer intent and orchestration
Step 7  Phase 65 Streaming Gateway
Step 8  Phase 66 Legacy OSD bridge
Step 9  Phase 67 Public API and client hardening
Step 10 Phase 68 Recommendation and knowledge graph
```

A gap is not considered closed merely because an ADR is accepted. Closure requires implementation, tests, documentation and the relevant phase exit criteria.

---

## Relationship to the Older Parity Matrix

[Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md) remains useful for RESTfulAPI, Live, epgsearch, VDR-Core and frontend product parity.

Its older Phase 57 and Phase 58 sequencing notes are historical. The strict roadmap and this architecture gap matrix are authoritative for future execution order.

The parity matrix should be refreshed during the relevant domain phase rather than used as the architecture source of truth.

---

## Maintenance Rules

- Update a row only when repository evidence changes.
- An accepted ADR changes the target-decision column, not automatically the implementation status.
- A phase completion must update this matrix, the phase map and Completed Phases together.
- New gaps require an ID, evidence, an owner ADR or explicit decision need, and a roadmap destination.
- Do not remove a closed gap; mark it Implemented and preserve the history through Git.
- Additional source audits should link their evidence document from the affected row.

---

## Back

- [Back to Planning Index](index.md)
- [Back to Strict Roadmap](roadmap.md)
- [Back to Current State](../CURRENT.md)
- [Back to Documentation Index](../index.md)
