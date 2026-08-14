# VDR-Suite Project Overview

## Navigation

- [README](../README.md)
- [Current State](CURRENT.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)

---

## Purpose

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. It complements VDR instead of forking or replacing VDR's native runtime responsibilities.

This overview intentionally contains no active PR number, exact branch head or CI checkpoint. Those volatile facts belong only in [Current State](CURRENT.md).

## Platform ownership

```text
VDR
  remains native authority for devices, schedules, timers,
  recordings, replay, OSD and plugin execution

VDR-Suite Control Plane
  owns Suite identity, authorization, policy, orchestration,
  durable read models, reconciliation and client-facing semantics

Backend Agent
  owns bounded site-local observation, command delivery,
  provider ownership/selection, fencing and cleanup

Private adapters/providers
  RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge and
  future native/media providers remain implementation details
```

Clients consume Suite-owned contracts. A reachable private provider never becomes the public security or compatibility boundary merely because it can supply data or bytes.

## Durable platform foundations

The accepted platform direction includes:

- daemon-owned SQLite with repository-owned SQL and explicit persistence boundaries;
- backend identity, backend scope and server-enforced access policy;
- channels, EPG, recordings, metadata, people, artwork, Genres and search read models;
- Recordings 2 as the delivered Recording browsing/detail owner;
- persistent actor identity, browser-session security, exact backend-scoped authorization and append-only accountability;
- Backend Agent enrollment, lifecycle/generation fencing, observations, durable command/result handling and explicit local provider ownership;
- protected-write safety built around idempotency, leases/fences, expected revisions, unknown-outcome handling and authoritative readback;
- backend-neutral TimerIntent, TimerAssignment and NativeTimerBinding architecture;
- a server-side Streaming Gateway / MediaSession target that keeps Streamdev and other media providers private;
- modular first-party clients behind `VdrSuiteClientApi` and later stable public API contracts.

Exact implementation progress within the currently active phase is recorded only in [Current State](CURRENT.md).

## Product direction

The platform is intended to make these user-visible flows backend-neutral:

- choose a channel or programme and watch Live TV;
- browse a Recording, play it, seek where supported and resume through Suite-owned progress state;
- choose an EPG programme and create one recording intent without selecting a private VDR provider;
- allow VDR-Suite to select an eligible backend deterministically in a multi-backend installation;
- surface backend/provider failures as understandable Suite states rather than leaking provider errors or performing unsafe hidden retries.

These vertical acceptance paths are defined in [Golden User Journeys](planning/golden-user-journeys.md).

## Key architectural rules

- VDR remains authoritative for VDR-native runtime state and execution.
- Suite IDs and backend-native IDs are never interchangeable.
- Authorization and capability are separate decisions.
- Provider reachability does not grant provider authority.
- A possible mutation dispatch followed by timeout does not authorize a blind retry.
- Successful transport acknowledgement is weaker than authoritative native readback where verification is required.
- Browser, TV and native clients must not construct private RESTfulAPI, SVDRP, Streamdev or SuiteBridge URLs as application contracts.
- Stable architecture and historical evidence must not duplicate volatile current-state markers.

## Execution order

The binding numbered phase sequence and phase completion gates live in the [Strict Roadmap](planning/roadmap.md). The current completed/active/next phase position lives in [Current State](CURRENT.md).

This separation is deliberate: changing day-to-day repository state must not require synchronized edits across product overview, architecture, indexes and historical closeouts.

## Authoritative navigation

- [Current State](CURRENT.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Completed History](development/completed-phases.md)
- [Architecture Decision Records](adr/index.md)

## Back

- [Back to Documentation Index](index.md)
- [Back to README](../README.md)
- [Back to Current State](CURRENT.md)
