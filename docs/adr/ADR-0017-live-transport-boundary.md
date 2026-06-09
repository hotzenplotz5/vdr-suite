# ADR-0017: Live Transport Boundary

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

VDR-Suite now has a daemon-owned snapshot and change feed architecture.

The current implementation includes:

- snapshot cache
- snapshot access
- snapshot read APIs
- snapshot change feed
- runtime feed update integration
- snapshot cache generation tracking

The next phase introduces live update transport for future clients.

Candidate transports include:

- Server Sent Events
- WebSocket
- long polling fallback

A transport layer must not become the owner of polling, change detection or snapshot generation.

---

## Decision

Live transport is a consumer of the snapshot change feed.

Live transport must not own:

- VDR polling
- backend-specific event parsing
- change detection
- snapshot creation
- snapshot cache generation
- snapshot change feed generation

The authoritative data flow remains:

```text
VDR backend
VDR adapter layer
PollingService and change detection
SnapshotCacheService
SnapshotChangeFeedService
SnapshotChangeFeed
Live transport
Client
```

Server Sent Events may be implemented first because it is simple, HTTP-friendly and suitable for one-way event delivery.

WebSocket may be implemented later without changing the domain or snapshot architecture.

---

## Consequences

Positive:

- live updates stay backend-neutral
- RESTfulAPI-specific behavior remains inside the adapter layer
- frontends do not depend on PollingService internals
- SSE can be introduced without locking the project into a WebSocket design
- WebSocket can later consume the same feed boundary

Negative:

- the live transport layer must be introduced as an additional abstraction
- SSE implementation must avoid leaking HTTP connection details into VDR domain objects
- a transport fan-out strategy may be needed later

---

## Non-Goals

This ADR does not define:

- final SSE endpoint shape
- final WebSocket protocol
- authentication
- authorization
- multi-client fan-out implementation
- media streaming
- preview streaming
- image transport

Media streams and preview streams are separate concerns from live update notification.

---

## Architecture Rules

- live transport consumes SnapshotChangeFeed
- live transport does not mutate snapshots
- live transport does not call VDR adapters directly
- live transport does not parse RESTfulAPI responses
- live transport does not perform change detection
- live transport does not own snapshot generation
- live transport must remain replaceable

---

## Related ADRs

- [ADR-0007: RESTfulAPI Adapter Boundary](ADR-0007-restfulapi-adapter-boundary.md)
- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-005 Stream Provider Strategy](adr-005-stream-provider-strategy.md)

Important distinction:

Live update transport is not media stream transport.

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
