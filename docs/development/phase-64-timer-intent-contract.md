# Phase 64 Slice 1 — TimerIntent Domain Contract

## Status

Contract-only foundation for Phase 64. This slice defines the Suite-owned TimerIntent domain boundary required by ADR-0044. It does not create a TimerAssignment, NativeTimerBinding, scheduler, persistence schema, Agent command or native VDR mutation path.

## Why this is the first Phase-64 slice

Phase 63 established durable command delivery, fenced native execution, explicit local-provider ownership/selection and the generic protected-write safety contract. The next unsafe shortcut would be to route existing native Timer actions directly through that machinery and call the result multi-backend orchestration.

ADR-0044 forbids that shortcut. A backend-native VDR Timer is not the user's durable recording intent. SearchTimer and other automation providers may propose recording work, but they do not own the cross-backend scheduler and must not bypass durable TimerIntent state once Phase 64 runtime is active.

The first Phase-64 implementation therefore defines only the backend-neutral intent value contract and its immutable semantic identity.

## Ownership boundary

The Control Plane owns TimerIntent identity, revision, lifecycle and desired recording semantics.

The dependency direction remains:

```text
AutomationSource
  -> AutomationProposal
  -> TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

Only `TimerIntent` is introduced by this slice. The later concepts remain deliberately absent from runtime wiring.

A TimerIntent is not:

- a VDR Timer number or RESTfulAPI Timer ID;
- a `VdrTimerOperationRequest`;
- a TimerAssignment;
- a NativeTimerBinding;
- a SearchTimer definition;
- a backend capability report;
- an Agent command or protected-write reservation.

## Stable identity, revision and semantic identity

Three identities have separate jobs:

- `timerIntentId` is the stable Suite identity of the durable intent;
- `intentRevision` is an opaque optimistic-concurrency revision and must match exactly when used as a precondition;
- `timerIntentSemanticIdentity()` is a versioned, length-prefixed identity over the backend-neutral desired semantics used for exact equivalence/deduplication evidence.

The semantic identity starts with `timer-intent-semantic/1|`. Every field is length-prefixed so embedded `:`, `|` or similar text cannot create delimiter collisions.

The semantic identity does not contain `timerIntentId`, `intentRevision`, lifecycle timestamps or current state. Those describe the durable resource, not the desired recording semantics.

## Intent types

The canonical types are exactly:

```text
programme_event
manual_window
recurring_schedule
```

### `programme_event`

Targets one programme occurrence. It requires either a canonical `programEventId` or a complete backend-scoped event reference retained as explicit fallback evidence. A partial backend event reference fails closed.

### `manual_window`

Targets an absolute channel/time window. It must not smuggle programme-event identity or recurring semantics into the request.

### `recurring_schedule`

Targets an explicit recurring schedule. It requires a recurrence rule and remains distinct from a SearchTimer rule. SearchTimer may later propose occurrence TimerIntents; that does not make SearchTimer the TimerIntent owner.

## Intent lifecycle

The canonical states are exactly:

```text
draft
active
paused
satisfied
cancel_requested
cancelled
expired
failed
```

Only `active` is assignment-eligible. Terminal states are `satisfied`, `cancelled`, `expired` and `failed`.

The contract defines a fail-closed state-transition matrix. In particular:

- a `draft` may become `active`, request cancellation, or fail;
- an `active` intent may pause, become satisfied, request cancellation, expire, or fail;
- a `paused` intent may resume, request cancellation, expire, or fail;
- `cancel_requested` may become `cancelled` or `failed`;
- terminal states have no outgoing transition;
- self-transitions are not treated as state transitions.

A TimerIntent must not become `satisfied` merely because a future native Timer create command returns transport success. ADR-0044 requires policy-defined outcome evidence and, for managed native Timer writes, authoritative readback.

## Bounded desired recording semantics

`TimerIntentSpec` owns bounded backend-neutral desired semantics:

- owner actor;
- optional automation source and occurrence identity;
- optional canonical programme-event identity;
- optional complete backend event fallback evidence;
- canonical and/or source-qualified channel requirement;
- absolute start/stop instants plus timezone;
- recurrence rule only for recurring schedules;
- start/stop margins, priority, lifetime and VPS preference;
- directory and naming policy references;
- backend preference/exclusion policy and failover permission;
- explicit replica count and optional site-diversity requirement;
- duplicate-prevention and ambiguity-review policy.

Backend preferences and exclusions are bounded and mutually exclusive. Replica count is explicit and bounded; the default is one. More than one desired assignment is never inferred from an accidental duplicate.

## Fail-closed validation

The contract rejects malformed or ambiguous value objects before they can become durable orchestration inputs. Examples include:

- missing owner actor;
- incomplete source-qualified channel identity;
- incomplete backend event fallback identity;
- event identity on a manual or recurring intent;
- missing recurrence policy on a recurring intent;
- recurrence policy on a non-recurring intent;
- invalid or reversed absolute time window;
- missing timezone;
- duplicate backend policy entries;
- a backend simultaneously preferred and excluded;
- unbounded margins, priority, lifetime, identity text or policy text;
- zero or excessive replica count;
- invalid durable timestamps or empty resource revision.

Validation does not guess defaults from a backend-native Timer line, channel name, title or current default backend.

## Deliberate boundary

This slice intentionally does **not** add:

- TimerIntent persistence or repository APIs;
- TimerAssignment state or assignment epochs;
- NativeTimerBinding state or adoption;
- scheduler candidate selection, scoring or failover execution;
- SearchTimer-to-TimerIntent runtime conversion;
- public or browser TimerIntent APIs;
- Agent command types or command payloads;
- SuiteBridge Timer mutation commands;
- RESTfulAPI/SVDRP/native Timer execution;
- protected-write reservation persistence for Timer operations;
- native Timer create/update/delete/toggle;
- production reconciliation or authoritative readback execution.

Existing direct native Timer action paths remain compatibility/runtime code outside this new Phase-64 orchestration contract and are not silently reclassified as TimerIntent execution.

`mutations=disabled` remains unchanged for the new Agent/native command path.

## Validation

The slice has three binding validation layers:

1. focused C++ contract tests for type/state validation, revision fencing and semantic identity;
2. a static architecture guard that requires the contract markers and rejects premature Phase-64 production wiring;
3. registration in the normal `test-fast` and `test-architecture` graphs.

The semantic-identity regression explicitly proves that delimiter-bearing values cannot collide and that schedule, recording options, failover policy, replica policy and duplicate policy are part of desired semantics.

Because this slice is not linked into daemon, Backend Agent, SuiteBridge, packaging or installed services, it changes no installed runtime behaviour and requires no real yaVDR acceptance.

## Next bounded slice

After this contract is separately approved and merged, the next Phase-64 slice may add Suite-owned durable TimerIntent persistence and revision-controlled repository semantics. TimerAssignment, scheduler execution and native writes remain later slices and require their own bounded contracts and, when installed runtime changes, exact-head real-system acceptance.
