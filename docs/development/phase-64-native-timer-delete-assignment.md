# Phase 64 Slice 25 — Native Timer delete assignment persistence

## Status

Stacked on Phase 64 Slice 24 / Draft PR #177.

This slice introduces the dormant Control-Plane assignment/persistence boundary
for the already-defined domain-specific `vdr.timer.delete` Agent contract. The command is not advertised, delivered or executed by the Agent
in this slice.

## Goal

Persist exactly one Agent command for one already-claimed ADR-0042 native Timer
delete while preserving the existing Phase-63 authority model:

```text
shared mutation operation claim
        + active exact Agent/backend generation
        + explicit vdr.timer provider ownership
        + observed suitebridge:local vdr.timer.delete provider capability
        -> command + provider selection persisted atomically
```

No Timer-specific command table is introduced. The existing
`backend_agent_commands` and `backend_agent_command_provider_selections` remain
the single durable command/selection authority.

## Dormant command boundary

The shared command validator gains one exact allowlisted payload shape for:

```text
commandType = vdr.timer.delete
payloadVersion = 1
verificationPolicy = readback_required
```

The payload binds:

- operation revision;
- NativeTimerBinding identity and expected revision;
- TimerAssignment identity;
- backend-native Timer identity;
- Control-Plane dispatch-claim time;
- exact backend/provider ownership generation;
- provider instance epoch and provider generation;
- capability revision;
- `vdr.timer.delete` as the required capability.

This is validation only. Slice 25 does not add `vdr.timer.delete` to Agent
`supportedCommandTypes`, does not change `BackendAgentCommandClient`, and does
not add a SuiteBridge transport method.

## Assignment authority

`BackendAgentNativeTimerDeleteAssignmentService` requires:

1. authenticated system context;
2. bounded exact operation/binding/assignment/native identities;
3. non-zero exact backend generation from the already-claimed operation;
4. a live, non-revoked, compatible Agent lease;
5. exact Agent backend generation equality;
6. active explicit `vdr.timer` ownership;
7. current provider facts for the owned provider;
8. observed and authorized `vdr.timer.delete` capability;
9. exact provider identity, instance epoch, provider generation and capability
   revision.

Unlike the Phase-63 native probe assignment, Slice 25 deliberately does not
require the Agent command capability table to contain `vdr.timer.delete`.
That omission is a safety property: the command may be persisted for contract
validation but remains undeliverable until a later runtime slice explicitly
advertises and fences the command type.

Dormancy is also enforced server-side. Establishing the Slice-25 assignment
schema deletes any pre-existing `vdr.timer.delete` command-capability fact and
installs the temporary SQLite trigger
`trg_backend_agent_timer_delete_dormant_capability`. The trigger ignores any
new Agent attempt to publish `vdr.timer.delete` into the command-capability
table. Therefore a buggy or hostile Agent cannot make the stored delete command
deliverable merely by claiming support. Slice 26 may remove this gate only in
the same bounded change that adds the accepted provider/selection delivery
fences.

## Durable idempotency

The generic command store gains a bounded partial unique index:

```text
(backend_id, operation_id) WHERE command_type = 'vdr.timer.delete'
```

An exact retry reloads the existing command, parses the durable payload, checks
the provider-selection sidecar, re-resolves the current provider fence and
returns the same command as a replay. Changed target identity or changed
operation context fails as a conflict. A provider/ownership/generation change
makes the old assignment stale instead of creating a replacement command.

A race between two exact first assignments is also fenced by the unique index;
the loser reloads and can only become an exact replay.

## Atomic command + provider selection

The actual write continues to use
`BackendAgentCommandRepository::insertAssignment()`. That Phase-63 primitive
already inserts the command row and optional provider-selection sidecar in one
`BEGIN IMMEDIATE` transaction. Slice 25 does not duplicate that write path.

## Provider evidence is not command advertisement

The regression intentionally records local provider facts containing
`vdr.timer.delete` while publishing only `probe.noop` in
`supportedCommandTypes`. It verifies that the Timer-delete assignment exists in
the durable command store but is not returned by command polling.

Provider availability therefore remains descriptive evidence, not execution
authority.

## Runtime impact

`BackendAgentNativeTimerDeleteAssignment.cpp` is intentionally not added to any
runtime source manifest in Slice 25. The existing Slice-24 contract source also
remains unwired, preserving its architecture guard.

The already-installed shared `BackendAgentCommand.cpp` does gain the strict
payload-validation branch so the existing generic repository can validate the
new command envelope during the Slice-25 regression. That changes compiled
command-domain code but does not create a production assignment caller,
advertisement or executor. A bounded yaVDR build/test is therefore required for
the final candidate, but no live Timer mutation acceptance is applicable yet.

## Scope boundary

Slice 25 adds no:

- public API route;
- Timer delete administration action;
- Agent command advertisement for `vdr.timer.delete`;
- command-poll delivery rule for Timer delete;
- receipt/result runtime handling specific to Timer delete;
- `BackendAgentCommandClient` execution branch;
- SuiteBridge command or SVDRP write;
- RESTfulAPI write;
- native VDR Timer mutation;
- TimerAssignment lifecycle transition;
- replacement/failover;
- broad Timer UI;
- `mutations=enabled` runtime.

## Validation

The focused regression covers:

- explicit ownership required;
- exact provider fact selection;
- persisted payload and provider sidecar correlation;
- shared generic command validation and fingerprinting;
- exact assignment replay returning the same command/job identity;
- changed native target conflict;
- backend-generation conflict;
- non-system caller rejection;
- future claim-time rejection;
- provider generation/epoch replacement fencing an old assignment;
- a new operation binding to the new current provider fence;
- observed capability disappearance failing closed;
- absence of `vdr.timer.delete` in the Agent command capability table;
- no command delivery while only `probe.noop` is advertised;
- server-side suppression even if an Agent claims `vdr.timer.delete` support.

## Next bounded work

Slice 26 should add the first explicit delivery-runtime fence for
`vdr.timer.delete`: advertise the capability only when a separately accepted
SuiteBridge mutation capability exists, require the persisted provider selection
to remain current at command poll/receipt, and keep the Agent local executor
fail-closed until durable `starting` plus the real Timer-delete transport are
implemented together.

That is the first slice that approaches live mutation execution and therefore
requires a separate security/runtime acceptance gate.
