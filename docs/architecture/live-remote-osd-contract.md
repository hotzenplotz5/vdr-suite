# Live Remote, Overlay and Legacy OSD Compatibility Contract

## Navigation

- [Architecture Index](index.md)
- [Current State](../CURRENT.md)
- [RESTfulAPI Integration](restfulapi-integration.md)
- [ADR-0030](../adr/ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0046](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047](../adr/ADR-0047-legacy-osd-compatibility-bridge.md)

---

## Status

This document records the implementation audit and the first production contract for backend-neutral remote control and the live-TV overlay. It does not introduce a new ADR. The binding decisions remain:

- [ADR-0030: Domain-First UI over OSD Proxy](../adr/ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0046: Streaming Gateway and MediaSession Boundary](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](../adr/ADR-0047-legacy-osd-compatibility-bridge.md)

## Three separate responsibilities

The runtime keeps three concerns separate:

1. Media bytes belong to the future MediaSession and streaming-gateway runtime from ADR-0046.
2. The modern live-TV overlay is a Suite-owned structured read model.
3. The native VDR OSD remains a future compatibility surface for functionality without a Suite domain.

The SSE live transport is not a media stream. It carries sequenced state-change notifications only.

## Existing reusable path

| Concern | Existing owner | Reuse in this slice |
|---|---|---|
| Backend selection | `BackendRegistryService` | Every request carries and validates `backendId`. |
| Read-only enforcement | `BackendAccessPolicy` | Remote control is rejected before executor lookup. |
| Capability reporting | `VdrCapabilitySet`, `CapabilityResolver`, `CapabilityReportBuilder` | Adds `remote.control`, `live.overlay.read`, future `osd.view`, future `osd.control`. |
| Backend adapter pattern | Timer and recording action executor registries | Remote actions use a backend-keyed executor registry and service. |
| Backend transport | `IHttpClient` and `BasicHttpClient` | RESTfulAPI remains private behind the executor/provider boundary. |
| Suite read models | `VdrSnapshotReadService`, `SnapshotCacheService`, `EpgEventRepository` | Channel, timer and revision come from the Suite snapshot; present/following use an injected bounded lookup backed by the persistent Suite EPG cache, with snapshot fallback. |
| Change notification | `SnapshotChangeFeedService`, `LiveTransportService`, `SseLiveTransport` | `liveOverlay` is a changed domain; no second SSE stack is introduced. |
| Browser boundary | `VdrSuiteClientApi` | Browser code knows only `/api/vdr/...` Suite routes. |

## Public Suite API

### Execute a remote action

`POST /api/vdr/remote/actions`

```json
{
  "backendId": "default",
  "operationId": "remote-4711",
  "action": "volumeUp"
}
```

For a direct channel switch only:

```json
{
  "backendId": "default",
  "operationId": "remote-4712",
  "action": "switchChannel",
  "channelId": "C-1-1039-10376"
}
```

The public action names are a fixed Suite allowlist. Raw RESTfulAPI key names, `/remote/kbd`, `/remote/seq`, SVDRP commands, shell commands and plugin service names are not accepted.

Failure classes are deterministic:

- `validation`
- `backendNotFound`
- `permission`
- `capability`
- `executorUnavailable`
- `transport`
- `backendRejected`
- `backendFailure`

### Read the live overlay

`GET /api/vdr/live/overlay?backend=default`

The first snapshot contains only values backed by an implemented source:

- backend and snapshot revision;
- current channel identity from RESTfulAPI `/info.json`;
- channel number and name from the Suite channel snapshot;
- present and following events from the persistent, backend-scoped Suite EPG cache, with the Suite snapshot as a fallback;
- current-event timer and recording state from the Suite timer snapshot;
- audio explicitly unavailable with `muted` and `volume` set to `null`.

The startup snapshot intentionally excludes the large EPG event domain. The overlay therefore resolves only the current channel's now/next events through a backend-neutral lookup contract instead of forcing a full event snapshot refresh. `LiveOverlayService` does not link against SQLite or `EpgEventRepository`; the daemon wiring supplies the repository-backed implementation. The browser remains isolated from RESTfulAPI and from the EPG cache implementation.

