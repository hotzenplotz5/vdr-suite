# Phase 64 — Timer-delete local-state lifecycle

Slice 30 integrates the already-defined `vdr.timer.delete.local-state.v1` extension into the existing protected Agent `commands.state` lifecycle. It does not make Timer-delete executable.

## Recovery before generation fencing

A durable Timer-delete local state is safety evidence, not execution authority. The command-state owner therefore decodes and recovers that typed evidence before applying the generic current Agent/backend-generation fence.

This ordering has one purpose: already-durable completion evidence must survive a later Agent instance or backend-generation change, and a recovered durable `starting` record must never be reopened for execution. The generic generation fence still applies immediately afterwards and still prevents an old command result from being sent under a changed execution context.

## Durable `starting` recovery

`starting` is the no-blind-retry hazard boundary established by Slice 27. On recovery the typed state returns `reconcileOnly` with conservative `outcome_unknown` evidence. Slice 30 completes the typed state with that evidence and persists the completed extension together with its generic command result through the existing `commands.state` v3 atomic writer.

There is no retry decision and no call to `backendAgentNativeTimerDeletePrepareLocalStarting()` in the runtime owner. If the process fails before the completed write becomes durable, the original durable `starting` state remains and the next recovery again chooses reconciliation only.

## Completed evidence projection

Already-completed typed evidence is never recomputed from current provider availability. It is projected into the bounded generic command-result vocabulary while the typed extension remains the detailed local evidence authority:

- `rejectedWithoutEffect` -> verified generic rejection with no dispatch boundary
- `acceptedUnverified` -> reconciliation-only generic `outcome_unknown`
- `outcomeUnknown` -> reconciliation-only generic `outcome_unknown`

The two dispatch-capable outcomes deliberately remain unresolved at the generic layer. Native Timer absence readback and the shared mutation-operation lifecycle remain responsible for proving the final postcondition in later slices.

A pre-existing generic result that conflicts with the completed typed evidence fails closed instead of replacing or weakening that evidence.

## No execution boundary

Slice 30 does **not** add any of the following:

- Timer-delete configuration or command advertisement
- fresh Timer-delete `starting` preparation from a newly delivered command
- Timer-delete executor or SuiteBridge write transport
- RESTfulAPI or SVDRP Timer deletion
- shell fallback
- native VDR Timer mutation
- public Timer mutation API
- TimerAssignment transition or failover

The existing `availableCommands()` suppression remains in force, so the installed Agent still cannot request a new `vdr.timer.delete` assignment.

## Validation

The existing `commands.state` v3 regression is extended to prove:

- a durable `starting` extension becomes durable completed `outcome_unknown` evidence and sends only a reconciliation-only generic result
- completed evidence survives Agent/backend-generation context drift while the generic generation fence still blocks sending it
- `rejectedWithoutEffect` projects to a verified rejection
- `acceptedUnverified` remains unresolved and reconciliation-only
- v1/v2/v3 parsing, cross-command/fingerprint rejection, protected persistence, `probe.noop`, native-probe dependencies and Timer-delete advertisement suppression remain intact

The dedicated Slice 30 architecture guard forbids preparation, executor/write transport coupling and runtime advertisement.

## Next bounded slice

Slice 31 may introduce the fresh durable-starting handoff for a newly delivered Timer-delete command. It must persist the exact typed `starting` extension through this same protected state owner before any possible side-effectful call, and it must still stop before the real delete executor unless that executor boundary is separately fenced and tested.
