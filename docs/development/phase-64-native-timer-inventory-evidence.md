# Phase 64 — Authoritative Native Timer Inventory Evidence

## Purpose

Native Timer absence is stronger evidence than a failed lookup or an empty
vector. This slice introduces a backend-neutral contract for one **complete**
native Timer inventory and a pure absence assessor.

The contract is intentionally not produced by the existing VDR snapshot path.
Today that path cannot prove completeness: `RestfulApiVdrAdapter::getTimers()`
collapses a non-200 HTTP result to an empty vector, and the current Timer mapper
also returns an empty vector when the expected Timer array is absent. Therefore
legacy `VdrSnapshot.timers.empty()` or `VdrService::getTimers().empty()` must
never be interpreted as proof that a native Timer was deleted.

## Complete inventory evidence

`NativeTimerInventoryEvidence` carries:

- exact `backendId`;
- non-zero `backendGeneration`;
- positive `observedAt`;
- explicit `NativeTimerInventoryCompleteness::complete`;
- the canonical complete set of backend-native Timer IDs.

The ID vector is bounded to 4096 entries. Every ID is non-empty and bounded to
160 characters. IDs must be strictly lexicographically increasing, which makes
the representation both sorted and duplicate-free.

An empty ID vector is valid **only** when all other evidence is valid and
completeness is explicitly `complete`. That is how a future trusted producer can
represent a successfully observed backend that truly has zero native Timers.

`unknown` completeness is deliberately invalid for absence assessment.

## Absence assessment

`NativeTimerAbsenceAssessmentRequest` binds the question to:

- exact backend ID;
- exact backend generation;
- exact backend-native Timer ID;
- positive `notBefore` fence.

`assessNativeTimerAbsence()` fails closed unless both evidence and request are
valid. It then requires exact backend, exact generation, and
`evidence.observedAt >= request.notBefore`.

Only after those checks does it inspect the canonical ID set:

- ID present -> `present`;
- ID absent -> `absent`.

Backend mismatch, generation mismatch, stale evidence and invalid evidence are
separate results. None is converted into absence.

## Producer authority remains deferred

This slice defines evidence and evaluation only. It does not make the legacy
RESTfulAPI/VDR snapshot path authoritative.

A future producer may mint `complete` evidence only after it can explicitly
prove all of the following for one read:

1. transport/request succeeded;
2. response status is accepted;
3. payload parsing succeeded rather than defaulting to an empty result;
4. pagination/coverage is complete, if the provider can paginate;
5. every admitted native Timer identity is valid;
6. the exact backend generation is still the authoritative generation;
7. `observedAt` describes that successful complete read.

The current `getTimers()` API shape loses several of these distinctions, so the
producer will require a new explicit result/envelope instead of blindly wrapping
the existing vector-returning call.

## Scope boundary

This slice adds no:

- VDR adapter or snapshot changes;
- repository/SQLite persistence;
- NativeTimerBinding missing-state write;
- operation-aware delete verification;
- external-delete classification;
- TimerAssignment transition;
- replacement/failover execution;
- Agent/SuiteBridge/RESTfulAPI/SVDRP mutation;
- daemon wiring;
- public Timer mutation API;
- `mutations=enabled`.

## Next bounded work

Add a VDR/provider-side complete-inventory producer with a failure-aware Timer
read result. It must preserve transport failure and parse failure separately
from a legitimate empty complete inventory and must receive an explicit current
backend generation rather than deriving generation from `VdrSnapshot`.
