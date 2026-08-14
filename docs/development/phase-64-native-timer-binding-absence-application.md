# Phase 64 — NativeTimerBinding Absence Application

## Purpose

Slice 15 defines what a complete native Timer inventory can prove, and Slice 16
adds a failure-aware RESTfulAPI producer for that evidence. This slice adds the
backend-neutral application step that records **authoritative missing
observation evidence** on an existing durable `NativeTimerBinding`.

This service records the fact that the bound native Timer is absent. It does not
decide why the Timer is absent.

## Application contract

`NativeTimerBindingAbsenceApplicationService::apply()` accepts:

- one bounded `nativeTimerBindingId`;
- one valid `NativeTimerInventoryEvidence` whose completeness is already proven.

The service reloads current durable binding state from
`NativeTimerBindingRepository`; caller-supplied binding state is never trusted.
It then requires:

- exact backend identity;
- inventory generation not older than the durable binding generation;
- inventory observation time not older than the durable binding observation
  fence;
- the Slice-15 absence assessor to prove the exact backend-native Timer ID is
  absent from the complete inventory.

The service never treats HTTP failure, parser failure, an incomplete inventory,
or a stale generation as absence.

## Present inventory result

If the complete inventory still contains the bound native Timer:

- a binding with no existing missing evidence returns `present` with no write;
- a binding that was already marked missing returns `reconciliationRequired`
  with no write.

The second case is deliberately not auto-healed. A later reconciliation service
must decide whether the native Timer reappeared after an expected transition,
external edit, backend restart, or another state change.

## First authoritative absence

For the first proven absence the service preserves the last known present
`observedState` and `observedFingerprint`, then advances only:

- `backendGeneration` to the complete inventory generation;
- `lastObservedAt` to the complete inventory observation time;
- `missingSince` to that same first authoritative absence time.

Cause classification is conservative:

- existing `expected_transition` is preserved because it represents explicit
  operation context already present in durable state;
- every other first-missing state becomes `ambiguous`.

The generic absence application service therefore never invents
`external_delete`. The observation fact is ownership-neutral: an `external`
binding may also receive authoritative missing evidence, but its cause is still
recorded conservatively as `ambiguous` unless a separate classifier proves more.

## Repeated authoritative absence

When `missingSince` is already non-zero, a newer complete inventory that still
proves absence may refresh backend generation and `lastObservedAt` while
preserving:

- the original `missingSince`;
- the last known present state/fingerprint;
- `lastVerifiedOperationId`;
- the existing valid missing drift classification (`expected_transition`,
  `external_delete`, or `ambiguous`).

An exact generation/time replay returns `alreadyCurrent` and issues no new
binding revision.

## Optimistic concurrency

Persistence uses the exact binding revision loaded before evaluation and the
existing `NativeTimerBindingRepository::update()` fence. A concurrent writer
produces `repositoryConflict`; there is no hidden retry using the already
evaluated inventory evidence.

## Scope boundary

This slice adds no:

- direct SQLite access in the service;
- VDR/RESTfulAPI dependency;
- inventory producer/runtime wiring;
- operation-aware expected-delete contract;
- ADR-0042 operation lifecycle transition;
- external-delete classification;
- automatic clearing of missing state on present evidence;
- TimerAssignment transition;
- replacement/failover execution;
- Agent/SuiteBridge/SVDRP Timer mutation;
- daemon wiring;
- public Timer mutation API;
- `mutations=enabled`.

## Next bounded work

Define a separate operation-aware **expected absence** contract for native Timer
delete verification. It must bind operation ID, exact binding revision,
backend/generation/native identity and `readbackNotBefore`. Only that explicit
operation context plus complete inventory absence may verify a Suite-managed
delete. External-delete classification remains a separate reconciliation path.
