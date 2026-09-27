# Phase 69.F — Home EPG Single-Route Hardening

## Status

**CANDIDATE — sixth bounded 69.F slice.**

Baseline:

```text
main=64798ee1bd51694d70a01432d6260b4f84dfa26b
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #373
accepted head=096a4d2f1e011b876f80a38ef7e7b66ca04f0d83
merge/main=64798ee1bd51694d70a01432d6260b4f84dfa26b
CI #9335 / run 36338659080 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0045 Canonical EPG Event Identity and Provenance](../adr/ADR-0045-canonical-epg-event-identity-provenance.md)
- [ADR-0014 Recording Identity Strategy](../adr/ADR-0014-recording-identity-strategy.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)

## Fresh live audit after PR #373

The accepted Backend collection deliberately left the next public runtime
candidate open. The fresh audit therefore rechecked every remaining
`missing-public-v1` client domain rather than inheriting a route name.

Two tempting large resource slices are not yet justified:

- **Recording**: the current read stack exposes `recordingId`, but repository
  acceptance evidence explicitly warns clients not to cache that identifier as
  durable after a Recording mutation. ADR-0014 also keeps backend address and
  fingerprints distinct from a durable Suite recording identity. Freezing
  `/api/v1/recordings` now would make an identity promise the productive read
  model does not yet prove.
- **ProgramEvent / EPG**: ADR-0045 defines `programEventId` as the future
  stable opaque Suite event identity, but the current productive EPG read
  surface still uses backend event/channel identity. Freezing
  `/api/v1/program-events` now would bypass the canonical-identity work.

That leaves a smaller, already-proven 69.F client-hardening defect.

## Root cause and architecture proof

`loadLiveNowNextEvents()` currently performs:

```text
GET /api/epg/now-next?from=-1
if any non-success:
GET /api/vdr/events
```

The live `ApiRouter` proves these are not same-handler aliases:

```text
/api/epg/now-next -> EpgController::getNowNext()
/api/vdr/events   -> VdrController::getEvents()
```

Therefore the second request is semantic data-source substitution, not an alias
compatibility adapter. Because it runs after any non-success, it can hide an
authentication, authorization, parser, backend or server failure behind a
different representation. ADR-0048 permits safe GET fallback only for an
explicitly proven unsupported-route migration case.

This path is Home/LiveTV-sensitive, so the fix is intentionally narrower than a
Home refactor.

## Bounded change

The browser keeps exactly one owner:

```text
GET /api/epg/now-next?from=-1
```

`loadLiveNowNextEvents()` uses the existing `fetchJsonOrThrow()` helper and,
on failure, still resolves to:

```json
{"events":[]}
```

That preserves the existing fail-soft presentation behavior while removing the
second network request and alternate semantics.

The server-side `/api/vdr/events` route remains retained pre-v1. It is not
deprecated or removed by this slice.

## Explicit non-scope

- No public-v1 Recording resource.
- No public-v1 ProgramEvent resource.
- No EPG schema or identity change.
- No Home rail rebuild, refresh-policy change or LiveTV lifecycle change.
- No server alias removal or deprecation declaration.
- No Timer live/snapshot decision.

The Timer live/snapshot fallback remains separately deferred because
`/api/vdr/timers/live` and `/api/vdr/timers` intentionally expose different
semantics. It needs its own dedicated decision.

## Regression contract

The slice adds a focused contract that proves one Home EPG request, canonical
route ownership, no `/api/vdr/events` fallback and preserved empty-event
failure behavior. The slice target also runs the accepted Now/Next artwork,
LiveTV hero and post-Phase-66 Home performance regressions.

Architecture guards keep the predecessor matrix and 69.E inventory
successor-aware: the historical 69.E baseline remains history, while current
runtime truth records only the Timer live/snapshot fallback.

No real yaVDR acceptance is claimed by this client-routing-only slice. Hosted
CI remains the acceptance gate before Ready for Review.
