# Phase 69.F — Client Contract Matrix and Stable Integration Boundary

## Status

**CANDIDATE — fourth bounded 69.F slice.**

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

## Explicit debt that this slice does not hide

Two browser fallbacks remain separately classified:

```text
Timer:
  /api/vdr/timers/live
  -> /api/vdr/timers

Home EPG:
  /api/epg/now-next?from=-1
  -> /api/vdr/events
```

The Timer paths have distinct live/snapshot semantics. The EPG path is
Home/LiveTV-sensitive. Neither is an alias-cleanup candidate and neither is
changed here.

## Derived next public-runtime gap

The matrix exposes one enabling gap before external clients can cleanly use the
already-stable backend-scoped resources:

```text
GET /api/v1/channels?...        requires explicit backendId
GET /api/v1/timer-assignments  requires explicit backend
but:
GET /api/v1/backends           does not exist
```

The repository does have `GET /api/backends`, but 69.E classifies unversioned
routes as pre-v1 unless explicitly stabilized.

Therefore the first justified runtime candidate **after this matrix slice** is a
bounded read-only public Backend collection, subject to its own live audit of
identity, authorization, representation, ordering, pagination and failure
semantics. This document does not authorize or implement that resource.

## Guard contract

The dedicated architecture guard must fail when:

- a current base/Genre/Live-Remote Client API operation is not classified;
- one operation is classified twice;
- an unknown classification is introduced;
- the browser Client API starts consuming `/api/v1` without the matrix being
  intentionally advanced;
- the declared stable public resources disappear from the accepted runtime/docs;
- a currently absent `/api/v1/backends`, `/api/v1/recordings`,
  `/api/v1/program-events` or `/api/v1/search-timers` is silently invented;
- the two explicit deferred fallbacks disappear from their owning code without
  a dedicated successor slice;
- Browser-session routes are presented as a generic app authentication
  contract.

## Acceptance boundary

This slice changes documentation and architecture guards only.

It changes no daemon runtime, public route implementation, authorization,
native VDR behavior, SuiteBridge/Agent transport, Home lifecycle or LiveTV
playback. No real-yaVDR native-effect acceptance is required.

Acceptance requires the exact branch head to pass the complete repository CI
graph. Merge still requires explicit user approval.
