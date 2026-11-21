# Phase 64 — Shared Mutation Operation Repository

## Status

Phase-64 shared Control-Plane persistence used by the guarded Timer mutation work.

This boundary closes a prerequisite that became explicit while preparing the
Native Timer readback-to-operation handoff: the repository has the accepted
ADR-0042 lifecycle contract and Phase-63 protected-write/idempotency decision
contract, but **no existing durable generic operation repository** that can own
that lifecycle across domains and process restarts.

Creating a Timer-local operation table would establish a second lifecycle
authority and violate ADR-0042. The shared persistence boundary therefore owns
both the operation lifecycle and, when required, one immutable versioned
handler payload reserved atomically with that operation.

## Governing contract

[ADR-0042 — Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
requires one durable operation record and a stable idempotency scope for every
real mutation. Its shared lifecycle vocabulary is retained exactly:

```text
accepted
rejected
conflict
queued
dispatching
executed_unverified
succeeded
failed_before_dispatch
failed_verified
outcome_unknown
cancelled
```

The verification policies are likewise shared:

```text
none
readback_required
event_confirmation
reconciliation_required
```

## Single lifecycle authority

The backend-neutral, transport-neutral `vdrsuite::operations::MutationOperation`
and durable `MutationOperationRepository` remain the **Single lifecycle authority**
for ADR-0042 mutation state.

The stored immutable operation envelope contains:

- stable `operationId`;
- stable `idempotencyKey`;
- actor identity;
- backend ID and exact backend generation;
- resource type and stable Suite resource ID;
- expected resource revision and optional resource fingerprint;
- action family;
- normalized request fingerprint;
- requested time and optional dispatch deadline;
- verification policy.

Mutable operation evidence is limited to:

- repository-owned monotonic `operationRevision`;
- ADR-0042 lifecycle state;
- bounded durable result/evidence reference;
- `updatedAt`.

No Timer-specific operation table is introduced.

## Durable idempotency scope

The unique durable scope follows ADR-0042:

```text
actorId
+ backendId
+ resourceType
+ resourceId
+ actionFamily
+ idempotencyKey
```

`reserve()` creates an `accepted` operation and owns initial revision `1`.
An exact replay of the same logical operation returns the existing record and
performs no second insert. Reusing the same scope with another operation or
request fingerprint is an idempotency conflict. Reusing an operation ID for a
different logical operation is an operation conflict.

The request fingerprint is immutable; a later lifecycle transition never edits
it or any scope/resource/generation field.

## Atomic immutable payload reservation

Some mutations can reconstruct their dispatch handoff entirely from durable
resource state. Others, such as native Timer CREATE, need pre-dispatch values
which do not yet exist as a durable post-mutation resource. For those cases the
repository provides `reserveWithPayload()`.

The payload envelope contains:

- the exact `operationId`;
- stable `payloadType`;
- positive `payloadVersion`;
- immutable normalized serialized `payload`;
- immutable `payloadFingerprint`.

The operation row and payload row are inserted inside the **same `BEGIN
IMMEDIATE` SQLite transaction**. There is therefore no accepted-operation state
from which the corresponding required CREATE handoff has not yet been made
durable.

Exact idempotent replay requires both the logical operation and the complete
payload envelope to match. A missing payload, changed version, changed bytes or
changed payload fingerprint fails closed as an operation conflict. A payload
cannot be attached later to an operation previously reserved through the
payload-free API; the caller must have chosen the atomic reservation path before
acceptance.

The repository treats payload bytes as opaque versioned handler input. It does
not know about Timer, Recording, Agent or VDR domain types. Domain code owns
serialization, version compatibility and semantic validation.

Existing payload-free `reserve()` users remain valid and unchanged.

## Revision-fenced lifecycle transitions

`transition()` requires:

- exact operation ID;
- exact current `operationRevision`;
- exact expected lifecycle state;
- a transition admitted by the central ADR-0042 state graph;
- non-regressing update time;
- a bounded result/evidence reference.

A successful transition increments `operationRevision` exactly once. An exact
replay of an already-applied state/result transition returns the durable record
without creating a new revision. Stale revisions and impossible state jumps
fail closed.

Terminal states do not transition further.

## Relationship to ADR-0043

ADR-0043 defines a future production job model with versioned immutable payloads,
atomic claims, attempts, claim fencing, leases, retries and sagas. The current
legacy Recording `JobRepository` does not implement that contract and is not
promoted into Timer execution.

The immutable operation payload added here is deliberately narrower: it closes
the pre-dispatch crash gap for mutation-specific durable input while preserving
ADR-0042 as the operation lifecycle authority. It is not represented as a full
ADR-0043 job implementation and does not invent claim/attempt semantics.

A later production job layer may reference the same `operationId` and copy or
reference the versioned payload under ADR-0043 rules without changing the
operation's caller-visible identity.

## Relationship to Phase 63

Phase 63 already provides important lower-level pieces:

- durable Agent command assignment/result persistence;
- `operationId` correlation on commands;
- protected-write request and idempotency decision contracts;
- accountability evidence;
- provider/generation fencing.

Those are not replaced. In particular, Agent command persistence remains the
remote command transport/result owner. It begins only after Control-Plane
pre-dispatch state is durable.

## Scope boundary

This shared repository remains domain-neutral. It adds:

- no Native Timer dependency in `core/operations`;
- no Agent command creation or dispatch;
- no native Timer create/update/delete execution;
- no provider or SuiteBridge changes;
- no assignment/failover transition;
- no public mutation API;
- no broad Timer UI;
- no daemon/runtime wiring;
- no `mutations=enabled` switch.

The focused repository regression and normal repository CI remain the acceptance
boundary for this persistence change.
