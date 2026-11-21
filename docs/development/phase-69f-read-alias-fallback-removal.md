# Phase 69.F — Same-Handler Read Alias Fallback Removal

## Status

**ACCEPTED — third bounded 69.F slice via PR #371.**

Accepted evidence:

```text
PR #371
accepted head=bc3c31df17f923fe0877068fa550bc802c50e8d1
merge/main=83bd7c3522d4cdaef6d24b529fc46c44994275b9
CI #9321 / run 36333516626 = SUCCESS (6/6)
```

Baseline:

```text
main=b7ec2a6cf44e5b6b3e4652370a73e27aa071e772
phase69=ACTIVE
slice=69.F
```

Binding architecture: [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md).

Accepted predecessor:

```text
PR #370
accepted head=eaa1cb5c715fdf7970058c6f9b290fe96264bf2d
merge/main=b7ec2a6cf44e5b6b3e4652370a73e27aa071e772
CI #9317 / run 36331179868 = SUCCESS (6/6)
```

## Live repository proof

After PR #370, four wrapper fallback call sites remained in
`web/frontend/api/client-api.js`.

Three are not alternate data sources. The Phase-69 route inventory and
`ApiRouter` prove that each pair enters the exact same handler branch:

```text
/api/vdr/overview
/api/vdr

/api/vdr/persons
/api/persons

/api/vdr/recordings/persons/search
/api/recordings/persons/search
```

Specifically:

- `/api/vdr` and `/api/vdr/overview` both call
  `vdrController_.getOverview()`;
- `/api/persons` and `/api/vdr/persons` share the same
  `personController_->searchPersons(...)` branch;
- `/api/recordings/persons/search` and
  `/api/vdr/recordings/persons/search` share the same recording-person search
  branch and backend normalization.

Retrying the alternate spelling after an arbitrary rejection therefore adds no
provider redundancy. It can only issue a second request after authentication,
authorization, parser, backend or server failure.

The fourth wrapper fallback is materially different:

```text
/api/vdr/timers/live -> vdrController_.getLiveTimers()
/api/vdr/timers      -> vdrController_.getTimers()
```

That pair is **not** a same-handler alias. It is intentionally outside this
slice.

A separate manual EPG fallback in `web/frontend/epg-cache.js` also remains
outside this slice because it is Home/LiveTV-sensitive and requires a dedicated
lifecycle proof.

## Decision

The bundled first-party browser now dispatches each proven same-handler read
operation once:

```text
GET /api/vdr/overview
GET /api/vdr/persons
GET /api/vdr/recordings/persons/search
```

`fetchClientRecordingPersons()` keeps its existing
`backendQueryOptions(options)` behavior, including mapping `backendId` to the
existing `backend` query parameter.

The generic single-path compatibility helper remains only for
`fetchClientTimers()`, where the two routes have distinct semantics.

## Compatibility boundary

This is a first-party client routing cleanup, not server alias retirement.

- `/api/vdr`, `/api/persons` and `/api/recordings/persons/search` remain
  accepted retained pre-v1 server routes;
- the corresponding `/api/vdr/...` spellings also remain retained;
- `deprecatedAliases` remains empty;
- no `Deprecation`, `Sunset` or successor `Link` metadata is added;
- no new `/api/v1` resource is invented merely to remove browser fallback
  probing;
- third-party clients are unaffected and may continue using either retained
  server alias according to the 69.E compatibility classification.

## Remaining fallback debt

After this slice exactly one wrapper fallback remains in
`web/frontend/api/client-api.js`:

```text
fetchClientTimers():
  /api/vdr/timers/live
  -> /api/vdr/timers
```

It remains because the routes are semantically different live/snapshot reads,
not same-handler aliases.

The separate manual EPG fallback remains:

```text
/api/epg/now-next?from=-1
-> /api/vdr/events
```

That path remains explicitly deferred. This slice does not reopen Home or
LiveTV lifecycle behavior.

## Regression contract

The focused browser runtime regression proves that:

- `fetchClientVdrOverview()` emits exactly one request on 503;
- `fetchClientPersons()` emits exactly one request on 503;
- `fetchClientRecordingPersons()` emits exactly one request on 503;
- the structured `VdrSuiteClientError` preserves status, code and request ID;
- recording-person backend and search query normalization is preserved.

Architecture guards prove that:

- exactly one wrapper `requestJsonWithFallback()` call remains;
- that remaining fallback belongs only to Timer live/snapshot;
- the three cleaned-up wrappers contain only their chosen primary route;
- the three server alias pairs remain same-handler aliases;
- Timer live and Timer snapshot still call different controller methods;
- the manual EPG fallback remains untouched;
- server deprecation state remains unchanged.

## Acceptance boundary

No daemon, native VDR, SuiteBridge, Agent, Home rail or LiveTV implementation is
changed. No real-yaVDR native-effect acceptance is required.

The candidate is accepted only after the exact branch head passes the complete
repository-required hosted CI graph. Merge still requires explicit user
approval.
