# Phase 64 Slice 5 — Deterministic TimerAssignment Planning Contract

## Scope

Phase 64 Slice 5 adds the smallest deterministic scheduler boundary after the
TimerAssignment value and repository slices: a pure Control-Plane planning
contract that evaluates one current `TimerIntent` plus an immutable snapshot of
current assignment, backend, authority, capability, health, channel-mapping and
conflict evidence.

The output is either one explicitly selected backend or an explicit
`unassigned` decision with bounded evidence. The planner does not persist the
decision, issue an `assignmentEpoch`, create a `NativeTimerBinding`, dispatch an
Agent command or mutate VDR.

Binding architecture:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](../adr/ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0050: Domain Repository SQLite Boundary](../adr/ADR-0050-domain-repository-sqlite-boundary.md)
- [Phase 63 explicit local-provider selection runtime](phase-63-local-provider-selection-runtime.md)

## Authority boundary

The planner owns selection policy, not backend truth.

`TimerAssignmentPlanningBackendCandidate` is an immutable projection of
authoritative facts resolved before planning. It does not replace the existing
authorities:

- backend lifecycle/generation and lease truth remain owned by the Control
  Plane/Backend Agent lifecycle model;
- backend write permission remains the existing server-side backend access
  boundary;
- capability facts remain distinct from health facts and are generation- and
  revision-bound;
- channel evidence remains observation/mapping evidence with an exact mapping
  revision;
- execution authority remains the existing explicit provider-ownership and
  provider-selection boundary established in Phase 63.

The planner receives only an opaque `executionAuthorityFence` plus the fact that
that fence is current. It receives no provider list and has no provider
selection or fallback API. Availability therefore cannot become authority, and
RESTfulAPI, SuiteBridge or another provider can never be selected implicitly by
this planner.

`BackendNode.online` is not a planner input. The normalized planning state is a
snapshot derived from current lifecycle/Agent authority and is never persisted
as a competing source of truth.

## Exact planning input

One planning request carries:

- the complete current `TimerIntent`, including exact `timerIntentId` and
  `intentRevision`;
- the requested assignment role (`primary`, `replica` or `replacement`);
- the current durable assignments for that same TimerIntent;
- at most 32 uniquely identified backend candidates.

Each candidate carries:

- stable `backendId` and optional `siteId`;
- the current authoritative `backendGeneration`;
- normalized current backend state;
- server-side write-allowed evidence;
- one opaque current execution-authority fence;
- Timer create/readback capability evidence with exact generation and revision;
- Timer-domain health evidence with exact generation and revision;
- channel-mapping evidence with exact generation, mapping source and revision;
- bounded conflict evidence.

No wall-clock lookup, random value, provider discovery, database query or
network request occurs inside `planTimerAssignment()`.

## Fail-closed eligibility

Eligibility is a hard gate. A candidate is excluded when any required fact is
missing, stale, ambiguous, generation-mismatched or denies safe Timer
execution.

The first policy version requires all of the following:

- backend state is `online` or `degraded`;
- backend write policy permits execution;
- an explicit current execution-authority fence exists;
- current backend generation is non-zero;
- capability evidence is current, belongs to the exact current generation and
  supports both Timer create and authoritative readback;
- health evidence is current, belongs to the exact current generation and says
  Timer writes are operational;
- channel mapping is current, unambiguous, generation-matched and identifies a
  backend-native channel;
- a canonical channel requirement, when present, matches exactly;
- conflict evidence is `confirmed_clear`;
- the backend is not excluded by the TimerIntent;
- replica diversity requirements are satisfied when applicable.

`degraded` backend/health state is allowed only when the Timer-domain health
fact still explicitly says Timer writes are operational. Generic backend
degradation therefore never implies Timer executability by itself.

Missing, partial, unavailable or stale conflict evidence is fail-closed in
policy version 1. A later policy that deliberately accepts bounded conflict
risk requires a new policy version and separate review.

## Deterministic ordering

Slice 5 deliberately does not introduce a weighted dynamic scheduler score.

The complete deterministic ordering is:

1. hard eligibility gates;
2. explicit `preferredBackendIds` order from the TimerIntent;
3. stable lexical `backendId` order for candidates in the same preference
   class.

Input candidate order has no effect.

The existing `TimerAssignmentDecisionEvidence.decisionScore` stores only the
ordinal preference marker:

```text
decisionScore = -preferenceRank
```

The first preferred backend therefore has score `0`, the second `-1`, and so
on. All ordinary backends use rank `32` and score `-32`; their final tie-break
is only the stable backend ID. There are no tunable weights, load heuristics,
randomness or current-time inputs in this slice.

