# Phase 69.F — Client Error and Mutation-Fallback Safety

## Status

**CANDIDATE — first bounded 69.F slice.**

Baseline:

```text
main=5e6d7f7ce2b30bacf9b4bb3d57de8dbc677eb3e1
phase69=ACTIVE
slice=69.F
```

Binding architecture: [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md).

## Problem proven from the live repository

The bundled browser client had two separate but coupled compatibility defects:

1. `requestJson()` converted every non-success response into a plain JavaScript
   `Error`. The HTTP status, stable machine-readable error code, request ID,
   correlation ID and parsed error payload were therefore unavailable to callers.
2. `requestJsonWithFallback()` and `requestJsonWithFallbacks()` retry the next
   path after any rejected request. The Phase-69.E inventory found 15 wrapper
   fallback call sites, including four definite state-changing SearchTimer
   mutation fallbacks.

The four state-changing paths are:

- `fetchClientSearchTimerExecute()`;
- `fetchClientSearchTimerCreateAction()`;
- `fetchClientSearchTimerUpdateAction()`;
- `fetchClientSearchTimerDeleteAction()`.

For each of those operations the live `ApiRouter` already owns the
`/api/vdr/searchtimers...` path and its retained `/api/searchtimers...` alias
in the same server-side route branch. A client-side second dispatch is therefore
not needed for current-server compatibility and is unsafe after an ambiguous
first result.

ADR-0048 also prevents a generic shortcut for the remaining read fallbacks:
`404 not_found` can mean a missing public resource **or** a missing route.
A bare 404 is consequently not proof that a fallback route should be probed.

## Bounded decision

This first 69.F slice does exactly two product things.

### Common client error representation

JSON and binary HTTP failures now use one `VdrSuiteClientError` shape with:

- `status`;
- stable `code` when supplied by the server;
- `requestId`;
- `correlationId`;
- the parsed `payload`;
- a human-readable `message` for presentation only.

Both the public Problem Details shape and the retained nested
`error.code/error.message/error.requestId` shape are normalized. Callers can
identify the representation through `VdrSuiteClientApi.isClientError()`.

Machine decisions must use structured fields such as `status` and `code`,
not the human-readable message.

### No state-changing SearchTimer path retry

The four proven state-changing SearchTimer wrappers now make exactly one request
to their existing `/api/vdr/searchtimers...` primary path. They no longer call
`requestJsonWithFallback()` and no longer dispatch the alternate
`/api/searchtimers...` alias after a failure.

This does **not** remove the server aliases and does not declare either spelling
deprecated. The retained aliases remain part of the 69.E transition inventory.

## Explicitly deferred

After this slice, 11 wrapper fallback call sites remain, plus the separate manual
EPG GET fallback in `web/frontend/epg-cache.js`.

They are not silently accepted as correct. They are deferred because each needs
a route-specific unsupported-route proof or a genuine stabilized replacement
before changing behavior. In particular:

- the speculative `/api/vdr/searchtimers/live` client probe remains known debt
  and is still not a server route;
- generic GET fallback must not hide authentication, authorization, backend,
  parser or server failures;
- `/api/epg/now-next?from=-1` -> `/api/vdr/events` remains untouched in this
  slice so Home/LiveTV behavior is not reopened without a dedicated proof;
- wrappers move to `/api/v1` only when a genuine public-v1 successor exists;
- no `/api/v1/search-timers` resource is invented by this slice.

## Regression contract

The focused runtime regression proves:

- public Problem Details preserve status/code/request/correlation identity;
- retained nested legacy errors normalize to the same client representation;
- binary request failures use the same representation;
- a 503 from each of the four state-changing SearchTimer wrappers produces one
  request only and surfaces the original structured error;
- primary route and POST semantics remain unchanged.

Architecture guards additionally prove that the four client mutation functions
do not contain their alternate alias or `requestJsonWithFallback`, while the
server-side same-handler alias pairs remain intact.

## Acceptance boundary

This is browser-client request/error behavior only.

It changes no daemon, native VDR, SuiteBridge, Agent, Home rail or LiveTV
runtime. No real-yaVDR native-effect acceptance is required for this slice.

The candidate is accepted only after the exact branch head passes the repository
frontend, fast, architecture, docs, packaging and make-audit CI graph. Merge
still requires explicit user approval.
