# Phase 64 Slice 11 — Backend-Neutral Native Timer Observation and VDR Mapper

## Scope

Slice 11 introduces the backend-neutral evidence envelope for one authoritative
present native Timer observation and the first explicit VDR adapter that creates
that evidence from the existing `VdrTimer` read model.

The slice is mapping/contract only. It does not persist a binding, reconcile
ownership, transition a `TimerAssignment`, dispatch a Timer command or mutate
VDR.

The dependency direction is deliberate:

```text
core/vdr VdrTimer
  + explicit backend/generation/time evidence
  -> VdrNativeTimerObservationMapper
  -> core/timers NativeTimerObservation
```

`core/timers` remains independent of the VDR adapter. Later backend-neutral
readback services can consume `NativeTimerObservation` without depending on
`VdrTimer` or any VDR provider implementation.

## Backend-neutral observation evidence

`NativeTimerObservation` lives under `core/timers` and contains only:

- `backendId`;
- `backendGeneration`;
- `backendNativeTimerId`;
- `observedAt`;
- `NativeTimerObservedState`;
- `observedFingerprint`.

`nativeTimerObservationValid()` is the canonical fail-closed validator for this
present-observation evidence. It requires bounded backend/native identities,
non-zero generation, positive observation time, a valid Slice-9 observed state
and an exact fingerprint match.

The value deliberately carries no ownership, assignment, binding revision,
drift, missing-state or operation-verification decision.

## Explicit generation authority

`VdrSnapshot` currently carries a backend ID but no backend generation, and
`VdrTimer` itself carries only native Timer fields. Slice 11 therefore does not
pretend that either value is sufficient authority for reconciliation.

Every VDR mapping call must receive explicitly:

- `backendId`;
- non-zero `backendGeneration`;
- positive `observedAt`;
- one copied `VdrTimer` with non-empty native Timer ID.

The mapper never reads global backend state and never substitutes a "current"
generation. The caller must already have generation-fenced lifecycle evidence.

## Native Timer identity

`VdrTimer::id` becomes `backendNativeTimerId` only inside the explicit
`backendId` scope.

The mapper rejects an empty or oversized backend identity and an empty or
oversized native Timer identity. It does not use title, channel name or schedule
similarity as identity evidence.

## Copied domain fields

The mapper copies only the fields already admitted by
`NativeTimerObservedState`:

- `channelId`;
- `eventId`;
- `title`;
- `directory`;
- `day`;
- `weekdays`;
- `startTime` / `endTime`;
- `flags`;
- `priority`;
- `lifetime`;
- `enabled`;
- `vps`;
- `recording`;
- `pending`.

The fingerprint is generated only through
`nativeTimerObservedStateFingerprint()`, then the complete envelope is validated
through `nativeTimerObservationValid()`.

This keeps Slice 9 as the canonical definition of material native observation
fields and HHMM normalization while Slice 11 adds the backend/generation/time
evidence required to use such a read safely.

## Provider-private fields stay below the boundary

The existing `VdrTimer` additionally exposes display/provider details such as:

- `channelName`;
- `subtitle`;
- `aux`.

These are deliberately not copied into `NativeTimerObservedState` and therefore
do not affect the backend-neutral fingerprint.

In particular, changing opaque plugin `aux` data alone must not silently change
the scheduler/reconciliation fingerprint. If a later provider needs one such
fact for compatibility, it must be promoted as a separately typed normalized
fact under an explicit contract review.

## Representation normalization

The mapper preserves the copied VDR representation, including timer times such
as `930` or `0930`.

The domain fingerprint still canonicalizes valid native HHMM representation, so
provider-equivalent zero-padding differences remain fingerprint-equivalent:

```text
930  == 0930
0    == 0000
```

This keeps the adapter honest about what it observed while preventing
representation-only drift.

## Fail-closed behavior

Mapping/evidence validation fails when:

- backend ID is empty or exceeds the bounded identity size;
- backend generation is zero;
- observation time is not positive;
- native Timer ID is empty or oversized;
- copied state violates the Slice-9 `NativeTimerObservedState` contract;
- fingerprint generation is empty or does not match the copied state.

No partially valid observation is returned as success.

## What the mapper/evidence does not infer

Slice 11 does not infer or set:

- `NativeTimerBindingOwnership`;
- `NativeTimerBindingDriftState`;
- `timerAssignmentId`;
- `nativeTimerBindingId`;
- `bindingRevision`;
- `lastVerifiedOperationId`;
- missing/absence state;
- TimerAssignment `bound` state;
- failover/replacement eligibility.

Those require persisted prior state plus reconciliation and/or operation
evidence. A present native observation is evidence, not an ownership decision.

## No repository write

Neither `NativeTimerObservation` nor the VDR mapper includes or calls
`NativeTimerBindingRepository`.

This split is intentional:

1. adapter mapping proves one native read can be represented safely;
2. a later backend-neutral readback application service may combine that
   evidence with current binding/assignment/operation state;
3. only that later service may decide whether a repository create/update is
   appropriate;
4. reconciliation and mutation remain further downstream.

This prevents a read adapter from becoming an implicit ownership writer.

## Regression contract

The focused test proves:

- explicit backend ID, generation, native ID and observed time round-trip;
- the resulting backend-neutral observation passes `nativeTimerObservationValid`;
- material VDR fields copy exactly;
- the mapper-generated fingerprint equals the domain fingerprint;
- `930` and `0930` preserve distinct copied text but produce equal fingerprints;
- changing `channelName`, `subtitle` or `aux` does not change the fingerprint;
- changing a material field such as `enabled` does change the fingerprint;
- invalid HHMM fails through the domain contract;
- empty native ID fails;
- empty backend ID fails;
- zero backend generation fails;
- non-positive observation time fails.

## Architecture boundary

The VDR mapper includes the backend-neutral `NativeTimerObservation.h`, not
`NativeTimerBinding.h`. Consequently the existing Slice-9 guard remains fully
strict: no `core/vdr` exception is required at all.

The Slice-11 guard additionally rejects:

- SQLite or repository access;
- TimerAssignment planner/scheduling dependencies;
- Agent command dependencies;
- ownership or drift classification in the mapper/evidence contract;
- SuiteBridge/SVDRP mutation paths;
- `mutations=enabled`.

## Runtime boundary

`mk/vdr-sources.mk` uses an explicit source list and this slice does not add the
new mapper to it. The mapper is therefore not linked into the daemon merely by
existing under `core/vdr/src`.

This slice changes no installed runtime behavior and requires no real yaVDR
runtime acceptance. A focused real yaVDR compile/test is still useful portability
evidence.

## Next bounded work

After this evidence contract and mapper are accepted, the next safe slice is a
backend-neutral readback application service under `core/timers`.

It can receive `NativeTimerObservation` directly and combine it with
`NativeTimerBindingRepository` state. Initially it should only persist
observation refreshes that do not destroy unresolved drift evidence; changed or
missing native state must be surfaced for reconciliation rather than silently
overwritten. Implicit adoption, assignment `bound`, replacement and native
mutation remain separate later slices.
