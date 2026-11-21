# Phase 64 Slice 27: Native Timer delete local durable starting

Slice 27 defines the Agent-local durable `starting` and crash-recovery contract
for the exact `vdr.timer.delete` envelope introduced by Slices 24-26.

## Scope

This slice is deliberately a typed local-state contract, not a native Timer
executor. It reuses the strict Slice-25 Timer-delete payload, the Slice-24
command/evidence chronology, and the generic Agent command identity fields.

The local state is bound to:

- command ID and request fingerprint;
- operation ID and operation revision;
- native Timer binding ID and expected binding revision;
- TimerAssignment ID and backend-native Timer ID;
- job, attempt and claim epoch;
- backend, Agent instance and backend generation;
- the exact `vdr.timer` local-provider ownership/capability selection; and
- the Control-Plane claim timestamp.

## Hazard window

Before a future side-effectful Timer delete may be called, the typed local
`starting` state must already have been durably persisted by the Agent command
state owner.

A durable `starting` record opens the hazard window immediately. If the Agent
crashes after that persistence, recovery cannot prove whether the future
side-effectful call was never entered, was entered, or completed without its
response becoming durable. Therefore recovery never returns an execution or
retry decision.

Instead it returns `reconcileOnly` and conservative `outcomeUnknown` evidence.
For that evidence, `dispatchStartedAt` is conservatively set to
`localStartingPersistedAt`. This is not a claim that VDR was actually mutated at
that instant; it records the earliest possible dispatch boundary so a restart
cannot manufacture a blind retry.

If a provider/context generation changes after `starting`, the decision remains
reconciliation-only. Provider drift cannot reopen execution.

## Proven no-effect and completed evidence

A future executor may complete the state only with evidence that still satisfies
the existing `backendAgentNativeTimerDeleteEvidenceMatches()` contract.

`rejectedWithoutEffect` remains the only outcome allowed with
`dispatchStartedAt == 0`. `acceptedUnverified` and `outcomeUnknown` require a
dispatch boundary at or after durable local starting.

Once completion evidence is durable, recovery returns that exact persisted
evidence even if the Agent/backend generation has since changed. Later provider
replacement must not erase already-produced outcome evidence.

## Durable encoding

The slice defines a bounded, exact-key, versioned encoding for the typed state.
The format carries the immutable command/provider fence and either:

- `phase=starting` with no outcome evidence; or
- `phase=completed` with exact outcome timing and evidence reference.

The codec is intended to be embedded into the existing protected Agent command
state ownership boundary. It does not introduce a second independently managed
runtime state file.

## No runtime wiring

`BackendAgentNativeTimerDeleteLocalState.cpp` is intentionally absent from
`mk/agent-sources.mk`. `BackendAgentCommandClient.cpp`, `BackendAgentClient.cpp`
and the packaged Agent configuration still contain no `vdr.timer.delete`
execution/advertisement path.

This slice adds no SuiteBridge Timer-delete write method, no SVDRP/RESTfulAPI
delete, no shell transport and no native VDR Timer mutation.

## Next bounded slice

The next separately gated slice may integrate this typed state into the generic
Agent command-state owner and define the executor handoff boundary. That
integration must preserve the rule that durable `starting` exists before any
side-effectful transport call and that recovery never blindly retries an
uncertain delete.
