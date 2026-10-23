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

## P1 recording-progress merged-source contract (2026-10-23; PR #524)

`GET /api/v1/recording-playback-sessions/continue-watching?backendId=...`
returns `{backendId, items: [{recordingId, positionSeconds, durationSeconds,
resumeSupport}], limit: 50}`. Without explicit administrator-created Device-to-Human Account binding this list is scoped to the Device's own Service actor. With an active binding it projects the existing Human Account actor's Continue Watching state only when the Device **and** Human Account still have the relevant exact-backend playback grants. Entries are reconciled against current recording truth and re-admitted through the existing Device playback authorization before projection. Missing/deleted or unpermitted recordings are omitted. No Human Account identity or stored Metadata/Artwork is duplicated.


`GET /api/v1/recording-playback-sessions/progress?backendId=...&recordingId=rec_...`
returns `hasProgress`, `positionSeconds`, `revision`,
`durationSeconds` (nullable) and `resumeSupport: "profile_dependent"`.
The response contains only public identities. The actual playback engine decides
whether its selected media profile implements resume; this read does not start it.

`POST /api/v1/recording-playback-sessions/progress` body:
`{"backendId":"default","recordingId":"rec_<32-lowercase-hex>","operationId":"unique","sessionId":"active-owned-session","positionSeconds":120,"expectedRevision":0}`. Nonzero positions require the current active, owned Device Recording Session for this exact recording. An explicit zero-position reset may omit `sessionId`.
A measured playback position is accepted only under live Device credential +
current backend play grant and the accepted public Recording resolver.
Zero resets Resume (revision remains persisted as a tombstone). A position
at/after a verified duration counts as completed. The backend-derived native
recording ID is never returned.

Each writer uses read-current-revision/compare-and-swap semantics. A stale
writer receives 409 with `currentRevision`; exact operation replay is idempotent,
but modified replays conflict. Browser-CSRF sessions and invented Human/Device
relationships cannot access these Device routes. The Device's own Service actor owns unlinked positions; an explicitly bound, active Human Account actor owns linked positions, subject to current Device and Human playback grants. This is an implemented **source-level** sharing mechanism, not verified cross-device hardware playback. The established Browser Continue Watching owner is reused, not replaced.

[PR #524](https://github.com/hotzenplotz5/vdr-suite/pull/524) merged on 2026-10-23 at `052350714f5b5904a969158925cb05d8f39aed78`; the merged source includes the production Device/Browser callback bindings, tests, OpenAPI and reference client. This does **not** prove the installed yaVDR build, a complete production-HTTP/Browser↔Device/VIDAA end-to-end run or the final combined API-v1 acceptance. See the [one central matrix](../docs/development/public-v1-final-hardware-acceptance.md).


### Public-v1 client start-at linkage

An explicit Device Playback CREATE may include
\`startPositionSeconds\` in the established
\`POST /api/v1/recording-playback-sessions\` request. The caller must
pass its real capabilities and request start-at voluntarily. The existing
MediaSession controller checks current VDR indexing, profile, source growth
and duration. Only index-ready HLS playback currently accepts a positive
start position; unsupported/nonready configurations receive 409 and a
position past the verified duration receives 422. A progress checkpoint
does not automatically create a media session and a media session does
not automatically update confirmed playback progress.

### Explicit Human Account binding for shared resume (P1 merged source)

The Device's canonical Service Actor is never a Human Account. An authorized
Browser administrator uses `GET /api/v1/devices/{deviceId}/playback-account`
to obtain `{deviceId, accountId, revision}` (absent: null/0), then
`POST` to the same resource with
`{"accountId":"account_<opaque>","expectedRevision":0}`. The browser
must send its normal authenticated Session and CSRF proof; a Device credential
cannot perform the binding operation. A subsequent `POST` with an empty
`accountId` and current revision explicitly unlinks, retaining the durable
revision fence. Approving Pairing does **not** bind its approver.

When linked, progress GET/POST and Continue Watching list project the existing
persistent state of the Human Account's explicit Actor, never a supplied
`actorId` or `accountId` in a Device request. The same authenticated Device
Session remains the MediaSession owner. To use the linked resume history, the
server revalidates the Device's active Actor, Device and Credential, current
`media.recording.play@backendId` grant, the active Human Account and its
current media playback grant, as well as per-Recording admission. Revoking
either grant, the Device Credential, the Device, the Account or the explicit
binding prevents subsequent linked progress operations. An unlinked Device
is restricted to its separate Service-actor history.

This behavior is **source-merged**, but complete current-HEAD production HTTP integration and Browser↔Device/VIDAA/real-yaVDR acceptance remain separately unverified; no client or hardware acceptance is implied.

### Existing Browser Continue Watching concurrency (P1)

The original CSRF-protected `POST /api/media/continue-watching` remains
backwards compatible. The bundled Browser frontend now calls its bounded,
authenticated `operation:"revision"` read for one internal Recording,
including retained zero-position tombstones. Subsequent sampled, pause/stop
and completed-position writes include `expectedRevision` and use exactly
the **same ContinueWatchingRepository optimistic compare-and-swap** as Public
v1 Device clients. A successful write returns a new revision; a competing
update returns HTTP 409 and blocks that playback owner's further progress
writes, rather than blindly replaying an obsolete position. A new playback
owner may establish a fresh revision at user-initiated start. The former
legacy request without `expectedRevision` is still accepted as a
migration-compatibility contract and is **not** claimed to be protected
against competing older clients.
