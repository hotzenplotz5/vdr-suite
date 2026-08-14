# Phase 64 Slice 23 — Native Timer Delete Dispatch Claim and Outcome Contract

## Status

Bounded dispatch-state contract stacked on Phase 64 Slice 22 / Draft PR #175.

Slice 22 reserves one shared ADR-0042 delete operation and returns immutable
pre-dispatch target context. Slice 23 defines how that context is claimed before
an executor may act and how a later typed executor outcome is translated into
the shared operation lifecycle plus, when necessary, the Slice-18 authoritative
absence-readback expectation.

The later fingerprint-CAS hardening extends the same claim boundary with the
exact native Timer observed-state fingerprint captured during preparation. This
adds a resource-state CAS fence without changing the dispatch-state vocabulary.

This slice still contains **no Agent/VDR transport wiring** and performs no
native Timer mutation itself.

## Dispatch claim

Before any future executor invocation, `claim()` reloads both the durable
`MutationOperation` and `NativeTimerBinding` and requires the exact Slice-22
handoff identity:

- operation ID and preparation-time operation revision;
- `resourceType = NativeTimerBinding`;
- action family `timer.delete`;
- verification policy `readback_required`;
- exact binding ID and preparation-time binding revision;
- exact expected native Timer observed-state fingerprint;
- exact TimerAssignment ID;
- exact backend ID/generation;
- exact backend-native Timer identity;
- managed/adopted ownership;
- binding still present and free of unresolved drift.

The durable operation must carry the same
`expectedResourceFingerprint` as the handoff's
`expectedNativeTimerFingerprint`, and the current binding's canonical
`observedFingerprint` must still equal that expected value.

Only then may the shared operation transition:

```text
accepted -> dispatching
```

through its exact `operationRevision` fence.

The resulting `NativeTimerDeleteDispatchClaim` carries the new operation
revision and the same `expectedNativeTimerFingerprint`. Exact claim replay while
the operation remains `dispatching` is idempotent and returns the original
durable claim time; it does not create a second dispatch authorization.

Any binding revision/generation/identity change after preparation fences the
claim before execution. A changed native Timer observed state also fences the
claim even if a stale or corrupted path were to leave the binding revision
unchanged. In that case the operation remains `accepted`; no dispatch claim is
minted and no executor may act.

A caller cannot substitute another fingerprint in the handoff: that conflicts
with the fingerprint already persisted in the durable mutation operation and is
rejected as an identity conflict before dispatch.

## Executor outcome vocabulary

The future runtime adapter must reduce its durable transport/native evidence to
exactly one typed outcome:

```text
rejectedWithoutEffect
acceptedUnverified
outcomeUnknown
```

These categories intentionally mirror the Phase-63 fenced-native-operation
principles without importing Agent transport types into the Timer domain.

### rejectedWithoutEffect

The executor has authoritative evidence that no native Timer effect occurred
after the operation was claimed. Because the shared operation has already
entered `dispatching`, this is a verified failure and maps to:

```text
dispatching -> failed_verified
```

It does **not** mint a readback expectation.

`failed_before_dispatch` remains reserved for failures closed before the
`accepted -> dispatching` claim. Once dispatch has been claimed, a missing or
ambiguous response must never be rewritten as `failed_before_dispatch`.

### acceptedUnverified

The executor accepted or reported the native operation, but authoritative VDR
state has not yet verified the postcondition:

```text
dispatching -> executed_unverified
```

The outcome must include the real native dispatch lower-bound timestamp. Slice
23 then creates the exact Slice-18 expectation with:

```text
operationState = executed_unverified
readbackNotBefore = dispatchStartedAt
```

### outcomeUnknown

A dispatch may have reached the executor but the result is ambiguous (for
example a lost response after dispatch began):

```text
dispatching -> outcome_unknown
```

The same real dispatch boundary is retained as `readbackNotBefore`, with the
Slice-18 operation state `outcome_unknown`. No blind retry is authorized.

## Trustworthy readback fence

`dispatchStartedAt` is forbidden for `rejectedWithoutEffect` and mandatory for
`acceptedUnverified` / `outcomeUnknown`.

For those two states it must satisfy:

```text
claim.claimedAt <= dispatchStartedAt <= outcome.completedAt
```

This is the first slice in the delete chain allowed to mint
`readbackNotBefore`, because only a dispatch-outcome producer can know a real
post-claim native dispatch boundary. Slice 22 intentionally could not provide
this fact.

An observation older than this value remains unable to prove the delete.

## Durable operation state and replay

The executor outcome carries a bounded durable evidence reference. The shared
operation stores that reference with its lifecycle transition.

The dispatch claim remains correlated to the exact durable expected native
Timer fingerprint when the outcome is applied; a claim with a missing or
substituted fingerprint cannot advance the operation lifecycle.

Exact outcome replay is idempotent: if the operation already has the same target
state and evidence reference, the repository returns the already-applied
transition. For readback-requiring outcomes the same expectation is reproduced
from the caller's durable outcome evidence.

A different outcome after another lifecycle state has won the race fails
closed. The Timer service does not rewrite history.

The eventual runtime integration must make the executor outcome itself durable
before relying on replay after process restart. Phase 63 already demonstrates
this principle with protected command/result state; Slice 23 only defines the
Control-Plane mapping.

## Relationship to Phase 63

Phase 63 established that:

- `starting` must be durable before a local executor call;
- executor acceptance is distinct from authoritative readback;
- a lost response after dispatch begins becomes reconciliation / unknown
  outcome rather than safe retry;
- transport success alone cannot make an operation successful.

Slice 23 preserves those semantics. It does not reuse the non-mutating
`vdr.native.probe` command as mutation authority and does not expose a generic
native command namespace.

## Scope boundary

This slice adds only the claim/outcome service, focused regression,
architecture guard, isolated Make fragment and documentation.

It adds:

- no Agent/VDR transport wiring;
- no RESTfulAPI/SVDRP/SuiteBridge command construction;
- no real native Timer delete;
- no provider selection change;
- no TimerAssignment transition;
- no replacement/failover;
- no daemon/runtime source wiring;
- no public Timer mutation API;
- no broad Timer UI;
- no `mutations=enabled` switch.

Because no installed runtime path changes, real yaVDR runtime acceptance is not
required for Slice 23.

## Next bounded work

**Slice 24** should define the domain-specific Agent command/executor contract
for native Timer deletion. It must bind one command/attempt to the Slice-23
claim, including the exact expected native Timer fingerprint, use the already
accepted Phase-63 generation/Agent/provider/claim fences, persist the
command-side `starting` state before local execution, and return one of the exact
Slice-23 outcomes with durable evidence.

The first real runtime wiring must remain separately acceptance-gated. It must
not enable broad mutations merely by adding a Timer-delete command capability.
