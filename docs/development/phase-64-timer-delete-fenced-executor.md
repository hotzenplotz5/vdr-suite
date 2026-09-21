# Phase 64 Slice 32: Fenced Timer-delete executor contract

Slice 32 introduces the first typed Timer-delete executor boundary, but deliberately does **not** wire it into the shipped Agent command loop or into a concrete SuiteBridge/VDR mutation transport.

## Scope

The new `BackendAgentNativeTimerDeleteExecutor` accepts only an already-valid typed local `starting` state plus the original command assignment. It is a fresh-handoff API, not a recovery API.

Before a delete transport can be called it rechecks:

- exact assignment-to-local-state command identity,
- Agent identity and backend generation,
- command deadline,
- the persisted local provider selection,
- provider id and kind,
- provider instance epoch,
- provider generation,
- capability revision,
- required `vdr.timer.delete` capability,
- current provider availability.

The persisted provider selection remains the authority decision. Current local provider facts are only a fence proving that the selected provider instance is still the same usable instance. Availability cannot replace or refresh persisted ownership.

## Exactly-once dispatch boundary

`backendAgentNativeTimerDeleteExecuteFreshStartingOnce()` contains exactly one `deleteTimer()` call and no retry loop.

A context, deadline, discovery, or provider-fence failure occurs before dispatch and produces `rejected_without_effect` evidence.

After the transport call begins:

- definitive no-effect rejection -> `rejected_without_effect`,
- accepted but not yet absence-verified -> `accepted_unverified`,
- transport exception, ambiguity, or invalid accepted evidence -> `outcome_unknown`.

`accepted_unverified` is intentionally not success. Final Timer-delete success still requires authoritative native-timer absence readback in a later slice.

## Recovery boundary

A durable `starting` record by itself is never sufficient authority to call this executor after process recovery. Slice 30 already converts recovered `starting` to persisted `outcome_unknown` / `reconcile_only`. A later integration slice may call the executor only from the same fresh control flow that has just completed Slice 31's durable starting handoff.

Completed local evidence is rejected as fresh execution input.

## No production mutation path

This slice adds only the typed executor and an injected transport interface used by focused fake-transport tests.

It does not:

- reference the executor from `BackendAgentCommandClient.cpp`,
- implement the Timer-delete interface in `SuiteBridgeSvdrpTransport`,
- add a SuiteBridge SVDRP Timer-delete command,
- advertise or configure `vdr.timer.delete`,
- call RESTfulAPI, SVDRP `DELT`, shell commands, or VDR timer mutation APIs.

The installed runtime therefore remains behaviorally fenced exactly as in Slice 31. A subsequent slice must separately wire the fresh durable starting boundary to this executor before any concrete mutation transport is considered.
