# VIDAA Recording R1 — Public client contract audit and implementation gates

Status: **R1 source audit completed; Public Recording API NOT IMPLEMENTED**.
Audit base: server `main` `b9fc52a975c7ed2bb1ec46339ab31326709dab43` (2026-10-09).
Independent client base: `vdr-suite-vidaa` `work/mu10f-vidaa-pairing-client` at `dc69e79b9c22366f199d4467c57fd38df96ebd0d`.

This is the contract-first implementation backlog for a **general Suite Public-v1 Recording resource** (TV, Kodi, mobile and desktop), not authorization to reuse private browser APIs. No existing API route, credential flow or productive daemon is changed by this document.

## Source findings and gap matrix

| Capability | Verified implementation / authority | Public-v1 availability | Required successor |
| --- | --- | --- | --- |
| Actor/device authentication | `core/security/include/SecurityHttpGate.h`, `DeviceCredentialAuthenticator`; `GET /api/v1/backends` is device-accessible | **Yes** for backend discovery | Reuse current device actor and backend-scope authorization; no browser session |
| Recording collection | `VdrRecordingQueryService`, `VdrRecordingQueryController`, `/api/vdr/recordings/query` | **No** | Public `Recording` collection read with authoritative stable opaque Suite identity and backend scope |
| Recording detail | `VdrRecordingQueryService::findRecordingById`, browser Recording detail and metadata/cache helpers | **No** | Public Recording item read; no filesystem/provider address |
| Folder/artwork/duration/progress | Recording cache, VDR metadata, Recordings 2 folder and artwork routes, ContinueWatching plane | **No unified public contract** | Authoritative fields or optional affordances only; avoid private cache/artwork URLs |
| MediaSession creation | `POST /api/media/sessions` + `RecordingMediaSessionController::createSession` | **Bounded media domain, not Public-v1** | Public session admission using Suite Recording ID; reuse canonical media issuance/route/worker; no duplicate playback owner |
| Stream security | `MediaAccessGrantAuthenticator` + `MediaGatewayHttpServer`; session-scoped media cookie | **Existing gateway mechanism, not proven on VIDAA/device admission** | Verify media-cookie behavior on TV and revoke/session stop. Never pass durable device secret to media URL |
| Client capabilities | `RecordingMediaSessionRequestParser` typed capabilities, `MediaPresentationSelector` | **Existing bounded media domain** | Stabilize public capability negotiation and advertise only measured device facts |
| Playback semantic contract | `MediaPlaybackContract`, `MediaPlaybackContractResponse`; canonical browser playback owner | **Existing media domain, not general public-v1 contract** | Define stable Public-v1 projection and stop/seek/track/admission semantics |
| Permissions | `SecurityHttpGate` protects legacy MediaSession POST with `media.recording.play`; ADR-0061 describes `recordings.view` and `recordings.stream` | **Public recording enforcement missing** | Explicit per-backend + future folder scope, authenticated device actor, audit, 401/403/404/503 distinctions |
| Device client | VIDAA `DevicePairingClient`, `PlaybackController`, fixture-only Recording selector | **Device pairing proven, Recording consumption missing** | Backends discovery first, then public collection, detail, playback |

The **Phase-69.F contract matrix** explicitly classifies `fetchClientRecordings` as `missing-public-v1` and the Recording cache/folder helpers as `first-party-private`. The accepted Phase-69.F Home/EPG hardening records why a public Recording identity must **not** be frozen from current backend `recordingId`: after a mutation it may change. `tools/check_phase69f_client_contract_matrix.py` and `tools/check_phase69f_home_epg_fallback_removal.py` currently guard this absence. Any successor must *intentionally* update their checks, the public inventory, capabilities and the client contract matrix alongside a genuine production route.

## R2 — Canonical identity before public list

ADR-0014 distinguishes mutable `RecordingAddress`, best-effort collision-prone `RecordingStableFingerprint` and `RecordingChangeFingerprint`. None alone is a durable public ID. `VdrRecordingQueryResultJsonSerializer` currently includes `path`, `recordingPath`, `backendNativeId`: this serializer **must not** be exposed as-is.

