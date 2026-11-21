# VDR Ecosystem Parity and Product Gap Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Golden User Journeys](golden-user-journeys.md)

---

## Purpose

This document compares VDR-Suite with VDR Core, Live, epgsearch and RESTfulAPI at the product/architecture level.

It intentionally contains no active PR, exact branch head, CI checkpoint or duplicated current phase snapshot. Exact operational state belongs only in [Current State](../CURRENT.md).

## Core comparison principle

VDR-Suite does not try to copy every reference surface one-for-one. It preserves VDR as the native runtime authority while adding stronger Suite-owned identity, policy, orchestration, reconciliation and multi-client boundaries.

```text
VDR native authority
  + Suite identity / policy / orchestration
  + Backend Agent / explicit provider ownership
  + stable client-facing Suite semantics
```

Private RESTfulAPI, SVDRP, Streamdev, TVScraper and SuiteBridge interfaces remain implementation details rather than public application contracts.

## VDR Core

VDR remains authoritative for devices, schedules, native timers, recordings, replay, OSD and plugin execution.

VDR-Suite is strong in backend-aware channels/EPG, Recordings 2, metadata, search, guarded Recording actions, remote-control foundations, persistent identity/RBAC/accountability and secure Backend Agent boundaries.

The Timer platform adds the explicit Suite model:

```text
TimerIntent -> TimerAssignment -> NativeTimerBinding
```

A native VDR Timer is therefore execution state/evidence rather than the owner of the user's global Suite intent.

## Live

Live remains a useful product reference for mature single-VDR workflows, especially playback, Timer/SearchTimer UX and compatibility surfaces.

VDR-Suite intentionally differs in several areas:

| Area | VDR-Suite direction |
| --- | --- |
| Multi-backend | Explicit backend identity, generation and access policy. |
| Security | Persistent users/sessions, backend-scoped authorization and accountability. |
| Mutations | Fenced execution, unknown-outcome handling and authoritative readback. |
| Metadata | Provider-neutral persisted Suite read models. |
| Timer ownership | Backend-neutral intent and deterministic assignments. |
| Streaming | Authenticated MediaSession/Gateway; private StreamProvider. |
| OSD | Separate compatibility plane rather than primary UI architecture. |

VDR-Suite should not claim complete Live replacement until the corresponding Timer, media and OSD user journeys have real acceptance.

## epgsearch

SearchTimer/epgsearch are important automation and behaviour references, but they do not own the future cross-backend scheduler.

Suite rules:

- SearchTimer/epgsearch may create proposals or TimerIntents;
- central authorization and assignment policy remain Suite-owned;
- duplicate/repeat/failover semantics must map to explicit Timer intent/assignment rules;
- unresolved native write outcomes block unsafe replacement or retry;
- edge semantics require regression and real-system evidence rather than endpoint-name parity alone.

## RESTfulAPI

RESTfulAPI remains a private backend adapter.

Suite clients should consume normalized Suite resources and errors, not RESTfulAPI response shapes or endpoints. This applies equally to channels, EPG, recordings, timers, remote actions and later media/OSD behaviour.

Exact parity with every RESTfulAPI endpoint is neither required nor automatically desirable.

## Provider boundary

TVScraper, SuiteBridge, Streamdev and future providers follow the same rule:

```text
provider/native fact
  -> bounded adapter/provider contract
  -> Suite identity / evidence / policy
  -> Suite-owned public behaviour
```

Provider reachability does not grant authority. An active operation or media route must not silently change provider based only on availability or priority.

## Durable product strengths

- backend-aware channels and EPG;
- Recordings 2 with metadata, people, artwork and Genres;
- guarded Recording maintenance;
- backend-scoped search;
- SearchTimer foundations;
- backend-neutral remote/live-state foundations;
- persistent identity, RBAC and accountability;
- secure Backend Agent lifecycle and multi-site fencing;
- durable protected-write safety;
- explicit Timer intent/assignment/native-binding architecture.

## Named remaining product domains

The roadmap still contains distinct completion domains for:

- reliable end-to-end Timer orchestration;
- authenticated Live and Recording playback through MediaSession/Streaming Gateway;
- Legacy OSD compatibility where product value requires it;
- stable independent-client public API compatibility;
- selected specialist epgsearch/Live semantics;
- optional later recommendation/content-graph behaviour;
- shared/cross-site Recording storage semantics if required.

This file does not authorize a successor implementation slice. Exact current authorization belongs only in [Current State](../CURRENT.md).

## Golden parity gates

### Live TV

```text
channel / EPG selection
  -> authorized MediaSession
  -> selected compatible delivery profile
  -> picture and sound
  -> channel change
  -> deterministic old route/provider cleanup
```

### Recording playback

```text
Recording detail
  -> authorized playback
  -> play
  -> seek where supported
  -> stop
  -> later durable Suite-owned resume state
```

Growing recordings must report changing length/seek capability truthfully.

### Recording creation

```text
EPG programme
  -> TimerIntent
  -> TimerAssignment
  -> managed NativeTimerBinding
  -> authoritative native VDR Timer readback
  -> recording result
```

### Failure

```text
backend / provider / transport failure
  -> classified Suite-visible state
  -> no blind duplicate mutation
  -> no silent provider switch
  -> reconciliation only when evidence permits
  -> understandable user/operator outcome
```

See [Golden User Journeys](golden-user-journeys.md).

## Media parity rule

Streamdev can be an internal explicitly owned provider, but it is not the public VDR-Suite streaming architecture.

The preferred transformation order for a valid MediaSession is:

```text
pass-through -> remux/repackage -> transcode
```

This is a media-transformation preference, not a provider-fallback chain.

## Readiness rule

Architecture, endpoint inventory and CI are necessary but not sufficient for product parity. A feature is ready only when its Suite-owned contract, failure semantics, required native/client integration and relevant vertical acceptance are all proven.

## Status separation

- volatile current state -> [Current State](../CURRENT.md)
- binding phase order -> [Strict Roadmap](roadmap.md)
- stable architecture -> [Target Platform Architecture](../architecture/target-platform-architecture.md)
- historical proof -> completed development closeouts
- user-visible proof -> [Golden User Journeys](golden-user-journeys.md)

## Back

- [Back to Planning Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
