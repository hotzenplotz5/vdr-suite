# Phase 64 Slice 8 — Replica TimerAssignment Scheduling Handoff

## Scope

Phase 64 Slice 8 extends the bounded Control-Plane scheduling handoff with the
first durable **replica** TimerAssignment path.

Slice 5 established deterministic planning. Slice 6 connected primary planning
to durable TimerAssignment persistence. Slice 7 added the per-TimerIntent
`assignmentSetRevision` concurrency primitive required when correctness depends
on the complete current assignment set rather than only the single-primary
unique invariant.

Slice 8 now wires those pieces together for deliberate replicas only:

```text
exact current TimerIntent
  -> assignmentSetRevision
  -> current durable assignments
  -> deterministic replica planning
  -> createAgainstAssignmentSetRevision()
  -> durable replica TimerAssignment
```

Replacement remains deferred. Native Timer execution remains deferred.

Binding architecture:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0050: Domain Repository SQLite Boundary](../adr/ADR-0050-domain-repository-sqlite-boundary.md)
- [Phase 64 Slice 5: Deterministic TimerAssignment Planning](phase-64-timer-assignment-planning.md)
- [Phase 64 Slice 6: Primary TimerAssignment Scheduling Handoff](phase-64-timer-assignment-scheduling-handoff.md)
- [Phase 64 Slice 7: TimerAssignment Set-Revision Fence](phase-64-timer-assignment-set-revision-fence.md)

## Why replica persistence needs a different race fence

The primary scheduling path can rely on the repository's atomic partial unique
index and ownership check: there can be at most one active primary assignment
for one TimerIntent.

Replica scheduling has a different invariant. Its correctness depends on the
complete active assignment set because the planner evaluates:

- current primary plus replica count against `desiredAssignments`;
- explicit `simultaneousRecordingIntentional` policy;
- backend diversity;
- site diversity;
- backends already used by active ownership assignments.

Two schedulers that both plan from the same assignment set must not both create
a new replica after one of them has already satisfied the requested replica
count. The Slice-7 `assignmentSetRevision` is the durable optimistic-concurrency
fence for that race.

## Exact service ordering

`scheduleReplica()` uses the following order:

```text
validate stable request identity
  -> resolve idempotent existing assignment ID
  -> reload exact current TimerIntent
  -> verify expectedIntentRevision
  -> read assignmentSetRevision
  -> list current assignments
  -> retain current active ownership assignments
  -> plan with role=replica
  -> persist only with createAgainstAssignmentSetRevision()
```

The set revision is intentionally **read before the assignment list**.

Any successful TimerAssignment insert, update or delete after that token read
advances the durable set revision. The final fenced create therefore fails
closed even when a subsequent list read happens to observe some or all of the
newer state. A scheduler never treats a later list read as permission to reuse
an older concurrency token.

The scheduling service contains no SQLite. The repository remains the sole
owner of the durable set-revision check, assignment revision, assignment epoch
and storage transaction.

## Replica policy remains planner-owned

The service does not reproduce Slice-5 replica policy. It supplies the exact
current TimerIntent, active assignments and caller-resolved backend evidence to
`planTimerAssignment()` using `TimerAssignmentRole::replica`.

The existing deterministic planner remains responsible for rejecting replica
planning unless:

- `desiredAssignments > 1`;
- `simultaneousRecordingIntentional` is true;
- the desired active primary+replica count is not already satisfied;
- backend and site diversity requirements are satisfied;
- the selected backend passes every ordinary capability, health, channel,
  authority and conflict fence.

The service cannot weaken those rules.

## Target already satisfied is a successful no-op

When the planner returns `unassigned` with reason
`replica_target_satisfied`, `scheduleReplica()` returns the dedicated successful
status `replicaTargetSatisfied`.

This is a **successful no-op**:

- no new `unassigned` TimerAssignment is created;
- no new assignment epoch is issued;
- `assignmentSetRevision` does not change;
- the stable request assignment ID remains unused;
- no backend mutation occurs.

Persisting an unassigned history row in this case would make a harmless
scheduler replay mutate the very assignment set whose satisfied state caused
the no-op. Slice 8 deliberately avoids that feedback loop.

## Ordinary unassigned replica decisions remain durable

`replica_target_satisfied` is different from a real scheduling failure such as
`no_eligible_backend`.

