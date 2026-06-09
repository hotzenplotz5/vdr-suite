# ADR-0018: Incremental Snapshot Synchronization

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current Project Status](../development/current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Status

Accepted

---

## Context

VDR-Suite now has a daemon-owned snapshot cache and a snapshot change feed.

ADR-0017 defines that live transport consumes the snapshot change feed and must not own polling, snapshot generation or backend-specific event parsing.

Future clients need an efficient way to synchronize state after reconnects, refresh cycles or transport interruptions.

A client should not always reload the full snapshot when it already knows an older snapshot generation.

---

## Decision

VDR-Suite will use generation-based incremental snapshot synchronization.

A client may provide the last generation it has seen.

The backend may answer with all change feed entries newer than that generation when they are still available.

The minimal synchronization model is:

```text
client generation
server generation
available feed entries
incremental update or full snapshot required
```

The first implementation should stay transport-independent.

It should work for:

- REST polling
- Server Sent Events
- WebSocket
- future multi-VDR federation

---

## Cursor Model

A cursor identifies the last known snapshot generation of a client.

The initial cursor model should be simple:

```text
ChangeFeedCursor
  generation
```

Later versions may include:

- backend identity
- source identity
- client identity
- timestamp
- feed window marker

These later additions must not be required for the first implementation.

---

## Incremental Update Model

An incremental update describes what the backend can provide for a client cursor.

The initial model should support:

- from generation
- to generation
- list of changed domains
- list of change feed entries
- full snapshot required flag

The full snapshot required flag is important for cases where incremental replay is no longer safe.

---

## Gap Handling

A full snapshot is required when:

- the client cursor is older than the retained change feed window
- the backend has restarted and lost in-memory feed history
- the cursor generation is invalid
- the requested backend identity does not match the feed source
- future federation cannot map the cursor safely

When a full snapshot is required, the incremental response must make this explicit.

The client should then reload the snapshot and continue from the new generation.

---

## Architecture Rules

- incremental synchronization consumes SnapshotChangeFeed
- incremental synchronization does not perform polling
- incremental synchronization does not parse RESTfulAPI responses
- incremental synchronization does not mutate snapshots
- incremental synchronization must be transport-independent
- transport layers may expose incremental synchronization but must not own it

---

## Consequences

Positive:

- clients can reconnect efficiently
- REST polling, SSE and WebSocket can share one synchronization model
- future frontends avoid unnecessary full snapshot reloads
- future federation can add backend identity to cursors later

Negative:

- feed retention policy becomes architecturally relevant
- clients must handle full snapshot required responses
- future multi-backend cursors need careful design

---

## Non-Goals

This ADR does not define:

- final REST endpoint shape
- final SSE protocol shape
- WebSocket message format
- persistent feed storage
- authentication
- authorization
- media streaming
- preview streaming

---

## Related ADRs

- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0007: RESTfulAPI Adapter Boundary](ADR-0007-restfulapi-adapter-boundary.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
