# Phase 69.C — Durable TimerAssignment Native Specification

## Status

**ACTIVE — internal durability prerequisite for the first public Timer CREATE submission.**

Baseline:

```text
main=6d822db653093ab5f513b8106929cfcf02fa85af
PR #328 / CI #9108=ACCEPTED
69.C=ACTIVE
```

## Problem proven from the accepted runtime

PR #327 composed durable Timer CREATE preparation. PR #328 composed the accepted
reservation -> dispatch-state -> activation owners.

The remaining public admission must call the accepted preparation contract with
one exact `NativeTimerSpecification`.

The Phase-64 architecture already establishes:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerSpecification
  -> NativeTimerBinding
```

but the durable `TimerAssignment` row did not yet retain that selected native
specification. It retained backend generation, channel binding, capability and
health evidence only.

There is also no independent `NativeTimerSpecificationRepository` and no
canonical HTTP-layer mapping from `TimerIntentSchedule` to VDR day/weekday/time
syntax.

Therefore opening the public POST first would force one of two unsafe choices:

- expose backend/VDR-specific native Timer fields in the stable public API; or
- invent new scheduling/timezone/recurrence policy inside the HTTP layer.

This slice closes that internal durability gap instead.

## Domain ownership

`TimerAssignment` now has an optional typed desired native specification:

```text
desiredNativeTimerSpecificationPresent
desiredNativeTimerSpecification
```

The field is deliberately optional for compatibility with already durable
assignments created before this schema extension.

New selected scheduler decisions must provide a valid desired specification.
Unassigned decisions do not carry one.

The desired specification is internal Control-Plane state. It is not added to
the public TimerAssignment representation.

## Planner contract

Each `TimerAssignmentPlanningBackendCandidate` carries the exact desired
`NativeTimerSpecification` associated with that candidate's already selected
backend/channel execution path.

A candidate is ineligible when:

- the specification is missing;
- the specification is structurally invalid; or
- its native `channelId` does not equal the candidate's selected
  `backendChannelId`.

The deterministic planning decision copies the selected specification and the
scheduling/reassignment handoffs persist it with the resulting assignment.

This keeps the mapping decision at the scheduling boundary rather than moving
backend-native policy into the later public HTTP admission layer.

## Persistence

`timer_assignments` gains one additive column:

```text
desired_native_timer_specification TEXT NOT NULL DEFAULT ''
```

The value uses an internal versioned length-prefixed codec:

```text
timer-assignment-native-specification/1|...
```

The codec round-trips all fields of `NativeTimerSpecification` and accepts only
a structurally valid specification.

One column is intentional: the specification is an immutable decision payload,
not a second queryable resource model.

## Upgrade compatibility

`TimerAssignmentRepository::ensureSchema()` checks the existing table and adds
the column only when absent:

```text
ALTER TABLE timer_assignments
ADD COLUMN desired_native_timer_specification
TEXT NOT NULL DEFAULT '';
```

Existing rows therefore remain readable and are explicitly represented as:

```text
desiredNativeTimerSpecificationPresent=false
```

They are not silently reconstructed from incomplete historical evidence.

A focused regression starts from the pre-extension 24-column
`timer_assignments` schema, runs `ensureSchema()`, reads the old row and then
round-trips a new assignment containing a desired specification.

## Immutability

Once a desired native specification is present on an assignment, normal
assignment updates must retain the exact specification.

The repository rejects:

- present -> absent;
- absent -> present through ordinary update;
- any field change to an already persisted desired specification; and
- a specification whose native channel differs from the assignment's durable
  backend channel binding.

This means re-planning must create/reassign through the established assignment
ownership workflow rather than rewriting the native execution target under the
same assignment identity.

## Validation ownership

`nativeTimerSpecificationValid()` is pure structural validation. It is now
inline in `NativeTimerSpecification.h` so planner/repository boundaries can use
the single canonical validation rule without pulling fingerprint/readback
linkage into every persistence test.

`NativeTimerSpecification.cpp` continues to own fingerprint and observed-state
matching behavior.

## Replacement/failover

Controlled replacement uses the same
`TimerAssignmentPlanningDecision`. The replacement assignment therefore copies
the selected desired specification together with backend/channel/capability
evidence.

The existing generation, set-revision, old-native-outcome and ownership fences
remain unchanged.

## Public API boundary

This slice does **not** modify:

- `PublicApiRuntime`;
- public TimerAssignment JSON;
- `SecurityHttpGate` public TimerAssignment handling;
- POST routing;
- `If-Match`;
- `Idempotency-Key`;
- public operation submission.

The public v1 contract does not expose:

- `NativeTimerSpecification`;
- VDR day/weekdays/HHMM syntax;
- backend generation;
- assignment epoch;
- NativeTimerBinding identity.

## Acceptance

CI/build/architecture validation is sufficient for this slice.

The database schema changes additively, and the migration is exercised from the
legacy table shape. No preparation, command reservation, dispatch activation,
Agent command, SuiteBridge command or VDR-native mutation is invoked.

Real yaVDR acceptance remains required for the later slice that actually opens
and activates the public Timer CREATE path.

## Phase-69 guard

The focused guard proves:

- typed desired specification fields exist only on the internal assignment
  model;
- planner candidates reject missing/invalid/channel-mismatched specifications;
- selected planning decisions carry the specification;
- primary/replica scheduling and replacement handoff persist it;
- the repository has the additive migration and versioned codec;
- the desired specification is immutable under ordinary assignment updates;
- the legacy-schema migration regression exists;
- the daemon links the codec exactly once; and
- PublicApiRuntime/Security do not expose or activate this internal field.

## Next bounded slice

After acceptance, Phase 69.C can finally open the public Timer CREATE admission
against the durable assignment.

The public handler can now derive the native specification from the authorized
`TimerAssignment` rather than accepting backend-native fields from the client.

The next slice must still prove and implement:

- backend-scoped `timers.create` authorization before resource disclosure;
- required strong `If-Match` against the public TimerAssignment ETag;
- required `Idempotency-Key`;
- closed public request JSON with no native/VDR fields;
- Suite-owned operation and NativeTimerBinding identities;
- exact actor/backend/resource/action idempotency scope;
- prepare -> reserve -> claim -> activate orchestration without blind retry;
- `202 Accepted` plus
  `Location: /api/v1/operations/{operationId}`;
- exact `409`, `412` and `428` mapping from ADR-0048;
- real yaVDR acceptance of the exact candidate once the native path is activated.
