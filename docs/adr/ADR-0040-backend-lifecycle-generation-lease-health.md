# ADR-0040: Backend Lifecycle, Generation, Lease and Health

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

The current `BackendNode` model already carries stable backend identity, connection configuration, capabilities, an access mode and simple `enabled` and `online` flags.

A Boolean online flag is not sufficient for remote or multi-site operation.

It cannot distinguish:

- a backend that is intentionally disabled
- a backend that is starting
- a backend whose last poll failed
- a disconnected agent
- a stale process instance after restart
- a partially available backend
- a backend whose lease expired
- a backend that is reconnecting

Distributed commands and snapshots also require protection against delayed messages from an older agent process.

---

## Decision

Every configured backend has a stable `BackendId` and a lifecycle record owned by the Control Plane.

Each new Agent runtime for that backend receives a new `backendGeneration`.

```text
Backend identity
  backendId          stable configuration identity

Agent runtime
  backendGeneration  changes for every accepted agent runtime
  agentInstanceId    identifies the concrete process instance
  leaseId            identifies the current lease
```

Snapshots, events, commands and results must carry the relevant backend identity and generation.

---

## Lifecycle States

The canonical lifecycle vocabulary is:

```text
disabled
connecting
online
degraded
reconnecting
offline
failed
```

Meaning:

- `disabled`: configuration exists but operation is intentionally disabled
- `connecting`: an agent or direct adapter is establishing the initial connection
- `online`: lease is valid and required health checks succeed
- `degraded`: lease is valid but one or more advertised domains or transports are unavailable
- `reconnecting`: a previously connected backend is attempting recovery
- `offline`: no valid lease or active connection exists
- `failed`: operator intervention is required or retry policy is exhausted

Lease expiry normally transitions the runtime to `offline` or `reconnecting`; it is recorded as a transition reason rather than a permanent additional state.

---

## Lease Model

A remote or embedded Agent must periodically renew a lease.

The lifecycle record includes at least:

```text
backendId
backendGeneration
agentInstanceId
leaseId
leaseIssuedAt
leaseRenewedAt
leaseExpiresAt
lastHeartbeatAt
lastSuccessfulSnapshotAt
lastSuccessfulMutationAt
state
stateReason
healthSummary
```

A backend is not considered online solely because a previous database row or in-memory Boolean says so.

The Control Plane considers an Agent authoritative only while its lease and generation are current.

---

## Fencing

Mutating commands must carry `backendGeneration`.

An Agent rejects commands when:

- the backend ID does not match
- the generation does not match its current accepted generation
- the command lease or deadline expired
- the command was already completed under the same idempotency key

The Control Plane rejects late results from obsolete generations.

This provides a fencing boundary against delayed messages from a restarted or replaced Agent.

---

## Snapshot and Event Ownership

A snapshot belongs to:

```text
backendId
backendGeneration
snapshotRevision
```

An event belongs to:

```text
backendId
backendGeneration
eventSequence
```

The following concepts must remain distinct:

- `backendGeneration`: Agent process generation
- `snapshotRevision`: revision of observed backend state
- `eventSequence`: ordering inside one backend generation
- `resourceRevision`: concurrency token for one resource

A client or Control Plane consumer must request a full resynchronization when it cannot safely map a cursor to the current generation and retained event window.

---

## Health Model

Health is more detailed than lifecycle state.

A backend health report may include domain results such as:

```text
adapter connection
channels read
EPG read
recordings read
recordings cache
Timer read/write
SearchTimer provider
metadata provider
stream provider
legacy OSD bridge
storage availability
```

Health values should use a bounded vocabulary such as:

```text
healthy
degraded
unavailable
unknown
```

A backend may remain `degraded` and usable for read-only EPG even when recording mutations are unavailable.

Capabilities and health remain separate:

- capability describes what the backend can support
- health describes whether that capability is currently operational

---

## Capability and Lease Interaction

Capabilities are associated with a backend generation and capability revision.

When a lease expires:

- cached capability descriptions may remain visible for diagnostics
- operations requiring live execution are unavailable
- frontend write hints must not imply current executability
- stale capabilities must be marked unavailable or unknown until refreshed

---

## Transition Rules

Lifecycle transitions are explicit events.

Examples:

```text
disabled -> connecting
connecting -> online
online -> degraded
online -> reconnecting
reconnecting -> online
reconnecting -> offline
reconnecting -> failed
any active state -> disabled
```

Transitions record:

- previous state
- new state
- reason
- timestamp
- backend generation
- relevant error category

---

## Rules

- Backend IDs remain stable across restarts.
- Backend generations change for new accepted Agent runtimes.
- Only the current generation may execute new commands.
- Online state requires a valid lease.
- Snapshot revision and backend generation are different concepts.
- Health and capabilities are different concepts.
- Cached data may remain readable while a backend is offline, but staleness must be explicit.
- Lifecycle transitions must be observable and auditable.
- A slow or failed backend must not block unrelated backends.

---

## Consequences

Positive:

- eliminates ambiguous online Booleans
- prevents stale Agent processes from executing current commands
- supports reliable multi-site reconnect behavior
- gives frontends precise stale and degraded states
- makes snapshot and event ownership explicit
- supports narrow per-domain degradation

Trade-offs:

- requires lease renewal and expiry processing
- introduces more runtime state and tests
- clients must distinguish cached availability from live executability
- direct local adapters need an embedded lifecycle implementation

---

## Superseded Historical Decision

This ADR supersedes the lifecycle direction recorded in the historical `ADR-004 Backend Lifecycle Strategy` while retaining its core state-machine intent.

---

## Non-Goals

This ADR does not define:

- exact heartbeat intervals
- one mandatory clock implementation
- the final persistent schema
- authentication credentials
- job retry policy
- client UI wording

---

## Related Decisions

- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0018: Incremental Snapshot Synchronization](ADR-0018-incremental-snapshot-synchronization.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
