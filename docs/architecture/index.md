# Architecture Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Target Platform Architecture](target-platform-architecture.md)
- [ADR Index](../adr/index.md)

---

## Purpose

This section documents stable VDR-Suite architecture. Exact active PRs, branch heads, CI checkpoints and the current phase tip belong only in [Current State](../CURRENT.md).

A target diagram or accepted ADR is not by itself evidence that runtime implementation is complete.

## Canonical architecture

- [Target Platform Architecture](target-platform-architecture.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [ADR Index](../adr/index.md)

## Stable ownership model

```text
clients
  -> VDR-Suite Control Plane / Suite contracts
  -> Backend Agent
  -> explicitly owned local provider
  -> VDR native runtime
```

VDR remains authoritative for VDR-native runtime state and execution. VDR-Suite owns external identity, authorization, policy, orchestration, reconciliation and client-facing semantics. Private adapters/providers remain behind Suite boundaries.

## Earlier designs and historical context

The following material is retained for traceability, **not** as an alternative
current component inventory:

- [Early component topology](suite-components.md) — proposed clients/plugins
  and adapters are not proof of installed implementations.
- [Early snapshot-era architecture](snapshot-architecture.md) — Phase-8
  “planned” statements are historical checkpoints.
- [Early REST controller topology](rest-api-runtime.md) and [Daemon REST wiring sketch](daemon-rest-runtime.md) — not exhaustive current HTTP/API diagrams.
- [Adapter-era VDR backend topology](vdr-backends.md) — not the current full Agent/SuiteBridge transport picture.
- [Early OSD/federation strategy ADRs](adr/index.md) — read later numbered
  ADRs and accepted closeouts for the binding boundaries.

## Main architecture areas

### Security and mutation safety

- [Security and Identity Foundation](security-identity-foundation.md)
- [ADR-0041](../adr/ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043](../adr/ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0049](../adr/ADR-0049-audit-security-event-model.md)

### Backend Agent and VDR integration

- [VDR Backends](vdr-backends.md)
- [VDR Domain Model](vdr-domain-model.md)
- [RESTfulAPI Integration](restfulapi-integration.md)
- [Suite Bridge Agent Handshake](suite-bridge-agent-handshake.md)
- [Suite Bridge Local SVDRP Transport](suite-bridge-svdrp-transport.md)
- [SuiteBridge Control-Plane Audit](suitebridge-control-plane-audit.md)
- [Suite Bridge Observation Lifecycle](suite-bridge-observation-lifecycle.md)
- [ADR-0064 SuiteBridge Local Prioritized Control Plane](../adr/ADR-0064-suitebridge-local-prioritized-control-plane.md)
- [ADR-0039](../adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040](../adr/ADR-0040-backend-lifecycle-generation-lease-health.md)

### Timer orchestration

- [ADR-0044](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)

Stable domain order:

```text
TimerIntent -> TimerAssignment -> NativeTimerBinding
```

### Media

- [ADR-0017](../adr/ADR-0017-live-transport-boundary.md)
- [ADR-0046](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)

The Streaming Gateway / MediaSession boundary keeps public media enforcement separate from private StreamProvider implementations such as Streamdev.

### Metadata, search and recordings

- [Metadata Identity Foundation](metadata-identity-foundation.md)
- [Metadata-Backed Genre Browser](metadata-genre-browser.md)
- [Backend-Scoped Global Search](global-search.md)
- [Recording Actions Architecture](recording-actions-architecture.md)
- [Lazy Recording Loading](../planning/lazy-recording-loading.md)

Provider data remains evidence behind Suite-owned persistence and contracts.

## Status separation rule

- `docs/CURRENT.md` owns volatile operational truth.
- `docs/planning/roadmap.md` owns binding future order and phase gates.
- this directory owns stable architecture.
- completed closeouts own historical exact acceptance evidence.

Do not copy active-head or CI checkpoint data into architecture pages.

## Historical notes

- [Architecture History](history/README.md)
- [Phase-30–32 Recording Action design](recording-actions-architecture.md) — original safety model; later marks/cutting completion is separately evidenced
- [Phase-12 Snapshot Change Feed proposal](snapshot-change-feed-architecture.md) and [Phase-8.94 wiring note](phase-8.94-runtime-wiring-notes.md) — early implementations/plans, not current phase gates

Historical snapshots remain traceability only and do not override accepted ADRs or current project state.

## Back

- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
