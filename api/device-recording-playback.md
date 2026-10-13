# Device Recording playback and media tracks — source-linked developer guide

## Navigation

- [API entrypoint](README.md)
- [Device pairing and grants](device-pairing.md)
- [Device administration](device-administration.md)
- [Recording browse](recording-browse-epg.md)
- [Source-level Device playback contract](../docs/development/public-v1-device-recording-playback.md)
- [Current State](../docs/CURRENT.md)

## Scope and acceptance status

The Device Recording controller is implemented in [RecordingMediaHttpRuntime](../core/daemon/src/RecordingMediaHttpRuntime.cpp) using the existing media controller and [Device-owned session registry](../core/daemon/include/PublicDevicePlaybackRegistry.h). This source is present in the merged R2 baseline. It is **not** part of the historically accepted original Phase-69.F reference-client contract, and its presence does **not** establish installed-daemon or Hisense picture/sound/subtitle acceptance.

**The seven methods below are now modeled in [the partial OpenAPI 3.1 contract](openapi.json)**, expanding its audited subset to 43 operations / 32 paths. Trickplay remains experimental and without hardware acceptance. This guide remains a source-linked implementation map, **not** a universal playback compatibility promise, a full gateway/media-byte schema or hardware acceptance evidence.

## Authentication and ownership

A paired Device presents `Authorization: VDR-Suite-Device <credential>` and must retain a resolved `media.recording.play` permission grant on the requested backend. Authentication, Device/credential state, backend scope and current session ownership are server-owned. A Browser Session, legacy Basic credential, or a mere `recordings.view` grant is not equivalent Device authority.

The implementation guards active sessions, media fragments, track reads and subtitle writes; a stale/revoked or foreign session is not recoverable by simply reusing an old media URL. Native VDR recording paths, media credentials, workers and private provider URLs must not be returned to clients.

## Source-backed operation map

| Method | Relative path | Implemented boundary |
| --- | --- | --- |
| GET | `/api/v1/recording-playback-sessions/capabilities?appVersion=…&firmwareVersion=…&playerVersion=…&engine=mse` | Device-authenticated no-store player-profile readiness; strictly read-only, never transcodes or probes a Recording |
| POST | `/api/v1/recording-playback-sessions` | Device-owned create; validates client capabilities, public `rec_<32 lowercase hex>` Recording ID and backend grant, then issues a scoped MediaSession |
| GET | `/api/v1/recording-playback-sessions/{sessionId}` | Owner-only `ms_<32 lowercase hex>` Session read; bounded profile and index-derived duration/preparing projection, not native Recording identity or persistent position |
| GET | `/api/v1/recording-playback-sessions/{sessionId}/trickplay?seconds=N` | Experimental JPEG frame from a completed VDR recording index, scoped to the currently authorized owned Device MediaSession; 10-second samples, no real-TV validation |
| POST | `/api/v1/recording-playback-sessions/{sessionId}/stop` | Explicit owner-controlled teardown of session/process/lease/cookie |
| GET | `/api/v1/recording-playback-sessions/{sessionId}/tracks` | Owner-only canonical audio/subtitle inventory, stripped of the internal native Recording ID |
| POST | `/api/v1/recording-playback-sessions/{sessionId}/subtitles` | Device-only validated subtitle-track selection; returns WebVTT text for supported text tracks, or JSON for `off` |

Both track and status GETs are `no-store`; tracks additionally set `X-Content-Type-Options: nosniff`. The track projection rejects invalid envelopes or an internal body over 64 KiB. The subtitle route reconstructs a safe internal request from parsed fields, removes `Set-Cookie`, enforces `no-store` and `nosniff`, and rejects unexpectedly large (over 1 MiB) or mismatched successful MIME/body output.

Source error examples from the runtime are `400 invalid_playback_request` / `invalid_device_subtitle_selection`, `403 device_media_session_not_owned`, `404 media_session_not_found`, `409 recording_track_status_unavailable`, and `503 recording_track_projection_invalid` / `device_subtitle_delivery_invalid`. These are handler-level examples; the outer SecurityHttpGate may reject requests earlier, so do not model one universal status code from these branches alone.

## Client workflow boundaries

1. Pair and receive the Device credential; request explicit backend playback grants separately. Pairing does not grant content access by itself.
   At connection time the client can GET the exact player profile status. The server gets its Device ID from the authenticated context, not from the query. This only reads stored corroborated paths and does not start playback or test decoders. Unknown app/firmware/player combinations remain unverified and need explicit authorized tests.
2. For known corroborated profiles, supply optional `capabilityProfile: {appVersion, firmwareVersion, playerVersion, engine}` on the existing Device CREATE. The server binds this key to the authenticated Device and uses the stored exact paths in the existing selector. Missing or mismatched verified evidence returns 409/422 rather than downgrading invisibly. Omitting this opt-in field retains the pre-existing Device playback behavior.
3. Use the public Recording browse/identity contract to select a completed Recording and derive actual decoder capabilities from the client device. Create a Session only upon user action.
3. Use only the returned scoped `mediaPath` and media cookie for playback; do not expose credentials in the URL, manifest, logs or client storage.
4. Poll status when index-backed duration is not yet known. `playback.durationSeconds` may be `null`; `playback.resume.preparing` does **not** implement saved resume position.
5. Read `/tracks` only while the Device owns that active Session. An audio change is accomplished through verified session replacement using `audioTrackId`, not an unrestricted Device audio-update route.
6. Select supported text subtitles through the owned `/subtitles` operation. WebVTT/SubRip/ASS/MovText conversions are source-dependent; DVB bitmap/Teletext are **not** guaranteed WebVTT text. `off` disables selection.
7. On STOP/BACK or credential/grant revocation, close the session and release playback state. Never fall back to private Browser media endpoints.

The sample bodies, session cookie semantics, indexed-duration safety and media route details are in the [merged playback contract](../docs/development/public-v1-device-recording-playback.md). The underlying [index reconciliation notes](../docs/development/vidaa-index-status-public-v1-20261011.md) explicitly forbid treating VDR index generation as a read-only probe.

## Evidence and remaining work

- Routing/Device projection: [RecordingMediaHttpRuntime](../core/daemon/src/RecordingMediaHttpRuntime.cpp).
- HTTP permission classification: [SecurityHttpGate](../core/security/include/SecurityHttpGate.h).
- Device recording admission: [PublicRecordingDevicePlaybackAdmission](../core/recordings/src/PublicRecordingDevicePlaybackAdmission.cpp).
- Internal media/session controller: [RecordingMediaSessionController](rest/include/RecordingMediaSessionController.h).
- Historical track and VIDAA testing context: [VIDAA Device track document](../docs/development/vidaa-device-recording-tracks-20261011.md).

The OpenAPI subset and its scoped guard now cover these seven source-backed Device routes, with method/path and Device-gate anchors and negative drift tests. This is **not** exhaustive HTTP security-denial/runtime conformance, formal OpenAPI Schema verification or live playback acceptance. Remaining work includes real Device integration testing, media-byte gateway documentation and validating all deployed denial envelopes.

## Back

- [API entrypoint](README.md)
- [Documentation index](../docs/index.md)
