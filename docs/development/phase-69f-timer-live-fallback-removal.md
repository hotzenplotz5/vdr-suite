# Phase 69.F — Timer Live Single-Route Hardening

## Status

**ACCEPTED — seventh bounded 69.F slice via PR #375.**

Accepted evidence:

```text
PR #375
accepted head=dfba6b0459c3521fe534900c03b383f539fa55ce
merge/main=19077e7110fdb8111aead43e15b6962767ac6234
CI #9339 / run 36343460569 = SUCCESS (6/6)
```

Baseline:

```text
main=fbaab982964c838e589dfce8adb62e14013e0e07
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #374
accepted head=03b4783a0cd9e53e65f9dad3e1c0bf611a35cb84
merge/main=fbaab982964c838e589dfce8adb62e14013e0e07
CI #9337 / run 36341522122 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)
- [Phase 69.F Same-Handler Read Alias Fallback Removal](phase-69f-read-alias-fallback-removal.md)
- [Phase 69.F Home EPG Single-Route Hardening](phase-69f-home-epg-fallback-removal.md)

## Fresh live audit after PR #374

After the Home EPG successor, the machine-readable client matrix contains
exactly one deferred browser route fallback:

```text
fetchClientTimers():
  GET /api/vdr/timers/live
  on any rejection:
  GET /api/vdr/timers
```

A fresh audit of the live Client API, router, controller and consumers proves
this is no longer justified compatibility behavior.

## Root cause and architecture proof

The two routes are not aliases:

```text
/api/vdr/timers/live -> VdrController::getLiveTimers()
/api/vdr/timers      -> VdrController::getTimers()
```

`getTimers()` serializes the snapshot read service. `getLiveTimers()` reads
the live VDR service. Its implementation already owns the one explicit
service-unavailable policy:

```text
if (liveService_ == nullptr)
    return getTimers();
otherwise
    liveService_->getTimers();
```

That is the correct layer for an intentional live-to-snapshot decision because
the server knows whether the live service exists. The browser cannot infer that
condition from an arbitrary rejected HTTP request.

The first-party consumers also require live/current semantics:

- `loadTimers()` renders “Lade aktuelle Timerliste direkt vom VDR...” and uses
  `fetchClientTimers()`;
- EPG Timer detail synchronization calls
  `fetchClientTimers({ cache: 'no-store' })` and stores the result in
  `epgLiveTimerCache`.

Retrying `/api/vdr/timers` after authentication, authorization, transport,
backend, parser or server failure can therefore hide the failure and replace
current-state semantics with a snapshot. ADR-0048 does not permit such
arbitrary-error route probing.

## Bounded change

The bundled browser now issues exactly one Timer-list request:

```text
GET /api/vdr/timers/live
```

The generic `requestJsonWithFallback()` helper is removed because this was its
last call site. A failed live request remains a structured
`VdrSuiteClientError` and is handled by the existing caller behavior.

Both server routes remain intact. In particular, the server-owned
`liveService_ == nullptr` snapshot fallback inside `getLiveTimers()` is not
changed.

## Compatibility boundary

This is first-party client hardening only.

- `/api/vdr/timers/live` remains a retained pre-v1 transition route.
- `/api/vdr/timers` remains a separate retained pre-v1 snapshot route.
- Neither route is deprecated or removed.
- No public-v1 native Timer-list resource is invented.
- Public TimerAssignment contracts remain non-equivalent and unchanged.
- No Timer mutation, SuiteBridge, Agent or native VDR behavior changes.

## Matrix successor state

After this slice:

```text
explicitDeferredFallbacks = []
stable public-v1 method/resource contracts = 8
derived next runtime candidate = pending-live-audit
```

Phase 69.F therefore has no remaining catch-all browser route fallback debt.
The next runtime/public-resource slice must again be derived from a fresh live
client-gap and identity/dependency audit.

## Regression contract

The focused browser regression forces a 503 structured backend error and proves:

- exactly one HTTP request is emitted;
- the request is `/api/vdr/timers/live`;
- no snapshot request is issued;
- status, stable error code and request ID are preserved;
- `cache: no-store` survives the wrapper.

Architecture guards additionally prove the distinct server owners, the
server-owned explicit service-unavailable snapshot decision, live Timer
consumer semantics, zero remaining deferred fallbacks and unchanged public-v1
resource count.

No real yaVDR acceptance is required for this client-routing-only slice because
it changes no daemon/native VDR execution path. Hosted CI on the exact head is
the acceptance gate before Ready for Review.