If replica policy requests another assignment but no backend can safely own it,
the planner's ordinary `unassigned` decision remains durable. The resulting
TimerAssignment has:

- role `replica`;
- state `unassigned`;
- no backend target or backend generation;
- the exact current TimerIntent revision;
- bounded planner evidence explaining why no backend was eligible;
- repository-issued assignment revision and assignment epoch.

That preserves auditable scheduling evidence without pretending that a native
Timer exists.

## Stale assignment-set outcome

A selected or ordinary-unassigned replica decision is persisted only through:

```text
createAgainstAssignmentSetRevision(candidate, exactSetRevision)
```

If another assignment mutation wins first, the repository returns its existing
optimistic-concurrency `conflict`. The service exposes this specifically as
`assignment_set_conflict`.

`assignment_set_conflict` is not retried invisibly. The caller **must re-plan**
from a newly loaded TimerIntent, new assignment-set revision, new assignment
list and current backend evidence. Reusing the previous selected backend after
a set conflict would defeat the purpose of the fence.

## Idempotent response-loss replay

The stable `timerAssignmentId` retains the Slice-6 idempotency rule.

If a response is lost after a replica assignment was durably created, replaying
the same logical request with the same:

- TimerAssignment ID;
- TimerIntent ID and revision;
- replica role;
- creation timestamp;
- scheduler policy version

returns the already-durable assignment as `alreadyPersisted` rather than
planning or creating another replica.

Reusing the same assignment ID with incompatible immutable request identity is
`assignmentIdConflict`.

## Primary behavior is intentionally unchanged

Slice 8 does not migrate `schedulePrimary()` to the complete assignment-set
fence. Its existing single-active-primary repository invariant is already the
narrow atomic race fence required by that path.

Keeping the paths asymmetric is intentional: an unrelated replica or
unassigned-history mutation must not make an otherwise safe primary
idempotency/ownership decision fail merely because the entire assignment set
changed.

## Replacement remains deferred

Replacement is not equivalent to replica creation.

A replacement requires controlled handover evidence including the old
assignment owner, assignment epoch, backend generation and authoritative native
outcome/absence state. In particular, a lost or uncertain native create result
must be reconciled before another backend may receive a replacement Timer.

Slice 8 therefore adds no `scheduleReplacement()` path and no replacement
persistence shortcut.

## Regression contract

The focused Slice-8 regression proves:

- a primary assignment can exist first on backend A;
- the first deliberate replica is selected on backend B under backend and site
  diversity;
- the replica receives the next repository-issued assignment epoch;
- the set revision advances after durable replica creation;
- response-loss replay returns the same durable replica;
- reusing the assignment ID with incompatible immutable request identity fails;
- once desired replica count is satisfied, a new request returns
  `replicaTargetSatisfied` and creates no third assignment;
- the successful no-op does not advance `assignmentSetRevision`;
- a real `no_eligible_backend` replica decision is durably represented as
  target-free `unassigned` evidence;
- stale TimerIntent revision fails before persistence;
- invalid replica policy fails as `planningInvalid`;
- missing and malformed requests fail closed;
- the dedicated `assignment_set_conflict` status is part of the scheduling
  contract for stale set-fenced creates.

The Slice-7 repository regression remains the authoritative cross-connection
race proof for the atomic storage primitive itself. Slice 8 proves that the
service uses that primitive in the required order rather than bypassing it.

## Runtime boundary

This slice changes **no installed runtime path**.

It adds no:

- daemon scheduler loop;
- replacement/handover executor;
- `NativeTimerBinding` contract or repository;
- Agent Timer command;
- SuiteBridge, RESTfulAPI or SVDRP Timer mutation;
- native VDR Timer create/update/delete/toggle;
- public Timer mutation API;
- broad Timer UI;
- `mutations=enabled` mode;
- Phase-65-or-later runtime.

Therefore real yaVDR runtime acceptance is not required. A focused build/test on
a real yaVDR checkout remains useful source/build evidence, but installation and
service restart are unnecessary.

## Next bounded work

After this slice, the next assignment-role work is **not** an immediate
replacement create. The safe next boundary is to define the exact controlled
handover evidence needed for replacement, or to proceed with the backend-neutral
`NativeTimerBinding` domain contract if replacement depends on binding/absence
evidence that is not yet modeled.

ADR-0044 requires authoritative native readback before a bound assignment or
uncertain-dispatch failover can be considered safe. Native Timer mutation still
comes later.
