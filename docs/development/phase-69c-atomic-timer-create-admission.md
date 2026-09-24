# Phase 69.C — Atomic Timer CREATE Admission Foundation

## Status

**ACTIVE — bounded prerequisite after accepted PR #331.**

Baseline:

```text
main=ac7807c0fce364ab5c918dfa5d1f28427e928db3
PR #331 / CI #9126=ACCEPTED
69.C=ACTIVE
```

Binding architecture:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0044](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [TimerAssignment Fulfillment State](phase-64-timer-assignment-fulfillment.md)
- [Shared Mutation Operation Repository](phase-64-mutation-operation-repository.md)
- [Native Timer CREATE Operation Preparation](phase-64-native-timer-create-operation-preparation.md)
- [Timer CREATE Suite-Owned Identity Authority](phase-69c-timer-create-identity-authority.md)

## Proven crash gap

After PR #331 every individual authority required by public Timer CREATE exists,
but the first proposed runtime sequence still had an unsafe durability gap:

```text
TimerAssignment selected
-> beginProvisioning() commits provisioning
-> process crash
-> prepare() never reserves MutationOperation + immutable payload
```

A retry would still present the original public resource revision and
Idempotency-Key, but the assignment revision had already advanced while no
durable operation/idempotency record existed.

That is not an acceptable ADR-0042 production mutation boundary. A public route
must not make its own state change destroy the information required to
recognize an exact response-loss retry.

## Decision

The first CREATE admission commit is atomic:

```text
BEGIN IMMEDIATE
  recheck durable idempotency scope
  reload exact selected TimerAssignment
  issue Suite operationId + pre-reserved bindingId
  accepted TimerAssignmentFulfillmentService:
      selected -> provisioning
  accepted NativeTimerCreateOperationPreparationService:
      reserve MutationOperation + immutable CREATE payload
COMMIT
```

If any step fails, the outer owner rolls the transaction back.

There is therefore no durable state with a newly entered public-CREATE
`provisioning` transition but no corresponding durable ADR-0042 operation and
payload.

## No second authority

This slice introduces no new lifecycle table and no replacement repository.

The existing owners remain authoritative:

- `TimerAssignmentRepository` owns assignment revision/persistence;
- `TimerAssignmentFulfillmentService` owns `selected -> provisioning`;
- `MutationOperationRepository` owns ADR-0042 operation/idempotency lifecycle;
- `NativeTimerCreateOperationPreparationService` owns the immutable CREATE preparation contract.

Each existing repository gains an explicit transaction-participating entry
point. Its normal entry point keeps the existing standalone
`BEGIN IMMEDIATE / COMMIT / ROLLBACK` behavior.

The transaction-participating entry points require an already-active SQLite
transaction and do not commit or roll it back.

## Admission owner

`NativeTimerCreateAdmissionService` is the bounded coordinator for this first
durable boundary.

Its request contains authenticated actor identity, Idempotency-Key,
TimerAssignment identity, exact pre-mutation assignment revision, explicit
expected backend identity, request timestamp and optional deadline.

It does not accept caller-generated operation or NativeTimerBinding identities.
For a new logical request it issues those identities through the accepted
PR-331 domain issuers only after the exact assignment has been reloaded inside
the write transaction.

## Durable request fingerprint

The admission service owns a deterministic length-prefixed fingerprint for the
currently empty CREATE submission semantics:

```text
native-timer-create-admission/1|
  backendId
  timerAssignmentId
  expected pre-mutation assignmentRevision
```

The actor/backend/resource/action/Idempotency-Key tuple remains the durable
ADR-0042 idempotency scope. The fingerprint additionally binds the exact
submitted precondition.

Changing the precondition while reusing the same key is therefore an
idempotency conflict rather than a new operation.

## Replay order

Replay lookup happens before current TimerAssignment precondition evaluation.

After a successful first submission the assignment is already `provisioning`
at the successor revision. An exact retry still carries the original
`If-Match`. If durable idempotency scope and fingerprint match, the service
returns the existing operation/payload and does not create new identities,
change the assignment again or redispatch anything.

The idempotency scope is rechecked again under `BEGIN IMMEDIATE` before any
write, closing a concurrent first-submission race.

## Failure injection

The focused regression installs a SQLite trigger that aborts
`mutation_operations` INSERT after the fulfillment owner has performed its
assignment UPDATE in the surrounding transaction.

It proves that after the forced failure:

- TimerAssignment remains `selected`;
- assignmentRevision remains the original revision;
- no operation exists for the Idempotency-Key.

After removing the fault, the same request succeeds normally.

## Runtime composition

The daemon owns exactly one dormant `NativeTimerCreateAdmissionService`,
composed from the shared Database and the existing assignment, operation,
fulfillment and preparation owners. It is destroyed before the services and
repositories it references.

## Still closed

This slice does **not** add or invoke:

- Public TimerAssignment POST;
- public `timers.create` admission;
- `If-Match` or `Idempotency-Key` HTTP parsing;
- Agent CREATE reservation;
- dispatch claim;
- command activation;
- SuiteBridge/VDR mutation.

No Agent command can become pollable from this slice.

## Acceptance

CI/build/architecture validation is sufficient because native execution remains closed.

The focused test/guard proves standalone repository transaction behavior remains
present, transaction-participating variants require an active outer transaction,
accepted fulfillment/preparation owners are reused, exact replay precedes
stale-current-state checks, changed fingerprint conflicts, forced operation
persistence failure rolls back assignment provisioning, exactly one dormant
admission owner is composed, and PublicApiRuntime/SecurityHttpGate remain closed.

## Next bounded slice

Once this atomic admission boundary is accepted, the next slice can open the
public HTTP admission surface without creating the crash window above:

```text
authentication + backend-scoped timers.create
-> backend write policy
-> strong If-Match
-> required bounded Idempotency-Key
-> closed JSON request
-> atomic NativeTimerCreateAdmissionService
-> durable operation response
```

Agent reservation/dispatch activation remains a separate boundary unless that
candidate also carries the required exact runtime and real-yaVDR acceptance.
