# ADR-0020: Multi-Source Federation Architecture

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

VDR-Suite is not intended to remain a single-VDR-only system.

The existing architecture already defines backend identity, backend federation, source capabilities, permissions, snapshots, change feeds, incremental synchronization and SSE transport.

The next architecture boundary is the representation of multiple VDR or VDR-Suite sources as one coherent system.

Future deployments may include:

- one local VDR
- one or more remote VDR systems
- multiple VDR systems in different houses
- read-only remote systems
- remote VDR-Suite instances
- archive-only sources
- mixed capability sets

The frontend must not be coupled to a single physical VDR.

---

## Decision

VDR-Suite will introduce a multi-source federation architecture based on BackendNode.

A BackendNode represents one addressable backend source managed by VDR-Suite.

A BackendNode may wrap:

- a local VDR
- a remote VDR
- a remote VDR-Suite instance
- a future plugin bridge
- a future archive source

The frontend and REST API must eventually address backend-aware resources through BackendNode identity instead of assuming a single global VDR.

---

## BackendNode

A BackendNode is the canonical runtime representation of a configured backend.

It contains at least:

```text
backendId
backendName
backendType
connection configuration
capability set
permission scope
runtime status
```

BackendNode is not the same as IVdrAdapter.

IVdrAdapter remains the backend-neutral technical adapter boundary.

BackendNode owns or references one adapter instance and adds identity, capabilities, permissions and runtime state around it.

---

## BackendId

Every BackendNode must have a stable BackendId.

BackendId must be:

- unique within one VDR-Suite installation
- stable across restarts
- independent from hostname
- independent from IP address
- independent from transport protocol
- safe for URLs and logs

Examples:

```text
home-vdr
parents-vdr
holiday-vdr
lab-vdr
```

Hostnames and IP addresses may be stored as connection configuration but must not become the canonical identity.

---

## Snapshot Ownership

Every backend-owned snapshot must eventually be associated with a BackendId.

A single global snapshot is only valid for a single-backend runtime.

The future model is:

```text
BackendNode
  BackendId
  Snapshot
  Generation
```

A federated view may aggregate multiple backend snapshots, but each contained state item must remain traceable to its origin BackendId.

---

## Change Feed Ownership

Snapshot change feed entries must eventually become backend-aware.

A change event without source ownership is not sufficient in a multi-backend system.

Future feed entries should carry:

```text
backendId
generation
changeType
domains
```

This enables clients to update only the affected backend view and prevents collisions between equal generation numbers from different backends.

---

## Capability Aggregation

Capabilities are backend-local.

A federation layer may expose an aggregate capability view, but the original backend capability set must remain available.

Example:

```text
home-vdr
  recordings.view
  recordings.modify
  timers.create
  livetv.view

parents-vdr
  recordings.view
  timers.view
```

The UI must make decisions per BackendNode, not only globally.

If one backend supports timer creation and another does not, the frontend must reflect that difference.

---

## Permission Boundary

Permissions must be evaluated per BackendNode.

Example:

```text
home-vdr
  recordings.view: allowed
  recordings.delete: allowed
  timers.create: allowed

parents-vdr
  recordings.view: allowed
  recordings.delete: denied
  timers.create: denied
```

This enables the intended use case where one VDR can be fully controlled and another remote VDR can be view-only.

Permissions and capabilities remain separate concepts.

An operation is allowed only if:

```text
backend has capability AND actor has permission
```

---

## API Direction

The initial single-backend API may continue to exist for compatibility.

The federation architecture should eventually add backend-aware API shapes such as:

```text
GET /api/backends
GET /api/backends/{backendId}/snapshot
GET /api/backends/{backendId}/changes?since={generation}
GET /api/backends/{backendId}/recordings
GET /api/backends/{backendId}/timers
```

Aggregated endpoints may later be added for frontend convenience.

Aggregated responses must preserve BackendId ownership.

---

## Frontend Principle

Frontends must not assume that there is only one VDR.

The frontend navigation model should eventually expose a source or backend selector.

Possible frontend views:

- all backends overview
- one backend detail view
- aggregated recordings view
- backend-specific timers view
- backend-specific live TV view

Actions must be scoped to a BackendId.

---

## Federation Modes

The architecture allows several future federation modes:

```text
single local backend
multiple local or remote backends
remote VDR-Suite as backend
read-only remote backend
mixed-capability backend set
```

This ADR does not require all modes to be implemented immediately.

It defines the boundary so that later support does not require a full API and snapshot redesign.

---

## Consequences

Positive:

- removes the permanent single-VDR assumption
- supports multiple houses and remote systems
- keeps identity separate from network configuration
- allows per-backend capabilities
- allows per-backend permissions
- prepares frontend architecture for source selection
- avoids later snapshot and feed rewrites

Negative:

- introduces a new BackendNode concept
- APIs must eventually become backend-aware
- snapshot and change-feed models need source ownership
- tests must cover single-backend and multi-backend behavior
- frontend complexity increases

---

## Non-Goals

This ADR does not define:

- final authentication mechanism
- user account management
- encrypted remote transport
- discovery protocol
- remote pairing protocol
- persistent backend registry schema
- final frontend UI
- media stream routing
- transcoding

---

## Implementation Direction

A future implementation phase should introduce:

- BackendId value object or type alias
- BackendNode domain object
- BackendRegistry service
- backend-aware capability lookup
- backend-aware permission evaluation boundary
- backend-aware snapshot and change-feed ownership

The first implementation may keep a single default BackendNode around the existing IVdrAdapter pipeline.

This preserves current behavior while removing the architectural single-backend assumption.

---

## Related ADRs

- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0018: Incremental Snapshot Synchronization](ADR-0018-incremental-snapshot-synchronization.md)
- [ADR-0019: SSE Event Stream Transport Strategy](ADR-0019-sse-event-stream-transport-strategy.md)
- [Historical ADR-001 Backend Identity Strategy](adr-001-backend-identity-strategy.md)
- [Historical ADR-002 Backend Federation Strategy](adr-002-backend-federation-strategy.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
