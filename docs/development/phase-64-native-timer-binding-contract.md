# Phase 64 Slice 9 — NativeTimerBinding Domain Contract

## Scope

Phase 64 Slice 9 introduces the backend-neutral domain value contract for the
third distinct Timer concept required by ADR-0044:

```text
TimerIntent
  desired recording outcome

TimerAssignment
  durable backend ownership decision

NativeTimerBinding
  observed relationship to one backend-native Timer
```

This slice is **contract only**. It does not persist NativeTimerBinding values,
observe VDR at runtime, reconcile drift, dispatch an Agent command or mutate a
native Timer.

Binding architecture:

- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](../adr/ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [Phase 64 Slice 8: Replica TimerAssignment Scheduling Handoff](phase-64-timer-assignment-replica-scheduling-handoff.md)

## Identity boundary

`NativeTimerBinding` has its own stable Suite identity:

```text
nativeTimerBindingId
```

It is not interchangeable with:

- `timerIntentId`;
- `timerAssignmentId`;
- a VDR Timer number;
- a RESTfulAPI Timer ID;
- title/channel/time similarity.

A backend-native Timer identity is meaningful only with its backend scope and
the Agent/runtime generation that most recently confirmed the observation:

```text
backendId
backendGeneration
backendNativeTimerId
```

`backendNativeTimerId` is therefore never accepted as a global identity by
itself. `backendGeneration` must be non-zero.

The contract carries an opaque `bindingRevision` for later exact repository
optimistic concurrency. Slice 9 does not issue or persist revisions.

## Ownership classification

The canonical ADR-0044 ownership classes are represented exactly:

- `managed` — VDR-Suite owns the native Timer through an active assignment;
- `adopted` — an existing native Timer was explicitly linked to an assignment;
- `external` — visible native state remains outside scheduler ownership;
- `orphaned_managed` — managed provenance exists but the current assignment
  relationship is incomplete;
- `ambiguous` — ownership cannot currently be resolved safely.

`managed` and `adopted` require a non-empty `timerAssignmentId`.

`external` requires an empty `timerAssignmentId` and cannot claim a
`lastVerifiedOperationId`. An external Timer therefore cannot accidentally be
presented as an ADR-0042 verified managed result merely because it resembles a
managed Timer.

`orphaned_managed` and `ambiguous` deliberately allow the assignment identity
to be absent because that incompleteness is part of their meaning. The later
reconciler must resolve it; the contract does not invent a relationship.

## Copied observed native state

The binding stores copied normalized native Timer facts in
`NativeTimerObservedState`.

The fields deliberately mirror only the bounded facts needed for identity,
readback and drift reasoning:

- backend-native channel identity;
- optional event identity;
- title and directory;
- native day/weekday and HHMM schedule representation;
- flags, priority and lifetime;
- enabled, VPS, recording and pending facts.

This structure contains copied values only. It carries no VDR pointer, VDR lock
or adapter-owned lifetime.

### No `VdrTimer` dependency

`core/timers/NativeTimerBinding` does not depend on the existing `VdrTimer`
read-model type. `VdrTimer` remains an adapter/read boundary under `core/vdr`.
A later explicit mapper may copy authoritative observed facts from that boundary
into `NativeTimerObservedState`.

This prevents the Control-Plane Timer domain from depending on the VDR adapter
and preserves the ADR rule that raw VDR/native implementation details do not
become scheduler authority automatically.

### No plugin-specific `aux` field

Plugin-specific native `aux` content is intentionally absent.

ADR-0044 keeps plugin/provider-specific `aux` below the adapter/provider mapping
boundary. Slice 9 therefore does not promote opaque `aux` data into a public
cross-backend Timer domain field. If later reconciliation needs a provider-
specific compatibility fact, that fact must be normalized explicitly rather
than smuggled through an untyped blob.

## Versioned observed fingerprint

`observedFingerprint` is not arbitrary caller text. A valid binding requires it
to equal:

```text
nativeTimerObservedStateFingerprint(observedState)
```

Policy version 1 starts with:

```text
native-timer-observed-state/1
```

and uses length-delimited field encoding for normalized observed fields. Native
start/stop values are first canonicalized to four-digit HHMM text before they
are encoded. Thus adapter-equivalent `930` and `0930` describe the same
fingerprinted time even though the copied observation may preserve either
provider representation.

The fingerprint deliberately excludes Suite binding identity, assignment
identity and backend generation. It describes the normalized observed native
Timer fields, while those other fences remain explicit separate values.

Changing any material observed field, including `enabled`, produces different
fingerprint material. Representation-only zero-padding differences in valid
native HHMM text do not. A later persistence/reconciliation slice can therefore
compare normalized readback without pretending that title/time similarity is
authoritative identity or reporting false drift for provider formatting alone.

## Existing adapter compatibility

The contract follows the native representation already emitted by the current
read boundary instead of inventing a second stored formatting convention.

The current RESTfulAPI Timer mapper may omit `day` and converts native integer
start/stop values with `std::to_string()`. Therefore values such as `0`, `5`,
`930` and `2015` are valid native HHMM number text when their zero-padded
semantic value is a valid time.

Slice 9 consequently defines:

- `day` as optional bounded native text;
- `weekdays` as exactly seven letter-or-`-` positions;
- start/stop as non-empty one-to-four digit decimal native HHMM number text;
- HHMM semantics after conceptual left-zero-padding, so hour must be 0–23 and
  minute 0–59.

Examples:

```text
0     -> 0000 -> valid
5     -> 0005 -> valid
930   -> 0930 -> valid
2015  -> 2015 -> valid
1960  -> invalid minute
2460  -> invalid hour/minute
09:30 -> invalid native number text
```

The copied observation keeps the adapter-provided value; Slice 9 does not turn
it into TimerIntent absolute-time semantics. The fingerprint, however,
canonicalizes valid start/stop values to four-digit HHMM before encoding them.
This makes `0`/`0000` and `930`/`0930` fingerprint-equivalent while preserving
the original copied readback value for diagnostics and later provider mapping.

## Observed-state validation

The initial native observation contract is deliberately bounded and fail
closed:

- channel identity is required;
- event identity is optional but bounded;
- title and directory are bounded copied text;
- native day is optional and bounded;
- weekday representation follows the exact seven-position rule above;
- native start/stop number text follows the adapter-compatible HHMM rule above;
- flags are non-negative;
- priority and lifetime are in the native 0–99 domain;
- booleans remain explicit copied facts.

This is adapter-level native representation below the assignment boundary. It
is not reused as TimerIntent absolute-time semantics and is not used for global
Timer deduplication.

## Observation and missing-state semantics

`lastObservedAt` is the latest authoritative observation time represented by the
binding and must be positive.

`missingSince` is zero while the native Timer is currently represented as
present. A non-zero value records the first authoritative absence observation
and must not be later than `lastObservedAt`.

Absence is never stored as unexplained `drift=none`.

A missing binding must be classified as one of:

- `expected_transition` — absence is consistent with a known expected state
  transition such as an in-flight verified delete path;
- `external_delete` — managed/existing native state disappeared outside the
  expected transition;
- `ambiguous` — current evidence cannot yet distinguish the outcome safely.

`external_delete` without a non-zero `missingSince` is invalid.

This gives later uncertain-dispatch and replacement work an explicit native
absence fact instead of allowing failover from a transport timeout alone.

## Drift classification

The ADR-0044 drift classes are represented exactly:

```text
none
expected_transition
external_field_change
external_disable
external_delete
native_identity_changed
ambiguous
```

`external_disable` additionally requires the current copied observed state to
have `enabled=false` and the Timer to remain present. That prevents a drift
label from contradicting the observed state it supposedly describes.

Slice 9 does not define automatic drift actions. Classification and validity are
domain facts only. Accept/restore/release/reassign actions belong to later
reconciliation slices and any mutation still requires ADR-0042/ADR-0043.

## Verification evidence

`lastVerifiedOperationId` is optional evidence identifying the most recent
operation whose result was authoritatively verified by this binding.

A transport success is not enough to populate it. The later readback path must
establish the verified relationship first.

External ownership cannot carry a verified operation identity. Managed,
adopted, orphaned or ambiguous bindings may retain one when durable evidence
exists, but Slice 9 performs no operation lookup and no mutation.

## What the contract deliberately does not infer

The contract does not infer ownership or equivalence from:

- Timer title;
- textual channel name;
- start/stop time similarity;
- VDR Timer number alone;
- REST response success;
- cached `online` state;
- provider availability;
- an old backend generation.

Adoption remains an explicit authorized future operation. Approximate matching
can only become evidence requiring policy/operator review.

## Regression contract

The focused contract regression proves:

- valid managed/adopted/external/orphaned/ambiguous ownership shapes;
- managed/adopted require assignment identity;
- external ownership rejects assignment and verified-operation claims;
- deterministic versioned observed fingerprint generation;
- material observed field changes change fingerprint material;
- mapper-compatible empty `day` plus unpadded `0`/`930` native HHMM text is
  accepted;
- equivalent padded and unpadded native HHMM values produce the same normalized
  fingerprint;
- malformed native HHMM and weekday values fail closed;
- fingerprint mismatch invalidates a binding;
- backend generation zero and empty backend-native identity fail closed;
- explicit missing plus `external_delete` is valid;
- missing with `drift=none` is invalid;
- `external_delete` while present is invalid;
- expected missing transition is representable;
- `external_disable` must agree with `enabled=false`;
- future `missingSince` relative to `lastObservedAt` is invalid;
- exact opaque binding revision matching is fail closed.

## Runtime boundary

This slice changes **no installed runtime path**.

It adds no:

- NativeTimerBinding repository or SQLite schema;
- mapping from a live VDR snapshot;
- reconciliation service;
- assignment state transition to `bound`;
- replacement/failover handover;
- Agent Timer command;
- SuiteBridge, RESTfulAPI or SVDRP Timer mutation;
- native VDR Timer create/update/delete/toggle;
- daemon scheduler/reconciler loop;
- public Timer mutation API;
- broad Timer UI;
- `mutations=enabled` mode.

No real yaVDR runtime acceptance is required because installed daemon, Backend
Agent, SuiteBridge, service configuration and native VDR Timer paths are
unchanged.

## Next bounded work

The next safe Slice-9 successor is NativeTimerBinding persistence and exact
repository revision semantics. That repository can then support authoritative
observation/readback reconciliation without yet enabling native mutation.

Only after binding persistence and reconciliation can replacement safely use
verified native absence/unknown-outcome evidence. Native Timer writes still
come later through ADR-0042 and ADR-0043.
