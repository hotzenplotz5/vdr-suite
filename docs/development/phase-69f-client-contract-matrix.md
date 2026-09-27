# Phase 69.F — Client Contract Matrix and Stable Integration Boundary

## Status

**ACCEPTED — fourth bounded 69.F slice via PR #372.**

Accepted evidence:

```text
PR #372
accepted head=97739a82b59913edf268bc33beece1b074f58aec
merge/main=ca7ddce8c1f4dcd94960c87d49c87533c5b0630b
CI #9324 / run 36335165236 = SUCCESS (6/6)
```

The successor [Phase 69.F Public Backend Collection](phase-69f-public-backend-collection.md)
now stabilizes the Backend-discovery gap derived by this matrix.

Baseline:

```text
main=83bd7c3522d4cdaef6d24b529fc46c44994275b9
phase69=ACTIVE
slice=69.F
```

Binding architecture: [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md).

Machine-readable inventory:
[phase-69f-client-contract-matrix.json](phase-69f-client-contract-matrix.json).

## Problem proved by the live repository

The first three 69.F slices removed unsafe fallback behavior from the bundled
browser. That does **not** make the remaining browser HTTP surface a stable API
for TV, mobile, desktop or Kodi clients.

The live repository currently contains 56 base `fetchClient*` wrappers plus
the Genre and Live Remote Client API extensions. Their routes span several
different contract classes:

- retained pre-v1 transition routes;
- first-party cache/session/helper surfaces;
- accepted bounded domain planes such as Broadcast Companion;
- a much smaller explicitly stabilized `/api/v1` surface.

Treating all browser routes as public would freeze implementation details that
ADR-0048 explicitly keeps outside the public compatibility promise.

## Stable public-v1 surface today

The matrix records only contracts that the current repository has deliberately
stabilized:

| Method | Resource | Stable meaning |
| --- | --- | --- |
| GET | `/api/v1` | Public API discovery and compatibility metadata. |
| GET | `/api/v1/capabilities` | Public API capability/compatibility policy discovery. |
| GET | `/api/v1/backends` | Actor-filtered minimal Backend discovery with stable `backendId ASC` keyset pagination. |
| GET | `/api/v1/operations/{operationId}` | Durable public mutation-operation read. |
| GET | `/api/v1/timer-assignments?backend={backendId}` | Backend-scoped keyset TimerAssignment collection. |
| GET | `/api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}` | Backend-scoped TimerAssignment item read. |
| POST | `/api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}` | Preconditions + Idempotency-Key protected native Timer CREATE admission for an existing assignment. |
| GET | `/api/v1/channels?backendId={backendId}` | Explicit-source federated Channel collection with keyset pagination and partial-source metadata. |

No route is promoted by this slice. The matrix describes repository truth only.

## Why the existing browser wrappers are not migrated blindly

### Channels

`fetchClientChannels()` currently consumes `/api/vdr/channels`.
The accepted public `/api/v1/channels` contract is deliberately different:
it requires explicit backend sources, has stable keyset ordering/pagination and
reports partial-source state. It is therefore not a drop-in browser replacement.

`fetchClientChannelMoveAction()` has no public-v1 mutation equivalent.

### Timers

The browser Timer wrappers expose native/live Timer semantics.
Public v1 exposes Suite-owned `TimerAssignment` identity plus a specific
idempotent native CREATE admission on an existing assignment. Those are not the
same resource model.

### Capabilities

`fetchClientCapabilities()` reads the VDR/backend capability report.
`GET /api/v1/capabilities` describes the public API contract and compatibility
policy. Replacing one with the other would destroy semantics rather than migrate
a route.

### Backends

The successor slice now stabilizes `GET /api/v1/backends` as minimal,
actor-filtered discovery. It intentionally exposes only `backendId`, `name`,
`enabled` and `online`; provider/backend implementation type stays private.

The bundled browser still consumes the richer pre-v1 `/api/backends` shape,
including selector/write/capability presentation data and separate default/
snapshot helpers. Therefore `fetchClientBackends()`,
`fetchClientDefaultBackend()` and `fetchClientBackendSnapshot()` are **not**
drop-in migrated merely because public Backend discovery now exists.

Therefore the current browser can legitimately keep transition routes while
new independent clients consume only the explicitly supported public contract
for domains that are already stabilized.

