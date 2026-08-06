# Phase 63 Slice 3 — Durable Agent Command Delivery Runtime

## Status

Bounded runtime implementation of the merged durable command-delivery contract.
It adds one explicitly non-mutating `probe.noop` command so delivery, receipt,
result, replay, restart and reconciliation semantics can be exercised without
crossing a native VDR execution boundary.

## Runtime boundary

The Control Plane durably records one operation, job attempt, claim epoch and
Agent assignment before exposing it on authenticated Agent-only routes. The
Agent stores the assignment in a protected local inbox before acknowledging a
receipt, persists `starting` before the non-mutating probe executor, and stores
the result before transport. Identical assignment, receipt and result replay is
idempotent; conflicting identities fail closed.

The current Backend, Agent actor, Agent instance and backend generation fence
poll, receipt and result traffic. Poll additionally requires a current lease and
an explicit `probe.noop` command capability. Expired assignments are not exposed.
An Agent restart at `starting` or `accepted_by_executor` produces
`outcome_unknown` and `reconcile_only`; it never re-executes blindly.

Equivalent assignment replay preserves the original durable receipt and result
payloads. The Control Plane exposes bounded delivery, receipt-replay and
result-replay counters for acceptance evidence. Capability replacement uses
parameterized SQLite statements. A probe cannot be assigned until the current
Agent instance and backend generation have published `probe.noop` explicitly.

A fully acknowledged local command from an older Agent instance or backend
generation is safely retired after restart. An unacknowledged or in-flight old
state remains fenced with `local_command_generation_fenced`; it is never silently
retired or executed under the new generation.

## Transport and persistence

Protected routes remain under `/api/agent/v1/commands/*` and use the existing
technical Agent authentication. There is no public Agent or provider endpoint.
Suite-owned SQLite tables hold assignments, command capabilities, receipts,
results and guarded acceptance fault flags. The Agent local state file defaults
to `/var/lib/vdr-suite/backend-agent/commands.state` with mode 0600.

The local administration utility can enqueue a bounded probe, inspect its
state, request equivalent replay, and arm one deliberate lost receipt/result
response for guarded real-system acceptance. These controls do not create a
native executor and cannot carry arbitrary payloads.

## Regression coverage

The focused runtime suite covers capability publication before assignment,
parameterized capability replacement, equivalent receipt/result replay,
delivery and replay counters, acknowledged stale-state retirement, and
fail-closed handling of unacknowledged stale state. The existing Backend Agent
lifecycle HTTP fixture constructs the same command-delivery service used by the
production Agent route wrapper, so constructor and linkage changes are exercised
by the complete Agent foundation regression rather than only by the new tests.

## Guarded real-system acceptance

The acceptance target is:

```text
make phase63-command-delivery-runtime-acceptance
```

It requires an exact branch, exact head and fresh evidence directory. Before any
runtime configuration change, the runner rebuilds daemon, Agent, enrollment,
Agent-admin and command-admin candidates and byte-compares every installed
binary with the checkout build.

The runner temporarily enables only `COMMAND_TYPES=probe.noop`, preserves and
restores the original Agent configuration and protected command-state file, and
uses repository-owned administration utilities rather than manual SQLite. It
must prove:

- a baseline command reaches `effect_reported` / `succeeded`;
- equivalent assignment, receipt and result replay are durable and observable;
- deliberately lost receipt and result responses recover without re-execution;
- Control-Plane command state survives a daemon restart;
- a fully acknowledged old local state retires across an Agent restart while the
  old generation is not replayed;
- a fresh command succeeds under the new generation;
- Agent identity and credential generation remain unchanged;
- VDR configuration, timers, SearchTimer state and recording directories remain
  byte/identity stable;
- original configuration and local state are restored and VDR, daemon and Agent
  are active at completion.

Evidence logs are scanned for secret-like material. The runner contains no
native VDR mutation and performs no manual SQLite inspection.

## Hard exclusions

- no Timer, Recording, SearchTimer, Remote, configuration or metadata mutation;
- no generic command tunnel or executable payload;
- no provider ownership or provider selection;
- no Phase 64 TimerIntent runtime;
- no browser/client command endpoint;
- no inbound listener on the VDR site;
- no automatic migration of existing direct adapters.
