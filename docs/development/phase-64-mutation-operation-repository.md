# Phase 64 Slice 20 — Shared Mutation Operation Repository

## Status

Bounded implementation slice stacked on Phase 64 Slice 19 / Draft PR #172.

This slice closes a prerequisite that became explicit while preparing the
Native Timer readback-to-operation handoff: the repository has the accepted
ADR-0042 lifecycle contract and Phase-63 protected-write/idempotency decision
contract, but **no existing durable generic operation repository** that can own
that lifecycle across domains and process restarts.

Creating a Timer-local operation table would establish a second lifecycle
authority and violate ADR-0042. Slice 20 therefore implements the missing shared
persistence boundary first.

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

The new `vdrsuite::operations::MutationOperation` is backend-neutral and
transport-neutral. The durable `MutationOperationRepository` is the **single
lifecycle authority** introduced by this slice.

The stored immutable operation envelope contains:

- stable `operationId`;
- stable `idempotencyKey`;
- actor identity;
- backend ID and exact backend generation;
- resource type and stable Suite resource ID;
- expected resource revision;
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

`reserve()` creates only an `accepted` operation and owns initial revision `1`.
An exact replay of the same logical operation returns the existing record and
performs no second insert. Reusing the same scope with another operation or
request fingerprint is an idempotency conflict. Reusing an operation ID for a
different logical operation is an operation conflict.

The request fingerprint is immutable; a later lifecycle transition never edits
it or any scope/resource/generation field.

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

## Relationship to Phase 63

Phase 63 already provides important lower-level pieces:

- durable Agent command assignment/result persistence;
- `operationId` correlation on commands;
- protected-write request and idempotency decision contracts;
- accountability evidence;
- provider/generation fencing.

Those are not replaced. In particular, `BackendAgentCommandRepository` remains
the command transport/result owner and `AccountabilityEventRepository` remains
the append-only accountability owner. Neither is promoted into the mutation
operation lifecycle implicitly.

The Phase-63 protected-write slice explicitly deferred SQLite idempotency
persistence; this slice supplies that missing common Control-Plane authority
without changing Agent behavior.

## Relationship to Native Timer readback

Slice 19 / PR #172 can prove and durably record the expected Native Timer delete
postcondition on `NativeTimerBinding`, including `lastVerifiedOperationId`.
It deliberately does not transition the ADR-0042 operation lifecycle.

**Slice 21** may now add the narrow correlation service that:

1. reloads the exact durable MutationOperation by operation ID;
2. requires the correct backend/resource/action identity and unresolved
   `executed_unverified` or `outcome_unknown` state;
3. requires the matching durable NativeTimerBinding verification evidence from
   Slice 19;
4. transitions this shared operation record to `succeeded` through the exact
   repository revision fence.

That later service must not infer success from transport acknowledgement or
from an uncorrelated missing Timer.

## Scope boundary

This slice intentionally adds only the shared domain model, SQLite repository,
focused regression, architecture guard and isolated Make test fragment.

It adds:

- no Native Timer dependency in `core/operations`;
- no Timer lifecycle transition service yet;
- no Agent command creation or dispatch;
- no native Timer create/update/delete execution;
- no provider or SuiteBridge changes;
- no assignment/failover transition;
- no public mutation API;
- no broad Timer UI;
- no daemon/runtime wiring;
- no `mutations=enabled` switch.

Because no installed runtime path changes, real yaVDR runtime acceptance is not
required for Slice 20. The focused repository regression and normal repository
CI are the acceptance boundary.
