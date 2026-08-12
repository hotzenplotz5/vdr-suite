# Phase 64 Slice 31: Timer-delete fresh durable starting handoff

## Goal

Slice 31 connects the already-defined Timer-delete local `starting` contract to the real Agent `commands.state` owner for a **fresh**, current-context command. It establishes the durable no-blind-retry boundary before the control plane can observe an accepted receipt.

This slice still has **no Timer-delete executor and no VDR mutation**.

## Ordering

A Timer-delete state with an existing typed extension is still handled by the Slice-30 recovery path before the generic current-generation fence. That preserves completed historical evidence across later Agent/backend-generation drift.

A Timer-delete state with no typed extension is different: it is a fresh handoff and may only be armed after all of the following are true:

1. the generic assignment loaded successfully from protected `commands.state`;
2. the assignment matches the current backend, Agent, Agent instance, and backend generation;
3. the command has not expired;
4. the generic dispatch state is exactly `not_started` and no generic result exists.

Only then does the command owner call `backendAgentNativeTimerDeletePrepareLocalStarting()`, wrap the typed local state in `vdr.timer.delete.local-state.v1`, set the generic dispatch state to `starting`, and persist the complete v3 state through the existing protected atomic writer.

The accepted receipt is sent **after** that durable write. Therefore an accepted Timer-delete receipt can never precede the local no-blind-retry boundary.

## Crash and transport behavior

If the process or receipt transport fails after the durable starting write, the state file remains `starting` with the typed extension. On the next reconciliation the Slice-30 recovery path consumes that existing evidence and produces conservative `outcome_unknown` / `reconcile_only` completion evidence. It never performs a second fresh preparation and never blind-retries a future side effect.

This is intentionally conservative even though Slice 31 has no executor yet. It establishes the exact crash semantics required before a later slice is allowed to place an executor call after the durable starting boundary.

## Deadline and generation fences

Fresh starting is never created for a stale Agent/backend generation. The generic generation fence runs before the fresh handoff.

Fresh starting is also never created for an expired command. An expired Timer-delete assignment remains `not_started`, receives the normal command receipt/result treatment, and is rejected as `expired` without acquiring a typed local starting extension.

Completed typed evidence remains different from fresh state: it is still interpreted before the generation fence so historical outcome evidence cannot be discarded merely because the current Agent generation changed.

## No execution boundary

Timer-delete remains suppressed from `availableCommands()`, absent from Agent configuration, and absent from packaged configuration. Slice 31 does not add a SuiteBridge write transport, RESTfulAPI delete, SVDRP `DELT`, shell fallback, executor interface, or any native VDR Timer mutation.

The only new runtime authority is permission to durably arm an already-present valid Timer-delete command state. The shipped Agent still cannot normally poll such a command and cannot execute one.

## Acceptance

The focused regression covers:

- fresh current-context `not_started` -> durable typed `starting` before accepted receipt;
- second reconciliation -> Slice-30 `outcome_unknown` / `reconcile_only`, never fresh re-preparation;
- stale generation -> no starting extension and no receipt;
- expired assignment -> rejection without a starting extension;
- receipt transport failure after durable starting -> recovery from the same starting evidence, never blind retry;
- protected `commands.state` mode preservation;
- all Slice-30, Slice-29 and Phase-63 command-delivery regressions through target dependencies.

A real yaVDR runtime gate is required because the installed Backend Agent command-state owner changes. That gate must keep Timer-delete unconfigured and execute no Timer mutation.
