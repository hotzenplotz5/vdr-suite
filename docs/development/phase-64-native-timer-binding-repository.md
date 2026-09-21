# Phase 64 Slice 10 — NativeTimerBinding Persistence Repository

## Scope

Slice 10 adds durable SQLite persistence for the backend-neutral
`NativeTimerBinding` contract introduced by Slice 9.

The slice remains repository-only. It does not map live VDR snapshots, reconcile
drift, transition `TimerAssignment` to `bound`, adopt external Timers, execute
replacement/failover, dispatch Agent Timer commands or mutate native Timers.

## Repository ownership

`NativeTimerBindingRepository` is the sole issuer of durable
`bindingRevision` values.

Creation requires an empty caller revision and persists revision `1`.
Updates require the caller to present the exact current opaque revision and the
repository issues the successor only after the durable write succeeds.

The current implementation uses positive decimal revision tokens internally.
Callers must continue to treat them as opaque concurrency values.

## Stable identity

One binding has one stable Suite identity:

- `nativeTimerBindingId`

A generic repository update cannot change:

- `nativeTimerBindingId`;
- `backendId`;
- `backendNativeTimerId`;
- `timerAssignmentId`;
- `ownership`.

This is deliberate. Observation persistence must not become an implicit
ownership or adoption API.

`backendGeneration` is different: it is observation evidence, not part of the
stable backend-native identity key. A later authoritative observation may
advance the generation for the same backend/native Timer relationship.

A generation rollback is rejected as `generationConflict`.

## Backend-native identity

The repository enforces one durable binding for each exact:

```text
backendId + backendNativeTimerId
```

through a unique SQLite index and transaction-scoped preflight check.

`backendGeneration` is intentionally not part of this unique key. ADR-0044
allows a backend restart to preserve the native Timer while the reporting Agent
generation changes. The existing binding must be updated with the newer
generation instead of duplicated.

`findByBackendNativeTimer()` therefore resolves the stable backend/native tuple
without pretending that a native Timer ID is globally meaningful.

## Assignment relationship

The Slice-9 domain contract already requires `managed` and `adopted` bindings
to carry a `timerAssignmentId`.

Slice 10 adds a durable single-current-binding invariant:

```text
at most one ownership in {managed, adopted}
for one non-empty timerAssignmentId
```

This is enforced by a partial unique SQLite index and a repository preflight
check.

`orphaned_managed` and `ambiguous` rows are not included in that partial unique
constraint because unresolved evidence may coexist while reconciliation decides
which relationship is authoritative.

This slice deliberately does not add a foreign-key-style requirement that every
stored assignment identity already resolves to a current `TimerAssignment`.
Orphaned and imported evidence must remain representable, and authoritative
relationship reconciliation is a later service boundary.

## No implicit adoption or ownership reassignment

`update()` is observation/revision persistence only.

It rejects changes to `timerAssignmentId` or `ownership` with
`immutableConflict`. In particular, a caller cannot use generic persistence to
turn:

```text
external -> adopted
external -> managed
managed assignment A -> managed assignment B
```

Adoption and ownership reclassification require a later explicit authorized
operation with the ADR-0044 checks and the Phase-62/ADR-0049 accountability
boundary.

## Observation monotonicity

A stale observation must not overwrite newer durable evidence.

`update()` rejects:

- `backendGeneration` lower than the durable generation;
- `lastObservedAt` lower than the durable observation time.

These produce `generationConflict` and `observationConflict` respectively.

Equal values are allowed so a readback pipeline can persist additional
classification or verified-operation evidence from the same observation
boundary without inventing a clock tick.

## Concurrency and transactions

Create and update execute under:

```text
BEGIN IMMEDIATE TRANSACTION
```

plus the existing in-process `Database::TransactionLease`.

The database transaction is the authoritative cross-connection writer fence.
Unique indexes backstop stable native identity and the single managed/adopted
assignment relationship.

Update uses exact revision matching and writes with:

```text
WHERE native_timer_binding_id=? AND binding_revision=?
```

A stale writer receives the current durable binding with `conflict` and must
reload/re-evaluate. It is never silently retried with stale observation data.

The focused regression additionally uses two independent `Database` connections
against the same file-backed SQLite database to prove that a writer holding
revision `1` cannot overwrite revision `2` committed by another connection.

## Lookup semantics

The repository exposes only bounded persistence-oriented reads:

- `findById(nativeTimerBindingId)`;
- `findByBackendNativeTimer(backendId, backendNativeTimerId)`;
- `listForAssignment(timerAssignmentId)`.

`listForAssignment()` is deterministic by stable binding ID and includes
unresolved/orphaned rows as evidence. It does not choose which binding should
win reconciliation.

No title/time similarity lookup is added.

## SQLite boundary

Direct SQLite remains confined to the repository implementation units:

```text
core/timers/src/NativeTimerBindingReadRepository.cpp
core/timers/src/NativeTimerBindingWriteRepository.cpp
```

The orchestration/schema unit `NativeTimerBindingRepository.cpp` and its internal
`NativeTimerBindingRepositoryStorage.h` helper declaration remain in the same
domain repository boundary. The SQLite-bearing implementation units end in
`Repository.cpp`, matching ADR-0050's existing approved
`core/timers/src/*Repository.cpp` rule.

No new SQLite exception, daemon adapter allowance or wildcard architecture rule
is needed.

## Regression contract

The focused repository regression proves:

- schema creation succeeds;
- create issues revision `1`;
- ID lookup round-trips the full domain value;
- backend/native lookup is backend-scoped;
- stable binding-ID replay reports `alreadyExists`;
- duplicate backend/native identity reports `nativeIdentityConflict`;
- one assignment cannot receive two simultaneous managed/adopted bindings;
- ambiguous evidence for the same assignment remains representable;
- assignment listing is deterministic;
- update issues the successor revision;
- stale revision returns current durable state with `conflict`;
- backend/native identity cannot be changed through generic update;
- ownership/assignment cannot be changed through generic update;
- generation rollback fails closed;
- observation-time rollback fails closed;
- caller-owned create revisions are rejected;
- a second SQLite connection cannot overwrite a newer revision with a stale
  previously-read token.

## Runtime boundary

This slice changes no installed runtime path.

It adds no:

- live VDR Timer observation mapper;
- reconciliation service;
- `TimerAssignment` transition to `bound`;
- ownership adoption API;
- replacement/failover executor;
- Agent Timer command;
- SuiteBridge, RESTfulAPI or SVDRP Timer mutation;
- daemon scheduler/reconciler wiring;
- public Timer mutation API;
- broad Timer UI;
- `mutations=enabled`.

Real yaVDR runtime acceptance is therefore not required. A focused real yaVDR
build/test remains useful as repository portability evidence.

## Next bounded work

After this repository is accepted, the next safe slice is an authoritative
native-observation mapping/readback boundary that converts existing backend
Timer read facts into `NativeTimerObservedState` without yet mutating native
state.

Only after persisted binding readback and reconciliation can controlled
replacement or native create/update/delete safely rely on confirmed native
presence, absence and drift evidence.
