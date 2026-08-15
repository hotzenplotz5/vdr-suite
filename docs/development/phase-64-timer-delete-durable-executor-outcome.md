# Phase 64 — Timer delete durable executor outcome

Slice 33 connects the Slice-31 durable fresh-starting handoff to the Slice-32 one-shot Timer-delete executor while deliberately keeping the production mutation transport absent.

## Scope

`BackendAgentCommandClientConfig` gains an optional typed `IBackendAgentNativeTimerDeleteTransport*`. It defaults to `nullptr`; neither the standalone Agent runtime nor packaged configuration supplies one in this slice. `vdr.timer.delete` therefore remains non-configured and non-advertised.

When the pointer is absent, the fresh Timer-delete path is unchanged from Slice 31: the Agent durably persists the typed `starting` extension, acknowledges the receipt, returns `native_delete_local_starting_handoff_persisted`, and performs no executor call.

When a bounded test/injected transport is present, the fresh path is:

1. validate current Agent/backend generation and the command deadline;
2. create and durably persist the typed local `starting` state;
3. acknowledge the command receipt and durably persist that acknowledgement;
4. invoke the Slice-32 fresh one-shot executor exactly once;
5. convert the returned typed evidence to local `completed` state;
6. project the same evidence to the generic command result;
7. durably persist typed completed evidence and the generic result together through the existing protected `commands.state` v3 writer;
8. only then send the generic result to the control plane.

The executor itself rechecks deadline, Agent/backend generation, provider instance epoch, provider generation, capability revision, required capability, and provider availability immediately before dispatch. Current availability is only a usability fence; it never replaces the persisted provider ownership/selection as authority.

## Crash and replay semantics

There is still no blind retry after `starting` becomes durable.

If receipt delivery fails, no executor call has occurred. The durable `starting` record remains. A later reconciliation enters the existing Slice-30 recovery path first and conservatively completes it as `outcome_unknown` / `reconcile_only`; it does not call the executor.

If the process fails after receipt acknowledgement but before executor dispatch, the same durable `starting` record is recovered as `outcome_unknown` / `reconcile_only`. The delete is not attempted later.

If failure occurs after dispatch begins but before the completed evidence can be persisted, recovery again sees only durable `starting` and therefore produces `outcome_unknown` / `reconcile_only`. The delete is never repeated.

If completed evidence and the generic result are durable but result delivery fails, the later pass replays only the persisted result. The executor is not re-entered.

Completed typed evidence remains historical authority even if provider or Agent generations drift later.

## Outcome projection

The Slice-32 executor categories retain their existing generic projections:

- `rejected_without_effect` -> `not_started` / verified rejection / no retry;
- `accepted_unverified` -> `accepted_by_executor` / `outcome_unknown` / `reconcile_only`;
- `outcome_unknown` -> `starting` / `outcome_unknown` / `reconcile_only`.

`accepted_unverified` is not final success. Final Timer-delete success still requires the authoritative native-timer absence readback and operation completion path from the earlier Phase-64 readback contracts.

## Production boundary

This slice adds orchestration but still adds **no concrete Timer-delete mutation transport**. `SuiteBridgeSvdrpTransport` does not implement `IBackendAgentNativeTimerDeleteTransport`; there is no SuiteBridge Timer-delete command, raw `DELT`, RESTfulAPI delete, shell fallback, or direct VDR `cTimers` mutation. The Agent configuration and packaged `COMMAND_TYPES` remain unchanged, and `availableCommands()` continues to suppress `vdr.timer.delete`.

The only executor calls in this slice are through an explicitly injected typed fake transport in focused tests. A later slice must introduce the concrete SuiteBridge transport behind a separate safety gate before any real VDR timer mutation is allowed.
