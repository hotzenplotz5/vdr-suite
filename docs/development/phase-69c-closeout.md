# Phase 69.C Closeout — Revision, Preconditions and Idempotency

## Status

**COMPLETED**

```text
69.C=COMPLETED
next=69.D - Collections, pagination and partial results
```

Phase 69 remains active. This closeout does not complete Phase 69 and does not
promote any pre-v1 route into the stable public contract.

Binding architecture:

- [ADR-0042 Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0063 Mutation Complexity Proportionality and Reuse](../adr/ADR-0063-mutation-complexity-proportionality-reuse.md)
- [ADR-0064 closeout](../architecture/suitebridge-control-plane-closeout.md)

ADR-0064 remains closed. Timer CREATE/DELETE/MODIFY stay on the accepted typed
SVDRP mutation transport unless a later operation-specific architecture slice
proves a separate need.

## Completion decision

The live post-PR-361 audit found no remaining 69.C runtime gap. The four roadmap
requirements are now satisfied by one coherent stable public Timer CREATE
vertical plus its public read resources:

| 69.C requirement | Accepted evidence |
| --- | --- |
| Resource-specific revisions | Durable MutationOperation and TimerAssignment revisions are exposed only as strong opaque public ETags. |
| ETag / conditional requests | Public operation and TimerAssignment item reads support conditional GET; Timer CREATE requires a canonical strong `If-Match`. |
| Explicit mutation idempotency | Public Timer CREATE requires `Idempotency-Key`; exact replay recovers the same durable operation, while changed scope/fingerprint fails with `409 idempotency_conflict`. |
| No unsafe fallback after ambiguous mutation errors | Native CREATE has one durable command path; `outcome_unknown` is reconciliation-only and never authorizes redispatch or a fallback transport. |

The atomic CREATE admission rechecks the durable idempotency scope while holding
`BEGIN IMMEDIATE`, so two first submissions cannot both create independent
logical operations. A new logical request with an old resource revision fails
with `412 revision_conflict`.

## Accepted implementation chain

69.C was delivered through bounded checkpoints rather than one large mutation:

- PR #321 — shared opaque ETag / public precondition foundation;
- PR #322-#326 — actor-owned operation reads and backend-scoped
  TimerAssignment read/public resource composition;
- PR #327-#330 — dormant native Timer CREATE preparation, dispatch,
  NativeTimerSpecification and fulfillment composition;
- PR #331 — Suite-owned operation and binding identity issuance;
- PR #332 — atomic `selected -> provisioning + MutationOperation/payload`
  admission boundary;
- PR #333/#334 — accepted reconciliation/runtime prerequisites;
- PR #341/#342 — durable typed Agent outcome evidence/application and readback
  reconciliation;
- PR #361 — productive native Timer CREATE runtime and final real-system gate.

Final productive merge:

```text
PR #361
accepted head = 9316fb48a49f3e61ff89d3c01a225915003fa8e8
merge/main    = a38e2363929c2a1f1b60984f6176aa88446a9054
CI #9285      = 36275800779 / SUCCESS (6/6)
```

## Real yaVDR acceptance

The exact PR #361 candidate was installed on the supported real yaVDR/VDR 2.7.9
system. The acceptance intentionally used a disabled future Timer so the test
could prove a native Timer write without starting a recording.

The final fresh CREATE proved:

```text
CREATE_HTTP=202
STATE=accepted
STATE=executed_unverified
STATE=succeeded
COMMAND_COUNT=1
ASSIGNMENT_STATE=bound
TIMERS_BEFORE=0
TIMERS_AFTER=1
PR361_FRESH_CREATE=PASS
```

The public safety contract then proved:

```text
REPLAY_HTTP=202
REPLAY_OPERATION=same durable operation
COMMAND_COUNT_AFTER_REPLAY=1
CONFLICT_HTTP=409
code=idempotency_conflict
STALE_HTTP=412
code=revision_conflict
COMMAND_COUNT_FINAL=1
PR361_IDEMPOTENCY_REVISION=PASS
```

After restarting only the daemon, durable state and native readback remained
stable:

```text
OPERATION_AFTER_RESTART=succeeded
ASSIGNMENT_AFTER_RESTART=bound
OWNERSHIP=managed
COMMAND_COUNT_AFTER_RESTART=1
GENERATIONS=173|173|173|173
TIMERS_AFTER_RESTART=1
DURABLE_RESTART_NO_REDISPATCH=PASS
PR361_REAL_YAVDR_ACCEPTANCE=PASS
```

The exact generation value is acceptance evidence for that run, not a permanent
runtime constant.

## Real-gate defects found and closed

The real-system gate found two cross-boundary gaps before acceptance:

1. Phase-64 reservation already required explicit durable `vdr.timer` local
   provider ownership, but the existing command-admin authority did not expose
   Timer CREATE status/set/clear controls. PR #361 added those controls without
   auto-claiming authority from observed provider facts.
2. The Agent/Timer domain correctly used the canonical
   `native-timer-specification/1|...` specification fingerprint, while the
   private SuiteBridge NTCREATE parser incorrectly expected a SHA-only token.
   PR #361 aligned SuiteBridge with the existing canonical authority and added a
   guard against reintroducing the split contract.

Neither fix created a second lifecycle, provider authority or mutation
transport.

## 69.C acceptance gate

| Gate | Result |
| --- | --- |
| Public resource revisions are stable and opaque | PASS |
| Conditional GET / strong ETag behavior is covered | PASS |
| Mutation preconditions use strong `If-Match` | PASS |
| Exact idempotent replay returns the same operation | PASS |
| Same idempotency scope with changed fingerprint returns 409 | PASS |
| New logical request with stale revision returns 412 | PASS |
| Concurrent first-submission race is serialized by the atomic admission owner | PASS |
| Ambiguous native outcome never triggers blind redispatch/fallback | PASS |
| Native CREATE reaches authoritative readback and verified managed binding | PASS |
| Restart preserves succeeded/bound/verified state with one command | PASS |
| Exact-head hosted CI | PASS — 6/6 |
| Exact-head real yaVDR native-effect acceptance | PASS |

## Retained boundaries

69.C does **not**:

- create a public collection contract;
- define pagination/cursor or multi-backend partial-result semantics;
- define deprecation/sunset policy;
- complete first-/third-party client migration;
- expose internal Agent, SuiteBridge, native Timer IDs or provider schemas as
  stable public API;
- authorize public Timer UPDATE/TOGGLE/DELETE merely because private native
  implementations exist;
- reopen the completed Phase-64 Timer engine or broad Timer UI milestone.

## Next bounded runtime slice

```text
69.C - Revision/precondition/idempotency exposure [COMPLETED]
  -> 69.D - Collections, pagination and partial results [ACTIVE]
```

69.D must begin from the live public-v1 inventory and select a coherent
collection resource before implementation. Existing pre-v1 collections are not
automatically stable, and no cursor/order/partial-result contract is inferred
from their current shapes.
