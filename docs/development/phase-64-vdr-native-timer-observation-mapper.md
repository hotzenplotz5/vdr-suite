# Phase 64 Slice 11 — VDR Native Timer Observation Mapper

## Scope

Slice 11 introduces the first explicit adapter boundary from the existing VDR
Timer read model into the backend-neutral `NativeTimerBinding` observation
contract.

The slice is mapping-only. It does not persist a binding, reconcile ownership,
transition a `TimerAssignment`, dispatch a Timer command or mutate VDR.

The dependency direction is deliberate:

```text
core/vdr VdrTimer
  + explicit backend/generation/time evidence
  -> VdrNativeTimerObservationMapper
  -> backend-neutral NativeTimerObservedState + fingerprint
```

`core/timers` remains independent of the VDR adapter. The VDR adapter depends on
the timer-domain observation value, not the other way around.

## Explicit observation envelope

`VdrSnapshot` currently carries a backend ID but no backend generation, and
`VdrTimer` itself carries only native Timer fields. Slice 11 therefore does not
pretend that either value is sufficient authority for reconciliation.

Every mapping call must receive explicitly:

- `backendId`;
- non-zero `backendGeneration`;
- positive `observedAt`;
- one copied `VdrTimer` with non-empty native Timer ID.

The mapper returns these facts together with the copied normalized state and
fingerprint in `VdrNativeTimerObservation`.

The mapper never reads global backend state and never substitutes "current"
generation. The caller must already have generation-fenced evidence before
calling the mapper.

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

The copied value is validated through `nativeTimerObservedStateValid()` and its
fingerprint is generated only through
`nativeTimerObservedStateFingerprint()`.

This means Slice 9 remains the single canonical definition of material native
observation fields and HHMM normalization.

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

Mapping fails when:

- backend ID is empty or exceeds the bounded identity size;
- backend generation is zero;
- observation time is not positive;
- native Timer ID is empty or oversized;
- copied state violates the Slice-9 `NativeTimerObservedState` contract;
- fingerprint generation unexpectedly returns empty.

No partially valid observation is returned as success.

## What the mapper does not infer

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
evidence. A pure snapshot mapper is not allowed to invent them.

## No repository write

The mapper does not include or call `NativeTimerBindingRepository`.

This split is intentional:

1. adapter mapping proves one native read can be represented safely;
2. a later readback application service may combine that evidence with current
   binding/assignment/operation state;
3. only that later service may decide whether a repository create/update is
   appropriate;
4. reconciliation and mutation remain further downstream.

This prevents a read adapter from becoming an implicit ownership writer.

## Regression contract

The focused test proves:

- explicit backend ID, generation, native ID and observed time round-trip;
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

Slice 9's architecture guard is narrowed, not removed: the only
`core/vdr` header allowed to include `NativeTimerBinding.h` is the explicit
`VdrNativeTimerObservationMapper.h` adapter boundary introduced by this slice.

The new Slice-11 guard additionally rejects:

- SQLite or repository access;
- TimerAssignment planner/scheduling dependencies;
- Agent command dependencies;
- ownership or drift classification in the mapper;
- SuiteBridge/SVDRP mutation paths;
- `mutations=enabled`.

No wildcard `core/vdr` permission is introduced.

## Runtime boundary

This slice is not wired into the daemon or Backend Agent. It adds only the mapper
value/API, implementation, tests, documentation and architecture guard.

It changes no installed runtime behavior and requires no real yaVDR runtime
acceptance. A focused real yaVDR compile/test is still useful portability
evidence.

## Next bounded work

After this mapper is accepted, the next safe slice is a readback application
service that receives already generation-fenced mapped observations and combines
them with `NativeTimerBindingRepository` state.

That service may persist exact observed-state updates but still must not perform
implicit adoption, replacement or native mutation. Assignment transition to
`bound` requires exact assignment/binding/readback consistency and remains a
separate controlled transition.
