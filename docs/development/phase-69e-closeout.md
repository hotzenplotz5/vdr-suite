# Phase 69.E Closeout — Compatibility and Deprecation Policy

## Status

**COMPLETED**

```text
69.E=COMPLETED
next=69.F - First-party and third-party client hardening
```

Phase 69 remains active. This closeout completes the compatibility/deprecation
policy vertical only; it does not declare Phase 69 complete and does not migrate
first-party clients ahead of 69.F.

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69 Public API Kickoff and Runtime Progress](phase-69-public-api-kickoff.md)
- [Phase 69.E Compatibility Policy Foundation](phase-69e-compatibility-policy-foundation.md)
- [Phase 69.E Retained Legacy Route Classification](phase-69e-legacy-route-classification.md)

## Completion decision

The live post-PR-367 audit found no remaining bounded 69.E runtime gap.

69.E had five roadmap responsibilities. The two accepted slices cover them
without manufacturing a deprecation target:

| 69.E requirement | Accepted evidence |
| --- | --- |
| Additive versus breaking schema rules | PR #366 publishes compatibility policy version 1: additive response evolution inside v1, breaking changes require a new major, request objects remain closed by default. |
| Versioned capability negotiation | PR #366 publishes `supportedApiMajors=["v1"]` plus versioned `public-api.compatibility-policy` and `public-api.deprecation-metadata` capabilities. |
| Alias retirement policy | ADR-0048 plus PR #367 require a stabilized canonical v1 successor and explicit semantic mapping before any legacy alias can move from transition to deprecated. |
| Deprecation headers/metadata | PR #366 publishes the lifecycle `supported -> deprecated -> sunset-announced -> removed` and the supported `Deprecation`, `Sunset` and successor `Link` metadata contract. |
| Compatibility matrix and contract tests | PR #366 guards public policy discovery; PR #367 machine-classifies the entire retained unversioned route set and guards that no unproven successor/deprecation appears. |

The classification baseline is intentionally:

- 124 route literals total;
- 6 declared public-v1 literals;
- 118 retained unversioned route literals;
- 27 proven same-handler alias groups / 54 alias-member literals;
- 64 standalone transition literals;
- 0 deprecated aliases.

No classified alias currently has a stabilized public-v1 successor. Therefore
`deprecatedAliases = []` remains the only truthful capability value at 69.E
closeout. SearchTimer aliases are not deprecated because
`/api/v1/search-timers` does not exist; `/api/v1/channels` is not declared a
drop-in successor for `/api/vdr/channels`; and
`/api/v1/timer-assignments` is not a successor for native
`/api/vdr/timers`.

## Accepted implementation chain

### PR #366 — compatibility-policy discovery

```text
PR #366
title         = Add Phase 69.E compatibility policy discovery
accepted head = e205be80f07c5e9f20c6b4a8747ae89b850708f2
merge/main    = 42c2d9fe0bf453f0dceddb77e85eaf4704ab6d1e
CI #9303      = 36314288241 / SUCCESS (6/6)
```

This slice made the ADR-0048 compatibility policy machine-readable through the
existing `GET /api/v1` and `GET /api/v1/capabilities` discovery resources.
It added no legacy-route deprecation or removal.

### PR #367 — retained legacy-route classification

```text
PR #367
title         = Classify retained Phase 69.E legacy routes
accepted head = 7a2ddd8ce24a9c2876f12b7aeea2da28bdc9f2cf
merge/main    = 11e6272604f7ed827f7671fcf85276bcebdce346
CI #9308      = 36316653141 / SUCCESS (6/6)
```

This slice classified every retained unversioned route in the existing
Phase-69 inventory authority, proved 27 same-handler alias groups, kept all
remaining 64 routes explicitly transitional and recorded the bundled client's
catch-all route-fallback debt without changing client behavior.

## First-party client boundary carried into 69.F

The compatibility policy now makes the remaining client problem explicit rather
than silently accepting it:

- `requestJsonWithFallback()` and `requestJsonWithFallbacks()` still retry
  alternate paths after arbitrary request rejection;
- there are 15 wrapper fallback call sites in
  `web/frontend/api/client-api.js`;
- four definite state-changing SearchTimer mutation paths still use route
  fallback;
- `/api/vdr/searchtimers/live` is still a speculative client probe, not a
  server route;
- the manual EPG GET fallback from `/api/epg/now-next?from=-1` to
  `/api/vdr/events` still falls back after any non-success response.

Those are 69.F migration/hardening concerns. 69.E does not change them and does
not pull broad first-party client migration forward.

## 69.E acceptance gate

| Gate | Result |
| --- | --- |
| Additive/breaking compatibility policy is public and machine-readable | PASS — PR #366 |
| Supported public API majors are explicitly discoverable | PASS — PR #366 |
| Deprecation lifecycle/header capability is public and testable | PASS — PR #366 |
| Unversioned routes remain transitional rather than silently stable | PASS |
| Every retained unversioned route is machine-classified | PASS — PR #367 |
| Alias retirement requires a real stabilized v1 successor | PASS |
| No alias is falsely deprecated without a successor | PASS — `deprecatedAliases = []` |
| Catch-all client fallback debt is explicitly inventoried | PASS — PR #367 |
| Home / Recording fanout / LiveTV / Timer/native ownership unchanged | PASS |
| Exact accepted-head hosted CI for both 69.E slices | PASS — 6/6 each |

## Acceptance boundary

PR #366 changes public HTTP discovery representation only. PR #367 is
classification/guard/documentation-only. Neither changes native VDR effects,
SuiteBridge/Agent transport, Home, Recording metadata fanout, LiveTV, Live
Preview, EPG ownership or Timer orchestration.

Therefore 69.E closes on exact-head hosted CI and architecture/docs guards. No
additional real-yaVDR native-effect acceptance is required.

## Next bounded runtime slice

```text
69.E - Compatibility and deprecation policy [COMPLETED]
  -> 69.F - First-party and third-party client hardening [ACTIVE]
```

69.F owns common client error representation, elimination of fallback probing
after arbitrary errors, migration of wrappers to stabilized v1 contracts where
those contracts genuinely exist, and a documented stable boundary for browser,
TV, mobile, desktop and Kodi integrations. It must not create v1 routes merely
to remove a fallback; each promoted domain still requires its own stable public
resource contract.
