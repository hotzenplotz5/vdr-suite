# ADR-0042: Safe Mutation, Revision and Idempotency Contract

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)

---

## Status

Accepted

Date: 2026-07-16

---

## Context

VDR-Suite already contains strong domain-specific mutation foundations:

- Recording actions validate before dispatch;
- backend capabilities and read-only policy gate execution;
- dry-run and preview paths exist;
- SearchTimer workflows can attach backend readback verification;
- backend-scoped identities, snapshots and change-feed sequencing exist.

These foundations are necessary but do not yet form one universal mutation contract.

Current mutating requests do not consistently carry:

- the backend generation against which the action was prepared;
- the expected resource revision;
- an idempotency key;
- a stable operation identity;
- a deterministic outcome for retries after transport interruption;
- one shared vocabulary for stale state, duplicate delivery and unknown completion.

This is especially dangerous for multi-site operation. A command may be executed by a remote Backend Agent while the acknowledgement is lost. Blind retry can then repeat a destructive action. Conversely, rejecting every retry can leave the caller unable to determine whether the first attempt succeeded.

The architecture therefore needs a common contract above individual Recording, Timer, SearchTimer, metadata and future Agent transport implementations.

---

## Decision

Every real VDR-Suite mutation must use a common mutation envelope and lifecycle.

The contract applies to:

- Recording actions;
- native Timer actions;
- SearchTimer mutations and workflow execution;
- metadata assignments and refresh commands;
- future trash, restore and purge transitions;
- future Backend Agent commands;
- any later API operation that changes authoritative state.

Dry-run and preview requests may omit execution-only fields when they cannot mutate state, but a preview intended for later confirmation must return the revision and generation values required by execution.

---

## Mutation Envelope

A mutation request carries at least:

| Field | Meaning |
| --- | --- |
| `operationId` | Stable VDR-Suite identity for this logical operation. |
| `idempotencyKey` | Caller-generated or server-issued key used to deduplicate equivalent delivery. |
| `actorId` | Authenticated user, service or system actor when identity support is available. |
| `backendId` | Stable target backend identity. |
| `backendGeneration` | Expected active backend generation. |
| `resourceType` | Domain type such as Recording, NativeTimer, SearchTimer or MetadataAssignment. |
| `resourceId` | Stable VDR-Suite resource identity. |
| `expectedRevision` | Revision the caller observed and intends to mutate. |
| `action` | Domain-level mutation name. |
| `payload` | Action-specific values. |
| `requestedAt` | Request creation time. |
| `deadline` | Optional point after which dispatch must not begin. |
| `previewToken` | Optional server-issued binding to a prior preview. |

Transport-specific paths, plugin endpoint names and native pointers are not public mutation identities.

The public contract uses domain-level identities. Adapter-specific native identity is resolved inside the backend boundary.

---

## Identity Rules

`backendId`, `resourceType` and `resourceId` together identify the mutation target.

A mutable filesystem path is not a sufficient stable resource identity.

For example, a Recording move changes its path. The operation must still refer to the same suite Recording identity before and after the move.

Native backend identity may be included in internal bindings, but must be validated against:

- the requested backend;
- the active backend generation;
- the current resource binding;
- the expected resource revision.

An Agent must never execute a command for another backend identity or obsolete generation.

---

## Revision Model

Every mutable resource exposed for real mutation must have a revision suitable for optimistic concurrency.

A revision must change whenever data relevant to the mutation decision changes.

Examples include:

- Recording path, name, deletion state or native binding;
- Timer channel, event, schedule, enabled state or backend-native identity;
- SearchTimer definition and relevant execution policy;
- metadata assignment state or provider evidence selected for replacement.

The revision may be implemented as:

- a monotonic stored revision number;
- a stable version token derived from authoritative state;
- an adapter-provided version mapped into a VDR-Suite revision.

The representation may vary internally, but the API-visible token is opaque to clients.

Before dispatch, the service must verify:

```text
requested backendGeneration == active backend generation
AND
requested expectedRevision == current resource revision
```

A mismatch is a conflict and must not be silently overwritten.

---

## Preview Binding

Preview and execution are separate operations but must be safely connected.

A mutation preview returns at least:

- backend ID;
- backend generation;
- resource ID;
- current revision;
- normalized action and payload summary;
- capability and policy decision;
- expected effects and warnings;
- optional expiry;
- optional signed or stored preview token.

Execution must either:

1. provide the exact generation and revision returned by preview; or
2. provide a preview token that resolves to those values and the normalized payload.

Execution must fail when the preview is stale, expired, belongs to another actor or backend, or does not match the submitted action.

A successful preview never reserves or locks a resource by itself.

