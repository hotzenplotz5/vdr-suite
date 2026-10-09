# Public-v1 Device recording playback — source-verified implementation gap

**Status:** implementation design and evidence audit, **no new playback route**.
**Date:** 2026-10-09. **Branch:** `work/r2-recording-folder-browse`.
**Purpose:** make an authorized VIDAA recording *playable* without reusing a
Web browser session or exposing VDR/Streamdev private media URLs.
Related accepted architecture: [ADR-0046](../adr/ADR-0046-streaming-gateway-media-session-boundary.md),
[ADR-0053](../adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md),
[ADR-0065](../adr/ADR-0065-human-account-profile-device-identity-boundary.md).

## Audited, existing owners

| Concern | Existing source | Verified contract | Public-v1 device gap |
| --- | --- | --- | --- |
| Browser play/start/stop | `web/frontend/recordings2-playback.js` | POST `/api/media/sessions` with browser CSRF and same-origin credentials, media profile negotiation, stop and resume | Not a device-credential-protected Public-v1 mutation or media delivery contract |
| Media session and recording read | `api/rest/src/RecordingMediaSessionCreate.cpp`; `api/rest/src/RecordingMediaSessionController.cpp` | Domain resolves recording using `VdrRecordingQueryService::findRecordingById`, probes source, starts existing MediaSession pipeline | Device actor authorization, backend scope and *canonical Public-v1 recording ID* binding must be checked **before** invoking this pipeline |
| Public recording identity | `core/recordings/include/PublicRecordingIdentityRepository.h` | `resolveOrCreate(backend,native)`, `find(backend,native)`, move/rebind/delete support | No verified reverse lookup `(backendId, publicRecordingId) -> native recording` on this interface. **Do not** pass `rec_...` directly to a service expecting a different identifier |
| Media byte enforcement | `core/http/src/MediaGatewayHttpServer.cpp` | Authenticates media grant on each media GET and validates active route lease; supports progressive and HLS artifacts | Device-compatible entry, authenticated playlist/segment delivery and reverse-proxy path have not been verified |
| Media bearer transport | `core/http/include/MediaAccessCredentialHttp.h` | Gateway accepts `X-VDR-Suite-Media-Authorization: Bearer ...` or short-lived `vdr_suite_media` cookie with HttpOnly, Secure, SameSite=Strict | A native `<video>`/HLS request cannot assume custom API headers; cookie path/origin and gateway admission need explicit Device-session handling |
| Client capabilities | `web/frontend/recordings2-playback.js`, `app/src/playback-controller.js` in VIDAA repo | Web example requests HLS/fMP4, H264/AAC; VIDAA currently exposes an unbound player adapter | VIDAA must advertise capabilities measured on the TV; never assume browser profile support on Hisense |

## Concrete proposed next implementation slices (NOT implemented)

### 1. Canonical identity and authorization, no media bytes

- Implement an explicitly backend-scoped *read-only* reverse resolution or
  lookup in the Suite-owned `PublicRecordingIdentityRepository`. Verify
  the returned native record still exists in the authorized backend snapshot
  and is the same canonical recording. Reject stale/rebound/cross-backend IDs.
- Add a **new versioned Public-v1 recording media-session operation** routed
  via the existing media session issuance/service; do not copy a session
  repository or recorder/ffmpeg state machine. Actor must be a validated,
  approved Device principal with effective playback permission for the
  requested backend/resource. Decide and document the permission key
  against the existing SecurityHttpGate registration before implementation.
  `recordings.view` by itself must not be assumed to grant playback.
- Explicit capability and authorization errors (401, 403, 404, 409, 503);
  one backend, one opaque recording ID, bounded request size and idempotency.
  Keep a single device-owned session or a documented bounded concurrency
  policy. No native paths, channel/provider URLs or raw media grants in logs.
- Separate stop/revoke semantics, per-actor owner checks and explicit
  revocation on Device credential/grant removal, session loss, backend
  generation change and app logout.

### 2. Versioned, guarded **media-plane** path

- Propose a versioned, same-origin media-plane URL rooted in Suite for the
  selected authorized MediaSession, rather than pointing the TV at the old
  Web-only `/api/media/sessions` route. Keep the existing
  `MediaGatewayHttpServer` and `MediaAccessGrantAuthenticator` as owners of
  each byte request; do not introduce an unprotected static/file handler.
- Bind every video manifest, segment, byte-range and track request to one
  active session/route lease. Test 401 after missing/expired grants, 403/404
  cross-session access, and 409 after revocation. Serve no upstream VDR
  file location or Streamdev URI.
- Determine actual public HTTPS deployment path
  `/vdr-suite/...` versus upstream `/api/...`. Current
  `MediaAccessCredentialHttp::cookiePath` constructs
  `/api/media/sessions/<id>/`; this **must be checked** against the
  external reverse proxy prefix. Do not assume Set-Cookie Path will apply
  after proxying. Choose a safe, scoped cookie strategy or an explicitly
  supported native media Authorization mechanism. Do not put credentials
  in playlist/segment URLs, query strings or localStorage.
- Validate CORS/origin/mixed-content behavior, HTTPS TLS trust on VIDAA,
  content type, range support, segment lifetime/cleanup and credential
  omission from diagnostics before any real-device rollout.

### 3. TV adapter and first recording smoke test

- Add a capability-gated device playback client using **only the approved,
  versioned** operation and media-plane contract, not the existing browser
  `VdrSuiteClientApi`/CSRF `/api/media/sessions` flow.
- Select only an actual loaded Public-v1 recording on the authenticated
  30-item page. Create one session after explicit OK; show preparing, ready,
  unsupported, forbidden and offline states without fake playback.
- Delegate to VIDAA's platform media engine behind the existing
  `playback-controller`; support OK/play/pause/BACK/stop, predictable return
  focus and owner cleanup. Probe H264/AAC, MPEG2/AC3, TS/fMP4/HLS on the
  *actual* Hisense before choosing transcoding/remux policy.
- Add recording seek/resume and audio/subtitle selection only once the first
  real recording is playable and the authorized session contract exposes
  those operations. Do not mark Live-TV or HbbTV complete based on a
  recording smoke test.

## Required acceptance evidence

1. Backend C++ tests: valid canonical ID, rebind/missing ID, wrong backend,
   authorized/unauthorized principal, invalid payloads, revocation,
   retry/idempotency, session creation/stop and cleanup.
2. Gateway tests: **each** manifest/segment/range unauthorized case,
   cookie path under actual HTTPS reverse proxy, expiry/lease invalidation,
   no private URL or credential in JSON/Location/manifest.
3. VIDAA Node tests: no POST before explicit play, no old `/api/media/*`
   request, wrong backend/stale item blocked, secure failure states, D-Pad
   restoration, no full recording catalogue and no implicit second session.
4. GitHub exact-head CI green on server and client branches.
5. Real yaVDR/Hisense after separate deployment authorization: play one
   existing recording first, then seek/stop/back, then growing recordings,
   codec variations and concurrency; verify byte-for-byte HTTPS assets,
   retain backup/rollback and never reset device pairing during updates.

**Present state:** Recording browsing/details, genres and EPG Now/Next have
Public-v1 candidates, but playback is **not yet implemented for the TV**.
This document does not authorize production deployment or claim a
successful real recording play.
