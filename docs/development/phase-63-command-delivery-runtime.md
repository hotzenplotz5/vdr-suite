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

## Hard exclusions

- no Timer, Recording, SearchTimer, Remote, configuration or metadata mutation;
- no generic command tunnel or executable payload;
- no provider ownership or provider selection;
- no Phase 64 TimerIntent runtime;
- no browser/client command endpoint;
- no inbound listener on the VDR site;
- no automatic migration of existing direct adapters.