---

## Idempotency Model

The idempotency key identifies one logical mutation within an explicit scope.

The canonical scope is:

```text
actor or trusted caller
+ backendId
+ resourceType
+ resourceId
+ action family
+ idempotencyKey
```

The server stores a fingerprint of the normalized request together with the operation record.

For a repeated key:

- same scope and same request fingerprint: return the stored operation state or result;
- same scope but different fingerprint: reject as an idempotency conflict;
- different actor or backend scope: treat as a separate key space and never disclose another actor's result.

A repeated request must not dispatch the backend mutation again merely because the previous response was lost.

Idempotency records must survive process restart for every production mutation path. In-memory deduplication alone is insufficient.

Retention must be long enough to cover client retries, queued work, Agent reconnect and operational recovery. Exact retention periods are configuration and implementation details, but destructive-operation records must not expire while their outcome can still be retried or reconciled.

---

## Operation Lifecycle

Every real mutation has a durable operation record.

The shared lifecycle vocabulary is:

| State | Meaning |
| --- | --- |
| `accepted` | Request passed envelope validation and has a durable operation record. |
| `rejected` | Request was denied before dispatch by validation, authorization, policy or capability. |
| `conflict` | Generation, revision, preview or idempotency fingerprint did not match. |
| `queued` | Operation awaits execution. |
| `dispatching` | Execution ownership has been claimed and backend dispatch is beginning. |
| `executed_unverified` | Backend executor reported success but required readback is not complete. |
| `succeeded` | Required backend execution and verification completed successfully. |
| `failed_before_dispatch` | No backend mutation was attempted. |
| `failed_verified` | Backend outcome was read back and is known not to satisfy the requested result. |
| `outcome_unknown` | Dispatch may have reached the backend but completion cannot yet be proven. |
| `cancelled` | Operation was cancelled before a point where cancellation would be unsafe. |

ADR-0043 defines job claim, retry, cancellation and saga mechanics. This ADR defines the mutation-level meanings those mechanics must preserve.

---

## Verification Rules

Executor success and verified mutation success are different facts.

A mutation declares one of these verification policies:

- `none`: only allowed for operations whose adapter contract provides an authoritative atomic result;
- `readback_required`: authoritative state must be re-read and compared;
- `event_confirmation`: an authoritative sequenced event may confirm the result;
- `reconciliation_required`: immediate confirmation is unavailable and later reconciliation owns completion.

Destructive or remote operations should default to readback or reconciliation rather than trusting transport acknowledgement alone.

Examples:

- Recording delete: verify absence or canonical trash state;
- Recording move or rename: verify the stable Recording identity now has the expected path or name;
- Timer create: verify a matching native timer binding exists exactly once;
- Timer delete: verify the bound native timer is absent;
- SearchTimer update: verify backend-native readback matches the intended normalized definition.

A backend timeout after dispatch does not automatically mean failure. The operation enters `outcome_unknown` until readback or reconciliation determines the result.

---

## Retry Rules

Automatic retry is permitted only when the operation record and idempotency contract make duplicate execution impossible or detectable.

The retry path must reuse the same:

- operation ID;
- idempotency key;
- normalized request fingerprint;
- target backend identity;
- intended resource identity.

Before redispatch after an unknown outcome, the system must first attempt verification or reconciliation.

A new resource revision does not automatically authorize retry. The original operation remains bound to its original expected revision and intended transition.

A user who intentionally wants a new mutation after a conflict must create a new operation with a new idempotency key after refreshing state.

---

## Backend Generation and Fencing

Mutations are generation-bound.

The Control Plane, service layer or Agent must reject a command when:

- the backend generation is missing where required;
- the generation is obsolete;
- the Agent lease no longer authorizes execution;
- execution ownership belongs to another valid generation;
- the resource binding was replaced during backend restart or resync.

An operation created for one backend generation is not silently replayed against a newer generation. It requires reconciliation and, where necessary, a fresh user decision.

---

## Authorization and Policy Order

The required decision order is:

```text
authenticate actor or trusted caller
-> validate mutation envelope
-> resolve backend and active generation
-> authorize actor for backend and action
-> enforce backend read-only and mutation policy
-> check capability availability
-> resolve stable resource identity
-> compare expected revision
-> check idempotency record and request fingerprint
-> create or load durable operation
-> dispatch
-> verify or reconcile
-> persist final outcome
-> emit audit and security events
```

A frontend-hidden action is not a security control.

Backend read-only policy remains a hard server-side denial boundary.

---

## Error Contract Requirements

ADR-0048 defines the final public API error schema. Mutation implementations must preserve these semantic categories:

