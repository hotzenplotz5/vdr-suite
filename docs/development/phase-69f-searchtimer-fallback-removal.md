# Phase 69.F — SearchTimer Client Fallback Removal

## Status

**ACCEPTED — second bounded 69.F slice via PR #370.**

Accepted evidence:

```text
PR #370
accepted head=eaa1cb5c715fdf7970058c6f9b290fe96264bf2d
merge/main=b7ec2a6cf44e5b6b3e4652370a73e27aa071e772
CI #9317 / run 36331179868 = SUCCESS (6/6)
```

Baseline:

```text
main=587352abfb4ffb04628831a2fbc3b528be3c722b
phase69=ACTIVE
slice=69.F
```

Binding architecture: [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md).

Accepted predecessor:

```text
PR #369
accepted head=061b0f1909a228c57d88c3b53f4f50d0275b1bba
merge/main=587352abfb4ffb04628831a2fbc3b528be3c722b
CI #9314 / run 36328486580 = SUCCESS (6/6)
```

## Live repository proof

After PR #369 the browser client still contained seven SearchTimer fallback
call sites:

- list loading used the three-candidate sequence
  `/api/vdr/searchtimers/live` -> `/api/vdr/searchtimers` ->
  `/api/searchtimers`;
- discovery used `/api/vdr/searchtimers/discovery` ->
  `/api/searchtimers/discovery`;
- preview used `/api/vdr/searchtimers/preview` ->
  `/api/searchtimers/preview`;
- preview-cache refresh used
  `/api/vdr/searchtimers/preview/cache/refresh` ->
  `/api/searchtimers/preview/cache/refresh`;
- plan used `/api/vdr/searchtimers/plan` -> `/api/searchtimers/plan`;
- validate used `/api/vdr/searchtimers/validate` ->
  `/api/searchtimers/validate`;
- real-test used `/api/vdr/searchtimers/real-test` ->
  `/api/searchtimers/real-test`.

The Phase-69 route inventory proves that every listed two-spelling SearchTimer
pair is a retained **same-handler alias** in `ApiRouter`. The aliases are
server-side compatibility spellings, not independent providers.

The first candidate in the old list-loading sequence,
`/api/vdr/searchtimers/live`, is different: it is not a server route at all.
The browser therefore performed a guaranteed speculative failed request before
using the actual SearchTimer list resource.

Because `requestJsonWithFallback()` and the former multi-path helper react to
any rejection, the old behavior could also issue an alternate alias after
authorization, parser, backend or server failures. A bare `404 not_found`
cannot safely distinguish a missing resource from a missing route under
ADR-0048.

## Decision

The bundled first-party client now uses one existing primary SearchTimer path
per operation:

```text
GET  /api/vdr/searchtimers
GET  /api/vdr/searchtimers/discovery
GET  /api/vdr/searchtimers/preview
POST /api/vdr/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/plan
POST /api/vdr/searchtimers/validate
POST /api/vdr/searchtimers/execute
POST /api/vdr/searchtimers/real-test
POST /api/vdr/searchtimers
POST /api/vdr/searchtimers/update
POST /api/vdr/searchtimers/delete
```

The seven remaining SearchTimer fallback call sites are removed. Together with
the four mutation fallbacks removed by PR #369, the first-party SearchTimer
client no longer performs route fallback probing at all.

The unused `requestJsonWithFallbacks()` helper is removed, and the speculative
`/api/vdr/searchtimers/live` literal disappears from the client.

## Compatibility boundary

This is a **client-routing cleanup**, not server alias retirement.

- `/api/searchtimers...` server aliases remain accepted retained pre-v1 routes;
- `deprecatedAliases` remains empty;
- no alias receives `Deprecation`, `Sunset` or successor `Link` metadata;
- no `/api/v1/search-timers` resource is invented;
- third-party callers may continue using the retained server aliases according
  to the 69.E compatibility classification.

The bundled browser simply stops using alternate spellings as an error-recovery
mechanism.

## Remaining fallback debt

After this slice only four wrapper fallback call sites remain in
`web/frontend/api/client-api.js`:

- `fetchClientTimers()`;
- `fetchClientVdrOverview()`;
- `fetchClientPersons()`;
- `fetchClientRecordingPersons()`.

The separate manual EPG fallback in `web/frontend/epg-cache.js` also remains.

They are intentionally outside this slice. Timer live/snapshot semantics and the
Home-sensitive EPG path require dedicated proof before changing behavior.
Overview and person aliases can be evaluated as a later bounded same-handler
client cleanup.

## Regression contract

The focused runtime regression proves that list, discovery, preview,
preview-cache refresh, plan, validate and real-test each emit exactly one
request on a 503 and preserve the structured `VdrSuiteClientError`.

Existing PR #369 regressions continue to prove one-shot execute/create/update/
delete behavior.

The existing query-cache refresh security regression is changed to require one
failed primary request rather than a second alias request while preserving
backend scoping, POST semantics and CSRF header ownership.

Architecture guards prove:

- `requestJsonWithFallbacks()` is absent;
- `/api/vdr/searchtimers/live` is absent from the browser client;
- every first-party SearchTimer wrapper owns one `/api/vdr/searchtimers...`
  path and contains no alternate `/api/searchtimers...` route;
- all corresponding server-side alias pairs remain in the retained route
  inventory;
- exactly four non-SearchTimer wrapper fallback call sites remain.

## Acceptance boundary

No daemon, native VDR, SuiteBridge, Agent, Home rail or LiveTV implementation is
changed. No real-yaVDR native-effect acceptance is required.

The candidate is accepted only after the exact branch head passes the complete
repository-required hosted CI graph. Merge still requires explicit user
approval.
