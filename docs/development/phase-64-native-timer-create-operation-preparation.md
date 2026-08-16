# Phase 64 — Native Timer CREATE Operation Preparation

This component closes the pre-dispatch durability boundary for managed native
Timer CREATE. It performs **no native mutation** and does not dispatch an Agent
command.

## Why CREATE needs a durable payload

DELETE can reconstruct its immutable dispatch handoff from the durable
`MutationOperation` plus an already existing `NativeTimerBinding`. CREATE cannot:
before native creation there is no native Timer identity and no binding row yet.
The pre-reserved Suite binding identity and desired native Timer specification
must therefore survive a process crash independently of RAM.

The shared `MutationOperationRepository::reserveWithPayload()` provides the
required **atomic operation + payload reservation**. The accepted operation and
its immutable versioned CREATE payload become durable in one SQLite transaction.
There is no accepted operation with a missing CREATE handoff.

## Preparation fences

Before reservation the service reloads the current durable `TimerAssignment`
and owning `TimerIntent` and requires all of the following:

- exact current TimerIntent revision on both intent and assignment;
- a currently assignable TimerIntent state;
- exact assignment revision;
- exact repository-issued assignment epoch;
- assignment state `provisioning`;
- no existing `nativeTimerBindingId` on the assignment;
- exact selected backend ID and backend generation;
- desired specification channel equal to the assignment's durable backend
  channel binding;
- a valid pre-reserved stable `nativeTimerBindingId`;
- a valid backend-neutral `NativeTimerSpecification`.

A stale intent, assignment revision, assignment epoch, backend generation or
channel mapping fails closed before operation acceptance.

## Immutable CREATE payload

`NativeTimerCreateOperationPayload` version 1 contains only the information
needed to reconstruct the future CREATE dispatch after restart:

- TimerAssignment identity and expected assignment revision;
- expected TimerIntent revision;
- assignment epoch;
- pre-reserved NativeTimerBinding identity;
- backend ID and generation;
- complete desired `NativeTimerSpecification`.

The codec uses a deterministic length-prefixed representation. HHMM values are
canonicalized, so semantically identical `930` and `0930` inputs produce the
same serialized payload and fingerprint. The parser rejects malformed,
non-canonical or version-mismatched payloads and reserializes the decoded value
to prove canonical equality.

The generic repository stores this as payload type `native.timer.create`,
payload version `1`, immutable bytes and immutable payload fingerprint.

## ADR-0042 operation identity

CREATE targets the durable `TimerAssignment`, because the NativeTimerBinding
does not exist yet:

```text
resourceType                 = TimerAssignment
resourceId                   = timerAssignmentId
expectedRevision             = provisioning assignmentRevision
expectedResourceFingerprint  = NativeTimerSpecification fingerprint
actionFamily                 = timer.create
verificationPolicy           = readback_required
```

The normal operation idempotency scope remains actor/backend/resource/action +
idempotency key. An exact replay with the exact payload returns
`alreadyPrepared`; changed desired state or changed reserved CREATE handoff is
an operation conflict.

## Crash/restart rule

After restart the immutable payload is loaded by `operationId`; no caller must
reconstruct it from UI input or guess a new binding identity. This is a
prerequisite for the later durable dispatch/start path and **no blind retry**
rule.

If an operation has already advanced beyond `accepted`, preparation does not
silently recreate or replace the payload. Later dispatch/reconciliation logic
owns that operation state.

## Scope boundary

This component does not dispatch, call Agent, SuiteBridge or VDR, advertise a
write command, create a native Timer, or mark an assignment bound. It only
produces durable pre-dispatch authority. Real runtime mutation remains closed
until the later execution path and exact-candidate yaVDR acceptance are ready.