| Category | Example meaning |
| --- | --- |
| `validation_error` | Required field or action payload is invalid. |
| `unauthorized` | No valid actor or trusted caller identity. |
| `forbidden` | Actor lacks permission or backend is read-only. |
| `capability_unavailable` | Backend cannot currently perform the action. |
| `generation_conflict` | Backend generation is stale. |
| `revision_conflict` | Resource changed after the caller observed it. |
| `preview_conflict` | Preview token or normalized request no longer matches. |
| `idempotency_conflict` | Key was reused for a different normalized request. |
| `operation_in_progress` | Equivalent operation exists and has not completed. |
| `outcome_unknown` | Dispatch may have occurred and requires reconciliation. |

Conflict responses return enough current-state metadata to support a safe refresh, but must not leak unauthorized resource details.

---

## Existing Foundation Mapping

The existing implementation is retained and becomes domain-specific evidence for this contract:

| Existing foundation | Role under ADR-0042 |
| --- | --- |
| Recording action validation | Domain payload and target validation. |
| Recording action safety service | Capability, availability, read-only and execution policy decision. |
| Recording action preview and dry-run | Basis for revision-bound preview. |
| Recording backend executor adapters | Backend-specific dispatch below the common envelope. |
| SearchTimer verified execution results | Existing executor-versus-readback distinction. |
| Backend registry and stable backend IDs | Backend scope and policy lookup. |
| Snapshot and change-feed generation | Read model input, not a substitute for resource revision. |

ADR-0042 does not discard these components. It prevents every mutation domain from inventing incompatible retry, conflict and verification semantics.

---

## Implementation Sequence

Implementation follows bounded domain slices:

1. introduce shared mutation envelope, revision and operation result types;
2. add durable operation and idempotency storage;
3. adapt Recording preview and execution first;
4. adapt native Timer actions;
5. adapt SearchTimer mutation and verified execution;
6. apply the contract to metadata writes;
7. bind Backend Agent commands to generation and operation identity;
8. expose the final versioned REST error and audit contract through ADR-0048 and ADR-0049 implementation.

No new production remote mutation path may bypass this sequence.

---

## Rules

- Real mutations use stable backend and resource identities.
- Mutable paths are not sufficient resource identities.
- Every production mutation is generation-bound where a backend runtime is involved.
- Every mutable resource supplies an opaque revision token.
- Revision mismatch blocks execution.
- Every production mutation has a durable operation ID and idempotency key.
- Same key plus same normalized request returns the existing operation.
- Same key plus different normalized request is a conflict.
- Transport acknowledgement alone is not sufficient where readback is required.
- Unknown outcome is a first-class state and is reconciled before redispatch.
- Preview and execution are bound by generation, revision and normalized request.
- Read-only backend policy is enforced server-side.
- Adapter-native details remain behind adapter boundaries.
- No frontend or Agent may manufacture a newer revision to bypass a conflict.

---

## Consequences

Positive:

- prevents silent lost updates;
- makes double clicks, proxy retries and client retries safe;
- supports deterministic recovery after timeout and process restart;
- protects remote multi-site mutation from obsolete Agent generations;
- unifies Recording, Timer, SearchTimer and metadata mutation semantics;
- makes readback verification and reconciliation explicit;
- provides the prerequisite for durable jobs, audit and public API hardening.

Trade-offs:

- mutation requests and responses become more explicit;
- durable operation and idempotency storage is required;
- resource revisions must be introduced domain by domain;
- adapters need authoritative readback or reconciliation support;
- old mutation endpoints require compatibility migration rather than silent semantic change.

---

## Non-Goals

This ADR does not define:

- the final SQL schema for operations and idempotency records;
- job worker claim and retry scheduling details, which belong to ADR-0043;
- final user and role storage, which belongs to Phase 62;
- final audit event schema, which belongs to ADR-0049;
- final versioned REST error body, which belongs to ADR-0048;
- cross-backend transaction atomicity;
- distributed filesystem locking;
- one mandatory hash or revision encoding algorithm;
- implementation of all domain migrations in this documentation change.

---

## Related Decisions

- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0014: Recording Identity Strategy](ADR-0014-recording-identity-strategy.md)
- [ADR-0015: Timer Operation Boundary](ADR-0015-timer-operation-boundary.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0018: Incremental Snapshot Synchronization](ADR-0018-incremental-snapshot-synchronization.md)
- [ADR-0024: Recording Action Transport Mapping](ADR-0024-recording-action-transport-mapping.md)
- [ADR-0029: Backend-Neutral SearchTimer Architecture](ADR-0029-backend-neutral-searchtimer-architecture.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Back to Current State](../CURRENT.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