There is no fictitious MediaSession field and no claim that media streaming is available.

RESTfulAPI `/info.json` emits the live channel only while the status monitor is in live-TV mode. During recording playback it emits a `video` object instead, so this first overlay contract reports the current channel as unavailable rather than guessing a channel or inventing playback state.

## RESTfulAPI adapter mapping

The upstream `vdr-plugin-restfulapi` implementation was checked directly. The Suite adapter maps normalized actions to the plugin's fixed key names, including:

| Suite action | Internal RESTfulAPI request |
|---|---|
| `up` | `POST /remote/up` |
| `ok` | `POST /remote/ok` |
| `channelUp` | `POST /remote/chanup` |
| `channelDown` | `POST /remote/chandn` |
| `volumeUp` | `POST /remote/volup` |
| `volumeDown` | `POST /remote/voldn` |
| `fastForward` | `POST /remote/fastfwd` |
| `rewind` | `POST /remote/fastrew` |
| `previous` | `POST /remote/prev` |
| `switchChannel` | `POST /remote/switch/<encoded-channel-id>` |

Channel IDs are encoded as one path segment. Browser data never becomes an unchecked URL fragment.

## Live change flow

The existing sequence and resynchronization contract remains authoritative:

1. A backend snapshot change or successful remote action creates a change-feed entry.
2. The entry retains `sequenceNumber`, `snapshotGeneration`, `backendId` and `changedDomains`.
3. Relevant status, channel, event and timer changes also mark `liveOverlay` dirty.
4. `LiveTransportService` publishes the entry through the existing SSE transport.
5. The remote frontend ignores duplicate sequence numbers and reloads the current overlay only for its selected backend and the `liveOverlay` domain.

No full overlay object is pushed through SSE.

## Permission and multi-backend behavior

Remote control requires all of the following on the server:

- a known and enabled backend;
- non-read-only backend access;
- `remote.control` capability;
- a registered executor for the same backend ID.

A read-only backend may still expose `live.overlay.read`; it can display Suite state but cannot be controlled. Frontend disabling is informational only. The server gate remains authoritative.

## Legacy OSD audit and compatibility preparation

### RESTfulAPI capabilities found

The current `/osd.json` implementation can return the current structured object for:

- `TextOsd` with title, message, menu items, selected item and color-key labels;
- `ChannelOsd` with channel and present event information;
- `ProgrammeOsd` with present and following event information.

The existing normalized remote-action domain can later carry the allowlisted navigation commands for an OSD controller. It does not expose raw key names, sequences or keyboard input and therefore does not prevent a future `LegacyOsdSession` boundary.

### Required properties not supplied by RESTfulAPI

The audited RESTfulAPI OSD endpoint does not provide:

- a reliable OSD epoch;
- monotonic OSD frame or state sequence numbers;
- push change notifications for OSD state;
- controller ownership or lease fencing;
- backend-generation fencing;
- viewer/controller policy;
- resynchronization tokens or delta continuity guarantees.

For that reason `osd.view` and `osd.control` are separate future capabilities and remain unavailable in this implementation slice.

### Adapter boundary

The future adapter order is:

1. Use RESTfulAPI for current structured OSD reads where it is sufficient.
2. Use the existing remote-action service for allowlisted controller input after an OSD session and lease gate authorizes it.
3. Add SuiteBridge functionality only for a demonstrated missing property such as epoch, sequencing, VDR-internal change observation or local controller fencing.

No SuiteBridge OSD extension is implemented by this work package.

## Explicitly out of scope

- media streaming gateway production runtime;
- MediaSession creation or playback lifecycle;
- LegacyOsdSession runtime;
- OsdControllerLease runtime;
- OSD frame/delta transport;
- OSD epoch and resynchronization runtime;
- browser, Windows or TV native-OSD renderer;
- speculative SuiteBridge OSD commands.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
