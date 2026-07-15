# ADR-0039: Backend Agent and Control Plane Boundary

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current Architecture State](../development/current-architecture-state.md)

---

## Status

Accepted

Date: 2026-07-15

---

## Context

VDR-Suite already provides an external orchestration layer, backend adapters, a backend registry, backend-scoped snapshots, change feeds and guarded domain actions.

The current runtime can connect directly to a VDR through RESTfulAPI or another adapter. That model is appropriate for a local installation, but it is not sufficient as the permanent trust and network model for multiple houses or remote sites.

A remote deployment must not require the central VDR-Suite instance to expose or directly trust:

- MariaDB access from every VDR
- SVDRP over site boundaries
- Streamdev over site boundaries
- RESTfulAPI as a public security gateway
- osd2web as a public control gateway
- arbitrary inbound access to VDR-internal plugin ports

The epgd and epg2vdr audit demonstrated the value of central coordination, but also the risks of making a shared writable database the protocol between every VDR instance.

---

## Decision

VDR-Suite will separate the central Control Plane from local Backend Agents.

```text
Clients
  -> VDR-Suite Client API
  -> Control Plane
  -> authenticated Agent Protocol
  -> Backend Agent
  -> local adapters
  -> VDR and VDR plugins
```

The Control Plane owns:

- backend registry
- user and service authorization decisions
- canonical API contracts
- snapshot and catalog aggregation
- timer intent and assignment decisions
- job orchestration
- audit records
- client-facing event delivery

The Backend Agent owns:

- local backend connectivity
- native VDR and plugin adapters
- short-lived VDR object access
- immutable snapshot production
- capability observation
- local mutation execution
- mutation verification
- local stream-provider access
- lease renewal and health reporting

The Backend Agent initiates the normal remote-site connection to the Control Plane. Remote sites should not need public inbound access to VDR-internal ports.

---

## Trust Boundary

The Agent Protocol is a distinct boundary from the public Client API.

```text
Client API
  user- and application-facing
  domain resources and user actions

Agent Protocol
  machine-facing
  backend registration, leases, snapshots, events, jobs and results
```

A client must never impersonate an agent by calling agent endpoints with a normal user session.

An agent must not automatically gain administrative access to other backends.

---

## Database Boundary

Backend Agents and frontends must not access the Control Plane database schema directly.

The database is an implementation detail of Control Plane services.

Forbidden coupling includes:

- agents writing central timer tables directly
- agents writing central recording tables directly
- clients reading metadata tables directly
- schema version equality as the only protocol compatibility mechanism

Agent and client compatibility is negotiated through versioned protocol and capability contracts.

---

## Adapter Placement

RESTfulAPI, SVDRP, plugin services, Streamdev and future native bridges remain adapter implementations.

For a remote site they run below the Backend Agent boundary:

```text
Backend Agent
├─ RESTfulAPI adapter
├─ SVDRP adapter
├─ epgsearch provider
├─ TVScraper provider
├─ Streamdev provider
└─ Legacy OSD adapter
```

Transport-specific payloads, errors, local paths and plugin object lifetimes must not escape this boundary.

---

## Local Deployment

A single-host installation may run Control Plane and Backend Agent in the same process or package set for operational simplicity.

The logical boundary remains mandatory even when deployment is co-located.

Code above the boundary must not depend on whether the agent is embedded or remote.

---

## Snapshot and Mutation Rules

Agents publish immutable observations. They do not hand out borrowed VDR pointers or plugin-owned mutable objects.

Mutating commands sent to an agent must include enough context to reject stale or misrouted execution, including:

- backend identity
- backend generation
- command identity
- expected resource revision when applicable
- idempotency key
- actor and audit context

The detailed mutation contract is defined by a separate ADR.

---

## Rules

- Frontends communicate with the Control Plane, not directly with VDR plugins.
- Remote Backend Agents use authenticated agent connections.
- Remote sites do not expose VDR-internal ports by default.
- Agents never receive direct central database credentials.
- The Control Plane does not execute native VDR operations without an adapter or agent boundary.
- Adapter results are copied into suite-owned immutable DTOs.
- One compromised backend must not automatically grant mutation rights over other backends.
- Read-only backend policy remains enforceable by the Control Plane and the executing Agent.
- Embedded and remote agents must follow the same logical contracts.

---

## Consequences

Positive:

- supports multiple houses without opening VDR plugin ports
- limits the impact of a compromised backend
- keeps the database private to the Control Plane
- allows local native execution close to VDR
- enables explicit protocol versioning and capability negotiation
- preserves the existing adapter architecture

Trade-offs:

- introduces an Agent Protocol and lifecycle
- requires secure enrollment and key rotation
- distributed operation requires offline and retry handling
- embedded deployments still need to preserve a logical separation

---

## Non-Goals

This ADR does not define:

- the final wire format
- one mandatory transport library
- the complete authentication mechanism
- job retry semantics
- timer scheduling policy
- streaming codecs or transcoding

---

## Related Decisions

- [ADR-0007: RESTfulAPI Adapter Boundary](ADR-0007-restfulapi-adapter-boundary.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0026: External Orchestration Layer Above VDR](ADR-0026-external-orchestration-layer-above-vdr.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
