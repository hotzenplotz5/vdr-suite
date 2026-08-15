# Phase 64 Slice 26 — Native Timer Delete Delivery Boundary

## Status

Bounded Control-Plane delivery slice stacked on Phase 64 Slice 25 / Draft PR
#178.

Slice 25 persists one operation-bound `vdr.timer.delete` command plus its exact
local-provider selection, but deliberately keeps Timer-delete capability claims
dormant. Slice 26 opens only the server-side protocol and delivery boundary.
It does **not** add a local Timer-delete executor and does not perform a VDR
mutation.

## Availability is not authority

An Agent poll may now name the bounded command type:

```text
vdr.timer.delete
```

That name alone is never execution authority. A Timer-delete advertisement is
accepted only when the same poll also carries one valid, available provider fact
for:

```text
providerId = suitebridge:local
providerKind = suitebridge
capability = vdr.timer.delete
```

The provider fact proves only current local availability. The Control Plane still
requires the separately configured, active `vdr.timer` provider ownership and the
persisted Slice-25 `BackendAgentLocalProviderSelection` fence.

No availability signal can create or replace ownership.

## Delivery fence

Before a persisted Timer-delete assignment is counted as delivered, the Control
Plane revalidates its recorded provider selection against current facts and
ownership:

- backend ID;
- exact Agent ID and Agent instance;
- backend generation;
- authority domain `vdr.timer`;
- provider ID `suitebridge:local`;
- provider kind `suitebridge`;
- ownership generation;
- provider instance epoch;
- provider generation;
- capability revision; and
- required capability `vdr.timer.delete`.

If any fence is stale or missing, the poll remains fail-closed and no assignment
is returned.

The repository also removes the temporary Slice-25 dormant-capability trigger at
schema initialization and immediately before a command poll. This is intentional:
Slice 25's assignment helper remains a separately testable dormant boundary, while
Slice 26 is the explicit delivery gate. If the earlier helper recreates that
temporary trigger, the next accepted delivery poll removes it before capability
facts are persisted.

## First receipt versus replay

The first receipt for `vdr.timer.delete` is accepted only while the persisted
provider selection is still current. This prevents a command that was delivered
under one local-provider fence from crossing the durable Agent receipt boundary
after that provider has been replaced.

Once an exact receipt is already durable, an identical receipt replay is accepted
without requiring the provider to remain current. This preserves lost-response
idempotency: a later provider change must not turn an already committed receipt
into an apparent failure.

A conflicting receipt is still rejected.

## Result correlation after provider drift

A result for a received Timer-delete command is correlated to the provider
selection embedded in the immutable command payload and to the separately
persisted provider-selection sidecar. Those identities must match exactly.

The result path deliberately does **not** require the provider to still be
current. After a durable receipt, execution or non-execution evidence can arrive
after provider replacement; the Control Plane must retain that evidence for
reconciliation rather than discard it as stale.

Slice 26 regression uses only a `not_started` / `rejected` result because there is
no local Timer-delete executor in this slice.

## Protocol boundary

`BackendAgentCommandJson` accepts `vdr.timer.delete` as one of the finite command
types in a poll request. Arbitrary command strings remain invalid.

A valid Timer-delete poll must pass the dedicated advertisement contract before
command capabilities or provider facts are committed. The command response still
uses the existing generic, fingerprinted `BackendAgentCommandAssignment` envelope.

## Shipped Agent remains fail-closed

This slice intentionally does not change:

- `BackendAgentClient.cpp` command-type configuration;
- `BackendAgentCommandClient.cpp` local execution/reconciliation;
- packaged `COMMAND_TYPES` defaults;
- SuiteBridge command transport;
- SVDRP or RESTfulAPI write paths; or
- `vdr-plugin-suite-bridge`.

Therefore the shipped Backend Agent still cannot advertise or execute
`vdr.timer.delete`. Slice 26 makes the Control Plane ready to validate a future
bounded advertisement, but production mutation remains disabled.

The later Agent-execution slice must first provide durable local `starting`
evidence and the exact Slice-24 command/evidence correlation before any native
dispatch boundary can be opened.

## Regression coverage

The focused regression proves:

- Timer-delete poll JSON round-trip;
- duplicate Timer-delete type rejection;
- missing exact provider fact rejection;
- unavailable provider rejection;
- missing Timer-delete provider capability rejection;
- wrong-provider rejection;
- retirement of the temporary Slice-25 dormant trigger at delivery time;
- successful delivery only with exact current provider ownership/selection;
- provider replacement fences redelivery;
- provider replacement fences the first receipt;
- an exact durable receipt replay survives later provider drift;
- a non-dispatch result survives later provider drift only when its immutable
  payload selection matches the persisted sidecar; and
- existing `probe.noop` delivery remains functional.

## Runtime acceptance boundary

This slice changes installed Control-Plane command JSON and delivery code, so it
requires exact-head GitHub CI followed by a bounded real-yaVDR build/install and
runtime check.

Real-yaVDR acceptance must **not** configure `vdr.timer.delete`, create a
Timer-delete command, call a native write transport or delete a VDR Timer. It must
instead prove that:

- the exact candidate binaries are installed;
- daemon and existing Backend Agent recover and remain active;
- the existing Agent returns to `online` and remains `readOnly=true`;
- backend generation and heartbeat progress remain valid;
- the packaged/live Agent configuration still does not advertise
  `vdr.timer.delete`; and
- the nearest existing non-mutating Agent/control-plane path still works.

## Scope boundary

Slice 26 adds no:

- public Timer mutation API;
- Timer-delete assignment runtime orchestration;
- Backend Agent Timer-delete executor;
- durable local `starting` implementation for Timer delete;
- SuiteBridge Timer-delete transport;
- SVDRP or RESTfulAPI Timer write;
- native VDR Timer mutation;
- operation completion integration;
- TimerAssignment transition, failover or replacement;
- broad Timer UI; or
- `mutations=enabled` state.

## Next bounded work

The next slice should define the Agent-local durable `starting` and fail-closed
receipt/execution handoff for the exact Slice-24 Timer-delete command. It must not
call a real SuiteBridge/VDR delete until the local crash-recovery state and
provider-instance revalidation are independently proven.
