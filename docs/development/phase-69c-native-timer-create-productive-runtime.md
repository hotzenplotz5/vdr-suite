# Phase 69.C — Native Timer CREATE Productive Runtime

## Status

**ACTIVE — first productive native-effect Timer CREATE candidate.**

Live implementation baseline for this candidate:

```text
main=feb45998dbb447ff0e3534422a97d3df47ac879d
69.C=ACTIVE
ADR-0064=CLOSED
Timer mutation transport=typed SVDRP
```

Binding architecture:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0063](../adr/ADR-0063-mutation-complexity-proportionality-reuse.md)
- [Native Timer CREATE Outcome Application](phase-69c-native-timer-create-outcome-application.md)
- [Native Timer CREATE Readback Reconciliation](phase-69c-native-timer-create-readback-reconciliation.md)
- [ADR-0064 closeout](../architecture/suitebridge-control-plane-closeout.md)

## Live gap proven before this slice

All durable Timer CREATE owners already existed on the baseline, but production
stopped after public admission:

```text
POST public TimerAssignment CREATE
-> NativeTimerCreateAdmissionService::admit()
-> accepted MutationOperation + immutable native.timer.create payload
-> STOP
```

The Agent result endpoint already durably accepted typed CREATE evidence, and
the accepted outcome/readback adapters already knew how to finish the lifecycle,
but neither was productively invoked.

The daemon already has one periodic VDR polling cadence. Introducing another
Timer-specific poller or retry queue would therefore create a second lifecycle
authority without need.

## Productive bounded lifecycle

This slice reuses the existing daemon poll and advances only durable,
non-terminal `timer.create` operations:

```text
accepted
-> exact BackendAgent command reservation
-> MutationOperation dispatch claim
-> exact reservation activation
-> Agent pollability
-> existing SuiteBridge typed-SVDRP VDR Timer CREATE
-> durable typed BackendAgentCommandResult
-> applyDurableNativeTimerCreateResult()
-> executed_unverified OR outcome_unknown
-> complete authoritative Timer inventory
-> reconcileNativeTimerCreateReadback()
-> verified NativeTimerBinding
-> TimerAssignment bound
-> MutationOperation succeeded
```

The runtime does not own a table, repository, thread, timer, retry loop or
native transport. It is one stateless `advance...Once()` composition called
from the already existing `DaemonRuntime::pollVdrAndUpdateChangeFeed()` cadence.

## Durable discovery and restart recovery

`MutationOperationRepository` gains only one bounded read operation:
`listByActionFamilyAndStates()`.

It does not create a queue. It reads the existing operation authority with an
explicit action family, explicit owned states and a hard maximum. The productive
Timer runtime asks only for:

- `accepted`;
- `dispatching`;
- `executed_unverified`;
- `outcome_unknown`.

This makes crash recovery deterministic:

- crash after admission: exact reservation can be created/replayed;
- crash after reservation but before claim: the same reservation is replayed;
- crash after claim but before activation: the same reservation is activated;
- crash after native effect: only the durable Agent result is applied;
- crash after outcome application: the same result reconstructs the same
  readback expectation;
- terminal `succeeded` operations are no longer candidates.

No in-memory pending list is authoritative.

## No blind retry

Native dispatch is reachable only while the authoritative operation state is
`accepted`.

Once the operation is `dispatching`, `executed_unverified` or
`outcome_unknown`, the productive runtime never calls reservation/claim again.

In particular, `outcome_unknown` is reconciliation-only. A timeout or unknown
native outcome cannot cause another SVDRP CREATE.

Pre-effect reservation/activation replay remains exact and idempotent under the
existing command, operation revision, backend generation and provider-selection
fences.

## Authoritative readback

The runtime does not infer success from the Agent result.

For each backend generation needed by pending CREATE reconciliation, one
per-poll evidence acquisition is cached and reused. It requires:

1. the current BackendAgent generation to equal the operation expectation;
2. the existing VDR Timer observation path;
3. a successful `RestfulApiNativeTimerInventoryReader` complete inventory;
4. exact inventory/observation identity agreement through
   `VdrManagedTimerCreateReadbackEvidenceBuilder`.