Implement a persisted **opaque Suite-owned** Recording ID mapping scoped to a canonical Backend ID with tested reconciliation. Required cases: identical scan; rename/move; same title and broadcast repeats; collision; import; copy; delete/restore; backend removal/re-enrollment; cache warming/stale/unavailable; restart. Ambiguous matches fail closed or get new identity instead of silently merging records. A material mutation must resolve the current native address on the server, never from a TV-supplied path.

Only after tests and real read-model wiring introduce a minimal read collection/item pair. The exact versioned path, item fields, pagination, revision policy and error codes must be finalized against ADR-0048 and added to both public discovery and the machine-readable matrix. The contract must expose `backendId`, `recordingId`, title, optional subtitle/description, start, duration/status and explicit optional artwork/progress affordances without leaking source, private path or token. Later folders are Suite-owned hierarchy, not client-visible filesystem traversal.

## R3 — Public playback admission/security

Reuse the **existing** `MediaSessionIssuanceService`, `MediaAccessGrantAuthenticator`, `MediaGatewayHttpServer`, `RecordingMediaSessionController`, typed capability negotiation and `MediaPlaybackContract`. Current legacy POST uses `media.recording.play`; new public admission must enforce the accepted `recordings.stream` policy, with separate `recordings.view` on browsing and backend/folder scope. Do not silently equate these permissions.

Existing `RecordingMediaSessionCreate.cpp` sets a cookie named `vdr_suite_media`, `HttpOnly; Secure; SameSite=Strict`, scoped to `/api/media/sessions/{sessionId}/`. The Gateway accepts that cookie or the dedicated media authorization header, validates the grant and active route on media requests. This offers a **candidate** native-media-element auth mechanism; it is *not* verified for the device actor, cross-origin configuration or VIDAA's media HTTP stack. Test the full origin and cookie transmission chain on the real TV. If the TV cannot present this scoped cookie, design a narrowly scoped, short-lived, revocable, session-bound alternative **without** embedding device credentials into URLs.

Session creation, stop, seek/restart and audio/subtitle selection must act on the **same canonical owner** and enforce actor ownership. `MediaPlaybackContract` controls playback mode, absolute timeline, presentation-base, seek mode and track support. Client capabilities are evidence, not authorization. Prefer Direct Play, then remux, then transcode as proven by the selected source+device capabilities.

## R4/R5 — TV acceptance

Do not claim real playback from `201 Created`, HTTP `200`, fixture tests or CI. Require actual Hisense picture+sound, pause/stop, server cleanup and media grant invalidation. Probe native HLS/fMP4/TS, cookies and remote/media keys on the installed VIDAA 9 device. Then implement truthful seek/resume and tracks. No second VIDAA playback authority.

## Deployment/compatibility gate

The accepted live daemon is `c9f9a116071cf4cf4679e7bd9f39d345cd963d4d`, whereas main contains its merged tree at `b9fc52a975c7ed2bb1ec46339ab31326709dab43`. No reinstallation is required merely for this R1 document or a standalone VIDAA read-only client change.

Before any **new public Recording API** is used by the TV:
1. record expected exact server commit and current installed/running daemon identity;
2. apply focused tests, new API contract tests and CI;
3. verify artifacts, reserve, stage identity, sealed deploy and rollback via canonical process;
4. obtain approval before a productive service restart;
5. verify live capabilities and exact runtime binary identity; only then run a real TV acceptance step.

## Vertical checkpoints

- **R1:** this verified gap audit; no fabricated public endpoint.
- **R2a:** real device-authenticated Public-v1 Backend discovery in VIDAA; no fake Recording entries in the installed productive view.
- **R2b:** durable canonical Recording ID persistence + authorized public collection/item + VIDAA consumption; acceptance is *real* titles on the Hisense.
- **R3:** recording detail and device-actor MediaSession admission with real player-safe, revocable media authorization.
- **R4:** real TV image/audio/stop/cleanup through the one VIDAA PlaybackController.
- **R5:** normalized seek/resume/progress/track/OSD and classified recovery.

The server-side identity and authorization acceptance of R2b is a required gate; it must not be bypassed with the pre-v1 query or path-derived IDs.
