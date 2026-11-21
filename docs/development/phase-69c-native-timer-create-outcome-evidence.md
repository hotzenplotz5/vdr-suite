# Phase 69.C — Native Timer CREATE Outcome Evidence

## Purpose

This bounded slice closes one prerequisite immediately before the first productive
Native Timer CREATE effect can be activated.

The accepted Phase-69.C runtime already composes the durable Timer CREATE
admission, reservation, dispatch, readback-verification and completion owners.
However, the production Agent result envelope previously persisted only the
generic outcome projection. The Timer CREATE executor itself retained stronger
typed evidence in Agent-local durable state, including `dispatchStartedAt` and
`evidenceReference`, but those facts did not cross the existing
`BackendAgentCommandResult` authority.

Opening native dispatch in that state would be unsafe: the Control Plane could
receive `outcome_unknown` without enough durable evidence to construct the
already accepted `NativeTimerCreateExecutorOutcome` without guessing.

## Bounded change

The existing `BackendAgentCommandResult` gains one optional bounded
`resultEvidence` field.

For `vdr.timer.create` only, the Agent serializes a typed
`native-timer-create-result-evidence/1` payload from the already durable
executor evidence. The payload carries the Timer CREATE outcome category,
Agent-local durable-start timestamp, native-dispatch start timestamp,
completion timestamp and bounded native evidence reference.

The Control Plane:

1. parses the evidence against the exact durable command assignment;
2. validates it against the reserved Timer CREATE command/provider fence;
3. requires the generic result projection to agree with the typed outcome;
4. persists the evidence in the existing `backend_agent_command_results`
   row;
5. includes the evidence in the existing result identity;
6. exposes a read-only `resultForCommand()` lookup through the same repository
   authority.

No second result table, Timer-specific result repository or lifecycle owner is
introduced.

## Compatibility

The Agent protocol remains `vdr-suite-agent/1`.

Legacy result JSON without `resultEvidence` remains accepted for command types
that do not require typed evidence. Serialization adds `resultEvidence` only
when it is non-empty. For Native Timer CREATE, missing or conflicting typed
evidence fails closed.

Existing databases receive one additive
`backend_agent_command_results.result_evidence TEXT NOT NULL DEFAULT ''`
column through `BackendAgentCommandRepository::ensureSchema()`.

The Agent command-state file format is not revved. Timer CREATE already persists
the full typed executor evidence inside its accepted
`vdr.timer.create.local-state.v1` extension; result evidence is reconstructed
from that durable extension after restart and checked before result delivery.

## Boundary that remains closed

This slice deliberately does **not** invoke:

```text
BackendAgentNativeTimerCreateReservationService::reserve()
NativeTimerCreateDispatchService::claimAfterReservation()
BackendAgentNativeTimerCreateActivationService::activateDispatching()
NativeTimerCreateDispatchService::applyOutcome()
NativeTimerCreateReadbackVerificationService::verify()
TimerAssignmentFulfillmentService::bindVerified()
NativeTimerCreateOperationCompletionService::complete()
```

Therefore no Agent command becomes pollable and no Native Timer CREATE side
effect becomes reachable in this slice.

## Successor boundary

After this prerequisite is accepted, the next candidate may connect the
existing durable command result to the already composed
`NativeTimerCreateDispatchService::applyOutcome()` and authoritative Timer
readback path. Productive reservation/claim/activation must still be activated
only in a bounded slice whose end-to-end outcome/readback path is complete.

The first slice that makes `vdr.timer.create` newly pollable or executable in
the Phase-69 path requires real yaVDR acceptance before merge.
