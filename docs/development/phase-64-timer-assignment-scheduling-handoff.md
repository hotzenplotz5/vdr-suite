# Phase 64 Slice 6 — Primary TimerAssignment Scheduling Handoff

## Scope

Phase 64 Slice 6 connects the deterministic Slice-5 planner to the existing
TimerIntent and TimerAssignment repositories for the first durable scheduling
handoff.

The slice is deliberately limited to the primary-assignment path. It loads the
exact current TimerIntent and current active ownership assignments from the
existing repositories, builds a fresh `TimerAssignmentPlanningRequest`, runs the
deterministic planner and persists either one `selected` primary assignment or
one explicit `unassigned` primary decision.

This is still Control-Plane contract code. It does not dispatch a native Timer,
create a NativeTimerBinding or wire a recurring scheduler into the daemon.

Binding architecture:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0050: Domain Repository SQLite Boundary](../adr/ADR-0050-domain-repository-sqlite-boundary.md)
- [Slice 5: Deterministic TimerAssignment Planning Contract](phase-64-timer-assignment-planning.md)

## Why primary only

Slice 5 already defines planning semantics for primary, replica and replacement
roles, but the safe persistence handoff requirements are not identical.

For the primary path, `TimerAssignmentRepository` already owns the durable
single-active-primary race fence through the active-primary check and partial
unique index. If another primary becomes active after the service reads the
current assignment snapshot but before the create commits, repository creation
fails with `ownershipConflict`.

Replica and replacement handoff need an additional exact assignment-snapshot or
controlled-handover fence so concurrent schedulers cannot exceed replica count
or prepare overlapping replacement ownership. Slice 6 does not fake that
safety. Replica and replacement persistence remain deferred.

## Request contract

`TimerAssignmentPrimarySchedulingRequest` contains only caller-owned scheduling
attempt values and backend evidence:

- stable `timerAssignmentId` for this scheduling attempt;
- stable `timerIntentId`;
- exact `expectedIntentRevision`;
- explicit `createdAt`;
- bounded backend candidates in the Slice-5 planning shape.

The service does not accept a caller-supplied TimerIntent object or
caller-supplied current assignment list as scheduler truth.

Instead it reloads the exact current TimerIntent and all current durable
assignments from the repositories immediately before planning. Only active
ownership assignments are passed to the planner because terminal and inactive
history cannot influence primary ownership selection.

The exact durable intent revision must still equal `expectedIntentRevision`.
The repository rechecks that revision again inside `create()` before the
assignment is committed.

## Planning and persistence mapping

The service constructs a fresh primary `TimerAssignmentPlanningRequest` from:

```text
current durable TimerIntent
current active ownership assignments
request backend candidates
role = primary
```

The Slice-5 planner remains the sole selection-policy implementation.

A `selected` decision becomes a new TimerAssignment with:

- `state = selected`;
- exact TimerIntent ID and revision;
- `role = primary`;
- selected backend and backend generation;
- selected channel binding and mapping revision;
- capability and backend-health revisions;
- exact planner policy version;
- bounded planner decision evidence;
- no NativeTimerBinding.

An ordinary `unassigned` decision becomes a target-free durable assignment with
`state = unassigned` and the planner's explicit reason/evidence.

The repository remains the sole issuer of `assignmentRevision` and
`assignmentEpoch`. The scheduling service leaves both caller fields empty/zero
and never manufactures either value.

## Existing active primary

If fresh planning returns `active_primary_exists`, Slice 6 does not create
another target-free history row merely to restate an ownership fact that is
already durable.

The service reloads the active primary and returns
`activePrimaryExists` together with that assignment. No write occurs.

A second active primary that appears only after planning is still rejected by
the repository's durable ownership fence.

## Idempotent replay

The caller supplies one stable `timerAssignmentId` per scheduling attempt and
must reuse it on retry.

Before planning, the service checks that identity. If a durable assignment
already exists with the same:

- TimerAssignment ID;
- TimerIntent ID;
- expected intent revision;
- primary role;
- creation timestamp;
- planner policy version;
- `selected` or `unassigned` state;

the result is `alreadyPersisted` and no new assignment epoch is issued.

Reusing the same TimerAssignment ID for a different scheduling attempt is an
explicit `assignmentIdConflict`.

This idempotent replay behavior prevents response-loss retries from creating a
second durable assignment before native mutation even exists.

## Fail-closed cases

No assignment is created when:

- request identities or timestamp are malformed;
- the TimerIntent does not exist;
- the durable TimerIntent revision differs from the expected revision;
- the current TimerIntent is not assignable;
- the fresh planner input is invalid;
- repository ownership rejects a concurrent second active primary;
- repository persistence fails.

Planner `unassigned` is not an error. It is durable scheduling evidence unless
the reason is the already-durable `active_primary_exists` case above.

## Authority boundary

This slice does not resolve backend/provider authority itself.

The backend candidates still carry the same immutable evidence projection from
Slice 5, including the opaque current execution-authority fence. The scheduling
service neither discovers providers nor falls back between them.

Persistence of a selected assignment is not permission to perform a native
write. Before any later native dispatch, backend generation, assignment
revision/epoch, capability, health, channel and execution-authority evidence
must be revalidated under ADR-0042/ADR-0044.

## Runtime boundary

This slice has no installed runtime path.

It adds no:

- daemon scheduler loop or recurring job wiring;
- public TimerIntent or TimerAssignment API;
- frontend Timer mutation;
- Agent Timer command;
- SuiteBridge, RESTfulAPI or SVDRP Timer mutation;
- NativeTimerBinding contract or repository;
- native Timer create/update/delete/toggle;
- automatic failover;
- replica or replacement persistence handoff;
- account-management UI;
- `mutations=enabled`;
- Phase-65-or-later runtime.

The account/backend access-management gate before broad Timer UI remains
unchanged.

## Regression contract

The focused regression proves:

- fresh durable intent and active-assignment reads feed the planner;
- a safe selected primary decision persists with repository-issued revision and
  epoch;
- exact selected backend/channel/capability/health evidence is retained;
- the same scheduling-attempt ID is an idempotent replay;
- incompatible reuse of that ID fails closed;
- a current active primary produces no second write;
- stale intent revision prevents persistence;
- no eligible backend persists an explicit target-free `unassigned` decision;
- malformed planning input does not persist;
- a non-assignable draft intent does not persist;
- a missing intent fails closed.

The architecture guard prevents direct SQLite, NativeTimerBinding, Agent,
SuiteBridge, RESTfulAPI, SVDRP and runtime wiring in this slice.

## Acceptance

Acceptance for Slice 6 is:

1. Slice-1 through Slice-5 Timer regressions remain green;
2. the focused scheduling-handoff regression passes;
3. the Slice-6 architecture guard passes;
4. repository-wide architecture checks pass;
5. the complete hosted VDR-Suite CI graph is green on one exact Draft-PR head.

No real yaVDR runtime acceptance is required because this slice changes no
installed daemon, Backend Agent, SuiteBridge, service configuration or native
VDR Timer path. A local focused build/test on the exact head remains useful.