If explicit preferred backends are all ineligible and an ordinary backend is
selected, the decision records `preferred_backends_ineligible`. This makes the
fallback visible and auditable instead of silently changing authority.

## Primary, replica and replacement rules

For role `primary`, an existing active primary assignment immediately yields
`unassigned` with `active_primary_exists`. The planner cannot produce a second
active-primary selection and cannot bypass the repository's durable
single-active-primary invariant.

Replica planning is accepted only when the TimerIntent explicitly requests more
than one assignment and marks simultaneous recording intentional. The planner
then respects:

- desired assignment count;
- required backend diversity;
- required site diversity.

Missing site evidence under required site diversity fails closed.

Replacement planning is accepted only when `allowFailover` is true and excludes
a backend already used by an active assignment. A selected replacement is only
planning evidence; it does not perform handover, supersede an old owner or
authorize native dispatch.

## Selected and unassigned output

A selected decision carries:

- exact TimerIntent ID and revision;
- assignment role;
- `timer-assignment-planner/1` policy version;
- selected backend and exact generation;
- selected channel binding and mapping revision;
- selected capability revision;
- selected health revision;
- bounded reasons, warnings, exclusions and conflict facts;
- deterministic candidate evaluations in policy order.

An `unassigned` decision carries no selected backend, generation, channel,
capability or health target. It retains explicit reasons and bounded exclusion
evidence.

Malformed requests use the separate `invalid` outcome and cannot be treated as
a scheduling result.

## Persistence handoff

The planner does not construct a durable `TimerAssignment` because assignment
identity, revision and epoch have different owners.

A later scheduling service may translate a selected or unassigned plan into a
repository create request only after revalidating the exact current
TimerIntent/assignment evidence. `TimerAssignmentRepository` remains the sole
issuer of:

- initial and successor `assignmentRevision`;
- monotonic per-intent `assignmentEpoch`;
- the durable single-active-primary race fence.

Repeated planning with identical inputs therefore produces structurally
equivalent decision material but cannot create duplicate durable assignments by
itself.

Any later persistence/execution handoff must preserve the existing
`intentRevision`, `assignmentRevision`, `assignmentEpoch`, backend-generation
and ADR-0042 revision fences. Slice 5 does not relax them.

## Regression contract

The focused planner regression proves:

- structurally identical decisions for identical inputs;
- input-order-independent stable backend ordering;
- explicit preferred-backend ordering;
- excluded-backend rejection;
- offline, stale and incompatible backend rejection;
- generation-stale capability evidence rejection;
- missing Timer capability rejection;
- missing/stale health rejection;
- missing/stale and generation-stale channel mapping rejection;
- no selection without explicit current execution authority;
- explicit `unassigned` when no safe candidate exists;
- no second active-primary selection;
- explicit primary-versus-replica behavior;
- backend and site diversity;
- visible non-preferred selection when preferred candidates are ineligible;
- bounded decision evidence for the maximum 32-candidate input;
- stale conflict evidence rejection;
- operator-review fail-closed behavior;
- replacement requiring explicit failover permission and a different backend.

The architecture guard also rejects SQLite, repository, Agent command,
SuiteBridge, RESTfulAPI, SVDRP and runtime wiring in the planner slice and
integrates the planner checks into both `test-fast` and `test-architecture`.

## Runtime boundary

This slice changes no installed runtime path.

It adds no:

- scheduler daemon wiring or recurring scheduler job;
- TimerAssignment persistence call from the planner;
- `NativeTimerBinding` contract or persistence;
- actual failover or reconciliation executor;
- Agent Timer command;
- SuiteBridge Timer create/update/delete/toggle command;
- RESTfulAPI or SVDRP Timer mutation;
- public TimerIntent/TimerAssignment mutation API;
- broad Timer UI;
- account-management UI;
- `mutations=enabled` mode;
- Phase 65 or later runtime.

The account/backend access-management gate before broad Timer UI remains
unchanged and continues to build on the Phase-62 actor/session/RBAC foundation.

## Acceptance

Acceptance for Slice 5 is:

1. TimerIntent and TimerAssignment contract/repository regressions remain green;
2. focused deterministic planner regression passes;
3. Slice-5 architecture guard passes;
4. repository-wide architecture checks pass;
5. the complete hosted VDR-Suite CI graph is green on one exact Draft-PR head.

No real yaVDR acceptance is required because this slice changes no installed
daemon, Backend Agent, SuiteBridge, service configuration or native VDR Timer
path.