## Client classification rule

Every current browser Client API operation is classified exactly once in the
machine-readable matrix as one of:

- **pre-v1-transition** — Suite-owned and usable by current first-party code,
  but not a stable external compatibility promise;
- **first-party-private** — cache, browser-session, presentation, helper or
  lifecycle support that external clients must not copy;
- **bounded-domain-plane** — an accepted Suite-owned domain plane whose current
  browser contract is not implicitly part of public `/api/v1`.

This is intentionally stricter than saying “the web UI can call it, therefore
an app can call it.”

## Browser, TV, mobile, desktop and Kodi boundary

For independent clients the rule is now explicit:

1. use `/api/v1` only for the domain resources that are listed as stable;
2. do not infer public support from a route used by the bundled browser;
3. do not depend on Agent, SuiteBridge, provider/plugin, RESTfulAPI or SVDRP
   shapes;
4. do not copy EPG-cache, Recording-cache, preview-cache or browser-session
   helper routes into a third-party/client SDK;
5. MediaSession, Broadcast Companion and Legacy OSD remain their owning domain
   planes unless a later public-resource slice explicitly integrates them;
6. human-readable problem text is never machine control flow; clients branch on
   HTTP status, stable problem `code` and structured fields;
7. mutations never gain speculative fallback/retry behavior merely because an
   older alias exists.

The same public contract is intended for browser, TV, mobile, desktop, Kodi and
automation clients. Client-specific presentation and player engines remain
outside the platform resource contract.

## Explicit fallback debt and successor status

No catch-all browser route fallback remains in the current Client API.

The former Timer fallback:

```text
/api/vdr/timers/live
-> /api/vdr/timers
```

is retired by the dedicated Timer successor slice. The routes remain distinct
pre-v1 reads: the live route is the browser owner, while
`VdrController::getLiveTimers()` keeps the explicit server-side decision to
return snapshot timers only when the live VDR service itself is unavailable.
An arbitrary HTTP/auth/backend failure no longer causes a second browser request
with different semantics.

The former Home EPG fallback:

```text
/api/epg/now-next?from=-1
-> /api/vdr/events
```

was Home/LiveTV-sensitive and therefore was not removed as generic alias
cleanup. Its dedicated successor slice proved that the two paths have different
server owners and retired only the arbitrary-error alternate-source retry while
preserving the canonical EPG route and fail-soft empty-event behavior. No
public-v1 ProgramEvent contract is implied by that client hardening.

## Derived Backend gap — successor status

The matrix originally proved that stable Channel and TimerAssignment resources
required explicit backend identifiers while public Backend discovery was
missing. That gap is now **stabilized by the successor Backend collection slice**:

```text
GET /api/v1/backends
```

The successor is intentionally not a declaration that legacy `/api/backends`
has become deprecated or that the bundled browser can switch paths unchanged.
Its representation and authorization semantics are narrower and public-client
oriented.

After Backend discovery, this matrix deliberately does **not** preselect another
public route. The next 69.F runtime candidate requires a **fresh live audit** of
remaining client gaps, dependency order and already-stable domain contracts.

## Guard contract

The dedicated architecture guard must fail when:

- a current base/Genre/Live-Remote Client API operation is not classified;
- one operation is classified twice;
- an unknown classification is introduced;
- the browser Client API starts consuming `/api/v1` without the matrix being
  intentionally advanced;
- the declared stable public resources disappear from the accepted runtime/docs;
- a currently absent `/api/v1/recordings`, `/api/v1/program-events` or
  `/api/v1/search-timers` is silently invented;
- the stabilized `/api/v1/backends` contract disappears or is treated as a
  drop-in legacy-browser replacement;
- the retired Timer live/snapshot fallback or generic
  `requestJsonWithFallback()` helper is reintroduced;
- the retired Home EPG alternate-source fallback is reintroduced;
- Browser-session routes are presented as a generic app authentication
  contract.

## Acceptance boundary

This slice changes documentation and architecture guards only.

It changes no daemon runtime, public route implementation, authorization,
native VDR behavior, SuiteBridge/Agent transport, Home lifecycle or LiveTV
playback. No real-yaVDR native-effect acceptance is required.

Acceptance requires the exact branch head to pass the complete repository CI
graph. Merge still requires explicit user approval.
