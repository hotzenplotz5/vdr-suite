# Phase 64 — Failure-aware RESTfulAPI Native Timer Inventory Reader

## Purpose

Slice 15 defines when a complete native Timer inventory can prove presence or
absence. This slice adds a VDR/provider-side reader that can safely mint that
evidence from the RESTfulAPI `/timers.json` endpoint without reusing the legacy
failure-collapsing `getTimers()` path.

The reader is deliberately not wired into the daemon in this slice.

## Failure-aware read result

`RestfulApiNativeTimerInventoryReader` returns one of:

- `complete` — HTTP 200, the whole JSON payload is structurally valid, the Timer
  inventory is fully parsed, every Timer has a safe native identity, IDs are
  canonicalized without duplicates, and Slice-15 evidence validation succeeds;
- `invalidRequest` — backend identity, backend generation or observation time is
  invalid; no HTTP request is issued;
- `httpError` — the HTTP status is not 200; no inventory evidence is minted;
- `parseError` — payload structure, Timer identity or boundedness is unsafe; no
  inventory evidence is minted.

The result retains the HTTP status code for diagnostics but HTTP success alone
never implies completeness.

## Checked inventory parser

The reader uses a dedicated fail-closed JSON scanner rather than
`RestfulApiTimerMapper::parseTimers()`.

That separation is required because the legacy mapper is intentionally tolerant
for ordinary reads: malformed/missing inventory structure can collapse to an
empty vector and Timer objects without an ID can be skipped. Those semantics are
not acceptable for absence proof.

The checked scanner:

- validates the complete JSON value to end-of-payload;
- accepts the RESTfulAPI object form with one top-level `timers` array and the
  legacy-compatible root-array form;
- rejects a missing `timers` member in object form;
- requires every Timer-array item to be an object;
- rejects duplicate object keys;
- requires every Timer object to resolve a native identity from non-empty `id`
  or non-negative integral `number`;
- rejects malformed/truncated nested JSON instead of skipping it;
- rejects duplicate native Timer IDs;
- sorts the complete native ID set before producing evidence;
- caps JSON nesting at 64 levels;
- caps payload size at 4 MiB;
- relies on Slice 15 for the final 4096-ID and 160-character identity bounds.

A valid, explicitly complete empty `timers` array therefore remains distinct
from an HTTP error, malformed body, missing array, or partially identifiable
Timer list.

## Generation authority

The reader does not derive `backendGeneration` from `VdrSnapshot` because that
snapshot does not carry generation authority. The caller must supply:

- bounded backend ID;
- non-zero authoritative backend generation;
- positive observation time.

The reader copies those fences into `NativeTimerInventoryEvidence` only after a
successful complete read.

## Runtime boundary

The implementation is a new `core/vdr` source but is intentionally **not**
added to `mk/vdr-sources.mk`. The daemon therefore does not link or execute this
reader yet.

The legacy `RestfulApiVdrAdapter::getTimers()` and
`RestfulApiTimerMapper::parseTimers()` remain unchanged. This avoids changing
current UI/read behavior while the stricter evidence path is proven separately.

## Scope boundary

This slice adds no:

- daemon runtime wiring;
- NativeTimerBinding repository write or `missingSince` update;
- delete-operation verification;
- external-delete classification;
- TimerAssignment transition;
- replacement/failover execution;
- Agent/SuiteBridge/SVDRP Timer mutation;
- public Timer mutation API;
- `mutations=enabled`.

## Next bounded work

After this reader is accepted, add a backend-neutral absence-application service
that consumes current durable `NativeTimerBinding` plus one valid complete
inventory evidence value. Initially it should only persist authoritative missing
observation evidence without guessing whether the cause is an expected delete,
external delete or backend transition. Operation-aware delete verification and
external drift classification remain separate consumers.
