# VDR-Suite Current Architecture State

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Purpose

This document summarizes **implemented architecture by durable capability boundary**. It intentionally does not copy active PR numbers, exact current branch heads or CI checkpoints from [Current State](../CURRENT.md).

Historical exact acceptance evidence stays in phase/slice closeouts. Volatile implementation progress inside the active phase stays in `docs/CURRENT.md`.

## Ownership model

```text
VDR
  -> native devices, schedules, timers, recordings, replay, OSD and plugins

VDR-Suite Control Plane
  -> Suite identity and backend scope
  -> authentication, authorization and policy
  -> domain services and persistent read models
  -> operations, orchestration and reconciliation
  -> client-facing Suite contracts

Backend Agent
  -> enrolled technical identity and generation/instance fencing
  -> bounded observations and durable command/result transport
  -> explicit local provider ownership/selection
  -> site-local execution and cleanup

Private adapters/providers
  -> RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge and future adapters
```

Frontend modules do not call private backend protocols directly and do not own authentication, authorization, provider selection or retry safety decisions.

## Persistence and read architecture

Implemented foundations include:

- daemon-owned SQLite persistence and migrations;
- repository-owned SQL with service/controller separation;
- backend registry and backend-scoped read models;
- snapshots, caches and change-feed foundations;
- channels and EPG read models;
- Recordings 2, Recording metadata/people/artwork/Genre data;
- query-oriented global search and related persistent metadata reads;
- backend-neutral RemoteAction and LiveOverlay contracts.

Provider data remains evidence behind Suite persistence/contracts rather than becoming public authority.

## Identity, authorization and accountability

The implemented security boundary includes:

- persistent actor, device, credential and session identity;
- browser-session lifecycle and CSRF enforcement;
- exact backend-scoped permission grants and fixed-role semantics;
- strict server-side protected-mutation classification;
- append-only pre-dispatch accountability and protected operation outcomes;
- secret handling that keeps raw session/credential material out of durable accountability data.

Legacy Basic compatibility remains transitional where retained by deployment compatibility policy; its historical Phase-62 decision does not define the target identity architecture.

## Backend Agent and secure multi-site foundation

The implemented Agent architecture includes bounded support for:

- Agent enrollment and technical identity;
- credential lifecycle and generation/instance fencing;
- heartbeat/lease and backend health semantics;
- read-only observation/snapshot ingestion with sequence/resync rules;
- durable command delivery, receipts/results and reconnect handling;
- fenced native operation execution;
- explicit local provider ownership and selection;
- protected-write safety contracts that prevent silent provider fallback and stale-generation completion.

This capability is a reusable platform foundation for later Timer and media work. Exact currently active Timer/native-write checkpoint details belong only in [Current State](../CURRENT.md).

## Protected-write safety model

The implemented direction for protected native writes is:

```text
operation / intent
  -> authorization and backend/provider eligibility
  -> idempotency scope
  -> expected revision where applicable
  -> resource-scoped concurrency/lease fencing
  -> backend generation / provider ownership fence
  -> durable dispatch boundary
  -> one-shot or otherwise bounded native execution
  -> authoritative readback
  -> verified success / verified no-effect / outcome unknown
  -> reconciliation before any unsafe retry
```

A transport timeout after possible dispatch is not treated as proof of failure. Provider availability does not create authority and active execution does not silently switch providers.

## Timer architecture

The implemented Phase-64 foundation uses the accepted separation:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

`TimerIntent` is backend-neutral Suite-owned intent. `TimerAssignment` records deterministic backend ownership/role. `NativeTimerBinding` represents the backend-native VDR Timer relationship and observation state.

Current Phase-64 implementation progress is intentionally not repeated here; see [Current State](../CURRENT.md).

## Media architecture state

The accepted media target is defined by [ADR-0046](../adr/ADR-0046-streaming-gateway-media-session-boundary.md):

```text
Client
  -> authenticated Suite playback request
  -> MediaSession
  -> Streaming Gateway
  -> MediaRoute
  -> Backend Agent
  -> explicitly owned ProviderStreamLease
  -> private media provider / VDR source
```

This document does not claim Phase-65 runtime completion. Streamdev remains a private possible provider rather than the public API/security boundary.

## Public API and client boundary

Current first-party clients use Suite-owned REST/client-wrapper semantics rather than private provider contracts. Stable independent third-party `/api/v1` compatibility remains governed by its separate roadmap/ADR boundary and must not be inferred from internal transition endpoints.

## Acceptance model

Implementation claims require the appropriate combination of:

- domain/repository/service tests;
- architecture/static guards;
- aggregate CI;
- packaging/install validation;
- real yaVDR acceptance for native runtime changes;
- real client/media acceptance where product behaviour requires it;
- Golden User Journey proof for vertical product outcomes.

Exact historical acceptance heads/hashes belong in the closeout that accepted them.

## Related documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](current-status.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Completed Phases](completed-phases.md)

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
