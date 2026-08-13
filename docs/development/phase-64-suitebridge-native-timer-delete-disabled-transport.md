# Phase 64 — SuiteBridge native Timer-delete disabled transport

This slice introduces the first concrete local transport for the already fenced
`vdr.timer.delete` Agent command while deliberately keeping every real Timer
mutation disabled.

## Boundary

`SuiteBridgeSvdrpTransport` now implements the typed
`IBackendAgentNativeTimerDeleteTransport` contract. It discovers the local
SuiteBridge provider through `NTDEL CAP 1` and serializes `NTDEL EXEC` with the
complete persisted Timer-delete identity and provider fence: command and request
identity, operation/revision, native binding/revision, assignment/native Timer,
job/attempt/claim epoch, Agent/backend generation, provider ownership,
plugin-instance epoch, provider/capability generations, and the durable local
`starting` timestamp.

The SuiteBridge plugin owns a private `SuiteBridgeNativeTimerDeleteService` for
that typed protocol. The service shares the same plugin-instance epoch as the
existing native-probe service. `NTDEL` is intentionally not listed in public
SVDRP help.

## Disabled execution contract

The plugin advertises the typed transport contract with `mutations=disabled` and
`execution=disabled`. A syntactically valid and fully current `NTDEL EXEC`
request can only return `rejected_without_effect`. A provider/plugin fence drift
also returns a definitive no-effect rejection. There is no accepted outcome in
this slice.

Transport loss or a malformed/unrecognized post-dispatch reply is different:
the Agent maps it to `outcome_unknown`. That preserves the existing no-blind-
retry rule even though the current plugin implementation has no mutation path.

## Safety invariants

- No real VDR Timer deletion exists in this slice.
- No `cTimers`, VDR Timer delete API, RESTfulAPI Timer delete, generic SVDRP
  `DELT`, shell command, or mutation callback is reachable from the new plugin
  service.
- No replay ledger exists yet because no request can ever be accepted for
  mutation. The replay ledger is a required later boundary before an accepted
  mutation outcome can be introduced.
- The installed Agent does not inject this transport into
  `BackendAgentCommandClientConfig`.
- `vdr.timer.delete` remains not advertised and absent from packaged Agent
  configuration.
- Provider availability still does not create authority; the executor continues
  to compare the freshly discovered provider facts with the persisted provider
  selection before `deleteTimer()` can be called.
- Final Timer-delete success still requires authoritative native-Timer absence
  readback in the control plane.

## Next slice

Before SuiteBridge may perform any Timer mutation, add a plugin-instance-scoped
exact-request replay ledger with reserve-before-side-effect semantics and a typed
mutation callback boundary. That successor must still remain unconfigured until
a separate real-mutation acceptance gate proves no duplicate delete on replay or
ambiguous delivery.
