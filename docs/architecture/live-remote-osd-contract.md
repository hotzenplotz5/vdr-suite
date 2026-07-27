# Live Remote, Overlay and Legacy OSD Compatibility Contract

## Status

The backend-neutral RemoteAction and LiveOverlay runtime is implemented. PR #99 established the production contract; PR #110 completed the current mobile pressed-state and duplicate-dispatch behaviour.

Streaming and legacy OSD compatibility remain separate future domains:

- [ADR-0030: Domain-First UI over OSD Proxy](../adr/ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0046: Streaming Gateway and MediaSession Boundary](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](../adr/ADR-0047-legacy-osd-compatibility-bridge.md)

## Separate responsibilities

1. Media bytes belong to the future Phase 65 Streaming Gateway.
2. The current live-TV overlay is a Suite-owned structured read model.
3. Native VDR OSD compatibility belongs to the future Phase 66 isolated bridge.

The existing SSE transport carries sequenced state-change notifications, not media bytes or OSD frames.

## Implemented ownership chain

```text
BackendRegistryService / BackendAccessPolicy
  -> capability and read-only validation
  -> backend-keyed RemoteAction executor or LiveOverlay provider
  -> Suite-owned API runtime
  -> live-remote-client-api.js
  -> remote frontend module
```

Reused boundaries include:

- backend selection and stable `backendId`;
- server-enforced read-only policy;
- capability reporting (`remote.control`, `live.overlay.read`);
- adapter/executor registry pattern;
- private RESTfulAPI transport behind the executor;
- Suite snapshot/EPG cache for overlay state;
- existing change feed and SSE transport;
- `VdrSuiteClientApi` as the browser boundary.

## Public Suite API

### Remote action

```http
POST /api/vdr/remote/actions
```

Example:

```json
{
  "backendId": "default",
  "operationId": "remote-4711",
  "action": "volumeUp"
}
```

Direct channel switch adds an allowlisted `switchChannel` action and a validated `channelId`.

Public actions use a fixed Suite allowlist. Raw RESTfulAPI paths/key names, `/remote/kbd`, `/remote/seq`, arbitrary SVDRP/shell/plugin commands and unchecked URL fragments are not accepted.

Deterministic failure classes cover validation, backend lookup, permission, capability, executor availability, transport and backend rejection/failure.

### Live overlay

```http
GET /api/vdr/live/overlay?backend=default
```

The current snapshot exposes only implemented sources:

- backend and snapshot revision;
- current live channel identity from the private backend adapter;
- channel number/name from Suite snapshot state;
- present/following events from the persistent backend-scoped EPG cache, with bounded fallback;
- current-event Timer/Recording state from Suite snapshot data;
- audio explicitly unavailable where no implemented source exists.

It does not invent a `MediaSession` or claim streaming availability. During Recording playback, if the backend does not identify a live channel, the overlay reports it unavailable rather than guessing.

## PR #110 mobile interaction contract

The current frontend remote preserves all 35 hotspot actions and establishes:

- only the pressed key receives the `down`/pressed visual state;
- other keys remain visually raised and are not disabled during dispatch;
- one internal `actionInFlight`/busy guard rejects duplicate dispatch while a request is pending;
- pointer, keyboard, cancel, leave and blur release state is isolated per key;
- a completed/failed request clears only the originating key state;
- browser calls use `VdrSuiteClientApi` extension functions;
- scrolling remains possible around the full vertical Remote layout.

The in-flight guard is not authorization and does not replace server-side read-only/capability checks.

## Current remote asset caveat

Current `main` serves `vdr-remote-photorealistic.svg`; the SVG contains an embedded JPEG. Draft PRs #112 and #113 are competing old-base proposals:

- #112: pure self-contained SVG;
- #113: direct 360×1220 JPEG.

Neither draft changes the implemented action contract above. Select at most one after rebase, install-runtime validation and real-device mobile acceptance. Preserve the 35 hotspots, pressed-state, in-flight guard, Client API boundary and scroll behaviour.

## Private RESTfulAPI mapping

The adapter maps normalized Suite actions to private plugin requests such as:

| Suite action | Private adapter request |
| --- | --- |
| `up` | `POST /remote/up` |
| `ok` | `POST /remote/ok` |
| `channelUp` | `POST /remote/chanup` |
| `channelDown` | `POST /remote/chandn` |
| `volumeUp` | `POST /remote/volup` |
| `volumeDown` | `POST /remote/voldn` |
| `fastForward` | `POST /remote/fastfwd` |
| `rewind` | `POST /remote/fastrew` |
| `previous` | `POST /remote/prev` |
| `switchChannel` | encoded private switch route |

This table documents adapter evidence; browser code never sees or constructs these paths.

## Live change flow

1. Snapshot change or successful remote action creates a change-feed entry.
2. The entry retains sequence, snapshot generation, backend ID and changed domains.
3. relevant status/channel/event/Timer changes mark `liveOverlay` dirty.
4. existing SSE transport publishes the entry.
5. the frontend ignores duplicate/out-of-order sequence numbers and reloads only for its selected backend and `liveOverlay` domain.

No full overlay object is pushed through SSE.

## Permission and multi-backend behaviour

Remote control requires on the server:

- known/enabled backend;
- non-read-only access;
- `remote.control` capability;
- executor registered for the same backend ID.

A read-only backend may expose `live.overlay.read` while denying control. Frontend disabled state is informational; the server remains authoritative.

## Legacy OSD audit

RESTfulAPI can expose current structured Text/Channel/Programme OSD state, but does not provide the complete Phase 66 contract:

- reliable OSD epoch;
- monotonic frame/state sequencing;
- push change notifications;
- viewer/controller policy;
- controller lease and fencing;
- backend-generation fencing;
- resynchronization/delta continuity guarantees.

Therefore `osd.view` and `osd.control` remain future capabilities. The intended order is:

1. view-only OSD snapshots/frames;
2. ordered deltas and resynchronization;
3. authorized viewer sessions;
4. one fenced controller lease;
5. allowlisted/rate-limited input through the existing RemoteAction domain;
6. SuiteBridge extensions only for demonstrated missing native properties.

## Explicitly not implemented by the current Remote/Overlay runtime

- Streaming Gateway or MediaSession lifecycle;
- LegacyOsdSession runtime;
- OSD frame/delta transport;
- OSD epoch/resynchronization runtime;
- controller lease;
- arbitrary command tunnel;
- browser/TV native OSD renderer.

## Related documents

- [Current State](../CURRENT.md)
- [Post-Phase-61 Platform Runtime Closeout](../development/post-phase-61-platform-runtime-closeout.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)