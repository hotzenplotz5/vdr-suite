# ADR-0016: Snapshot Change Feed Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

Status: Accepted

## Context

The snapshot architecture already provides:

- VdrChangeState
- VdrChangeEvent
- ChangeDetectionService
- SnapshotRefreshPlanner
- SnapshotCache
- SnapshotAccessService
- VdrSnapshotReadService

Future frontends require a way to determine which snapshot domains changed without repeatedly reloading every snapshot endpoint.

Future live transports may include SSE or WebSocket implementations.

## Decision

Introduce a dedicated snapshot change feed architecture layer.

The change feed is a read-side model.

It is generated from domain-level change information and remains independent from transport.

Conceptually:

VdrChangeState
-> ChangeDetectionService
-> VdrChangeEvent
-> SnapshotRefreshPlanner
-> Snapshot Change Feed
-> Future REST/SSE/WebSocket consumers

The feed does not own snapshot data.

Snapshot ownership remains within SnapshotCache and SnapshotAccessService.

## Consequences

Positive:

- transport-independent architecture
- reusable by REST, SSE and WebSocket consumers
- avoids frontend coupling to polling internals
- compatible with future multi-VDR architecture
- preserves existing snapshot read APIs

Negative:

- introduces an additional read-side model
- feed retention strategy must be defined later
- sequence and generation strategy must be defined later

## Follow-Up

Future implementation phases may introduce:

- SnapshotChangeFeedEntry
- SnapshotChangeFeed
- SnapshotChangeFeedService
- read-only REST feed endpoint

Transport implementations remain separate architecture phases.
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
