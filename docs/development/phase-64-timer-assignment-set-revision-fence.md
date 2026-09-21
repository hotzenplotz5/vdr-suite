# Phase 64 Slice 7 — TimerAssignment Set-Revision Fence

## Scope

Phase 64 Slice 7 adds the repository concurrency primitive required before
replica and replacement scheduling can persist a planner decision safely.

Slice 6 proved the first planner-to-repository handoff for a primary
TimerAssignment. Primary creation already has an atomic durable race fence:
`TimerAssignmentRepository` enforces one active primary inside the same
`BEGIN IMMEDIATE` transaction that issues the assignment epoch.

Replica count and replacement handover cannot rely on that primary-only
invariant. Two schedulers may otherwise read the same assignment set, both plan
from that same state and then both create a new durable assignment.

This slice therefore adds an opaque per-TimerIntent **assignment-set revision**
and an atomic create-against-set-revision repository operation. It does not yet
wire replica or replacement scheduling.

Binding architecture:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0050: Domain Repository SQLite Boundary](../adr/ADR-0050-domain-repository-sqlite-boundary.md)
- [Phase 64 Slice 6: Primary TimerAssignment Scheduling Handoff](phase-64-timer-assignment-scheduling-handoff.md)

## Distinct fences

The assignment-set revision is deliberately distinct from every existing
identity or revision:

```text
intentRevision
  exact desired TimerIntent version

assignmentRevision
  exact version of one durable TimerAssignment

assignmentEpoch
  monotonic ownership-decision epoch for one TimerIntent

assignmentSetRevision
  exact version of the complete durable TimerAssignment set for one TimerIntent
```

`assignmentSetRevision` does not replace `assignmentEpoch`.

The repository remains the sole issuer of assignment revisions and assignment
epochs. The new set revision is only an optimistic-concurrency fence proving
that the assignment set used by a scheduler has not changed before its create
is committed.

## Safe planning order

A future replica/replacement service must use this order:

```text
read assignmentSetRevision
  -> list current assignments
  -> plan from that list
  -> createAgainstAssignmentSetRevision(expected token)
```

Reading the token **before** loading the assignment list is intentional.

If any assignment mutation happens after the token is read, the set revision
changes. Even if the subsequent list happens to observe some or all of that
newer state, the final create still fails closed because its token is stale.

This avoids requiring the planner itself to hold a database transaction while
performing deterministic policy evaluation.

## Durable revision source

The implementation stores one monotonic integer revision per TimerIntent in
`timer_assignment_set_revisions`.

Repository-owned SQLite triggers increment it after every successful:

- TimerAssignment insert;
- TimerAssignment update;
- TimerAssignment delete.

The revision-bump triggers are durable database schema, contain no process-local
state and therefore advance the counter for writes from any connection using the
same SQLite database.

For databases that already contain TimerAssignments when this slice is first
used, the set revision is bootstrapped from the durable maximum
`assignmentEpoch`. No assignment-set token existed before this schema was
installed, so this establishes a safe current baseline without pretending to
reconstruct historical revisions.

An intent with no assignments has set revision `0`.

## Atomic fenced creation

`createAgainstAssignmentSetRevision()` does not implement a second assignment
insert path.

Instead it installs one connection-local TEMP expectation for the stable
`timerAssignmentId` and then invokes the existing
`TimerAssignmentRepository::create()` while holding the repository transaction
lease.

A connection-local `TEMP TRIGGER` performs the `BEFORE INSERT` check against the
durable assignment-set revision at the actual insert boundary. A successful
insert then:

1. uses the existing repository path to issue `assignmentRevision`;
2. uses the existing repository path to issue the next `assignmentEpoch`;
3. preserves the existing active-primary ownership check;
4. increments the durable assignment-set revision through the persistent bump
   trigger;
5. drops the connection-local TEMP trigger and expectation table after the
   create result is resolved.

A stale set token is returned as the repository's normal optimistic-concurrency
`conflict` status.

The expectation itself is deliberately TEMP state rather than a durable row.
It is visible only to the database connection performing the fenced create and
is discarded automatically if that connection or process dies. A crashed
scheduler therefore cannot leave a durable expectation that blocks or
accidentally authorizes a later attempt. Concurrent processes each carry their
own connection-local expectation while SQLite's `BEGIN IMMEDIATE` serialization
and the shared durable set revision decide which write remains current.

## Why SQLite triggers are appropriate here

The trigger does not own scheduler policy or assignment semantics.

The persistent triggers only maintain the repository-local concurrency
counter. The fenced create additionally uses a connection-local TEMP trigger to
verify one expected token at the exact storage boundary. Keeping the authoritative
revision in SQLite is required because a process mutex cannot fence another process or another database connection; keeping the per-attempt expectation TEMP
avoids introducing a durable lock or crash-recovery problem.

The implementation remains under `core/timers/src/*Repository.cpp`, which is
the SQLite boundary allowed by ADR-0050.

## Regression contract

The focused Slice-7 regression proves:

- an empty assignment set starts at revision `0`;
- missing/invalid TimerIntent identities fail closed;
- a create against revision `0` succeeds and advances the set to `1`;
- a second create against stale revision `0` returns `conflict`;
- the stale create does not change the set revision;
- retrying against the current revision succeeds;
- ordinary repository `create()` advances the set revision after the trigger
  schema exists;
- ordinary repository `update()` advances the set revision;
- two planned assignments using one shared token cannot both persist;
- malformed set-revision tokens are rejected;
- pre-existing assignments bootstrap the first set-revision token from durable
  `assignmentEpoch`;
- post-bootstrap updates advance the revision normally.

## Runtime boundary

This slice is repository-only.

It adds no:

- replica scheduling service wiring;
- replacement/handover executor;
- `NativeTimerBinding`;
- Agent Timer command;
- SuiteBridge, RESTfulAPI or SVDRP Timer mutation;
- daemon scheduler loop;
- public Timer mutation API;
- broad Timer UI;
- `mutations=enabled`;
- Phase-65-or-later runtime.

No installed daemon, Backend Agent, SuiteBridge, service configuration or VDR
native Timer path changes, so real yaVDR runtime acceptance is not required.
Focused real-checkout build/test evidence remains useful.

## Next bounded slice

Once this fence is accepted, the next safe step is to wire **replica persistence** through the Slice-6 scheduling service using:

```text
assignmentSetRevision
  -> current assignments
  -> deterministic replica plan
  -> createAgainstAssignmentSetRevision()
```

Replacement remains separate because controlled handover additionally needs
explicit old-owner/native-outcome evidence. It must not be treated as merely
another replica.