Only that complete evidence is passed to the existing readback verifier.

The verifier still requires:

- `observedAt >= readbackNotBefore`;
- exact backend/generation;
- exactly one matching managed correlation;
- exact reserved TimerAssignment and NativeTimerBinding identities;
- exact native Timer specification.

The fulfillment and completion owners then independently reload and fence their
own durable state before `succeeded`.

## Transport boundary

ADR-0064 remains closed.

This slice does not add a Unix Timer mutation transport and does not move Timer
CREATE away from typed SVDRP. Activation merely makes the already accepted
`vdr.timer.create` Agent command pollable; the existing SuiteBridge executor
and bounded VDR Timer write lock remain the native-effect path.

Recording editing transport and Home/Live frontend code are untouched.

## Focused repository acceptance

The productive runtime regression proves:

- one accepted operation becomes one exact command;
- accepted -> dispatching advances the operation revision once;
- replay while dispatching retains the same command ID;
- durable typed accepted-unverified evidence is the outcome authority;
- readback freshness comes from the Agent dispatch-start timestamp;
- authoritative readback produces a verified NativeTimerBinding;
- TimerAssignment becomes `bound`;
- MutationOperation becomes `succeeded`;
- the terminal operation disappears from runtime discovery.

Architecture guards additionally reject a new Timer-specific thread/retry loop,
public/security authority leakage and unbounded operation discovery.

## Acceptance prerequisite gap

The first real yaVDR attempt on this candidate proved one additional acceptance
precondition that was not reachable from the installed product state: the
stable public CREATE route starts from an already durable `selected`
TimerAssignment, while the current production DaemonRuntime does not expose a
TimerIntent/TimerAssignment scheduling entry point and the legacy
`/api/vdr/timers/actions/create` path does not create Phase-64 TimerIntent or
TimerAssignment state.

The candidate therefore carries one **domain-backed acceptance fixture** used
only by the real-system harness. It is not linked into the daemon, not installed
as a product binary and not exposed as an HTTP API.

The fixture creates the missing prerequisite through the existing Phase-64
authorities only:

```text
TimerIntentRepository::create()
-> TimerIntentRepository::update(draft -> active)
-> TimerAssignmentSchedulingService::schedulePrimary()
-> durable selected TimerAssignment
```

It consumes the real backend generation and an existing VDR channel selected by
the acceptance runner. The fixture specification is disabled (`enabled=false`)
so the real native Timer produced by the subsequent acceptance cannot start a
recording. The fixture itself does not dispatch a native effect. The subsequent
public-v1 POST remains the first operation that transitions the selected
assignment into the productive CREATE lifecycle.

There are **no raw SQL fixture writes**. The helper may read runtime evidence
outside the process, but durable TimerIntent/TimerAssignment creation is owned
only by the accepted repositories and `TimerAssignmentSchedulingService`.
Architecture guards also keep the helper out of production daemon sources and
installation targets.

## Real-system gate

This is the first Phase-69.C candidate that can make `vdr.timer.create`
pollable and therefore can produce a real VDR Timer.

```text
NATIVE_EFFECT_REACHABLE=YES
YAVDR_ACCEPTANCE_REQUIRED=YES
```

Hosted CI is necessary but not sufficient. The exact PR head must first build
and self-test the domain-backed acceptance fixture and then pass real yaVDR
acceptance before merge. The real run must prepare one `selected` assignment
through the accepted Timer domain authorities and then prove one-native-Timer
creation through the public-v1 admission, durable result/outcome, verified
binding/bound assignment/succeeded operation, idempotency replay,
generation/revision/fingerprint fencing, `outcome_unknown` no blind retry and
restart persistence where exercised.

## Boundary after this slice

This slice does not complete all of Phase 69.C by itself. It closes the
productive native Timer CREATE runtime boundary. Subsequent 69.C work must be
derived from the live public revision/precondition/idempotency exposure roadmap
after this candidate is accepted; it must not reopen ADR-0064 transport work.
