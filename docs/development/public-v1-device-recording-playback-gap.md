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
| Public recording identity | `core/recordings/include/PublicRecordingIdentityRepository.h` | `resolveOrCreate(backend,native)`, `find(backend,native)`, move/rebind/delete support | Backend-scoped internal reverse resolution **implemented in a new branch-only repository method**; a MediaSession admission adapter and fresh recording snapshot validation are still missing. **Do not** pass `rec_...` directly to a service expecting a different identifier |
| Media byte enforcement | `core/http/src/MediaGatewayHttpServer.cpp` | Authenticates media grant on each media GET and validates active route lease; supports progressive and HLS artifacts | Device-compatible entry, authenticated playlist/segment delivery and reverse-proxy path have not been verified |
| Media bearer transport | `core/http/include/MediaAccessCredentialHttp.h` | Gateway accepts `X-VDR-Suite-Media-Authorization: Bearer ...` or short-lived `vdr_suite_media` cookie with HttpOnly, Secure, SameSite=Strict | A native `<video>`/HLS request cannot assume custom API headers; cookie path/origin and gateway admission need explicit Device-session handling |
| Client capabilities | `web/frontend/recordings2-playback.js`, `app/src/playback-controller.js` in VIDAA repo | Web example requests HLS/fMP4, H264/AAC; VIDAA currently exposes an unbound player adapter | VIDAA must advertise capabilities measured on the TV; never assume browser profile support on Hisense |

## Concrete proposed next implementation slices (NOT implemented)

### 1. Canonical identity and authorization, no media bytes

- **Repository step implemented on the development branch:** backend-scoped
  `PublicRecordingIdentityRepository::findNativeForPublicId` is read-only,
  validates opaque `rec_` IDs and never returns a foreign-backend match.
  **Still missing:** an authorized MediaSession admission adapter must verify
  the returned native record against the current backend snapshot; it must
  reject stale, removed and wrong-backend IDs without disclosing the native
  identifier to the TV.
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

**Reverse identity unit-test target:** `make test-public-recording-identity-repository`.
The tests cover missing/wrong-backend IDs, moves, removal/reuse and persistent
reopening. These are identity tests, **not** a working playback gateway.

**Present state:** Recording browsing/details, genres and EPG Now/Next have
Public-v1 candidates, but playback is **not yet implemented for the TV**.
This document does not authorize production deployment or claim a
successful real recording play.

### Server-only canonical playback target resolution — implementation candidate

The isolated internal PublicRecordingPlaybackTargetResolver checks the opaque
rec_ identifier against the exact *previously authorized* backend, performs
a native-ID-to-current-cache-recording lookup, verifies the current backend
and native binding, and rechecks the binding after reading the snapshot.
Unlike the opaque ID, VdrRecording::id is the internal identity expected
by the existing media controller; it is never exposed to VIDAA.

The CI regression covers wrong backends, malformed IDs, stale/deleted
recordings, moved identities, mismatched snapshots and missing internal
recording IDs. This helper is NOT an authorization gate, MediaSession
operation, fresh-file/lease admission or protected byte-delivery route.
A future handler must FIRST authenticate the Device and enforce
media.recording.play on that backend, then validate a live recording source,
session lifecycle, route lease and media gateway delivery. No Public-v1
playback endpoint is enabled by this step.

### Authenticated device playback admission — internal test slice (2026-10-10)

The new `PublicRecordingDevicePlaybackAdmission` is a **server-only**
adapter composed from the existing `AuthorizationService` and
`PublicRecordingPlaybackTargetResolver`. It rejects browser sessions,
non-Device principals, missing canonical Device/credential identities,
revoked or expired identities, and unresolved grants. Only a freshly
authenticated Device Service principal with the **effective**
`media.recording.play` grant for the selected backend can reach the
backend-owned availability check and scoped Public-ID resolution.
`recordings.view` by itself is insufficient. The native media-service ID
remains internal; negative decisions return no recording/path. Tests cover
401-like unauthenticated decisions, 403-like permission denials, offline
backends, invalid and foreign IDs, identity moves and removals.

**Not yet wired to an HTTP handler:** these are internal admission statuses,
not asserted HTTP response codes. The future route must obtain a verified
`RequestSecurityContext` from the SecurityHttpGate's Device authentication
and persistence checks, audit the decision, and call this adapter before
invoking existing MediaSession issuance. Backend availability and permissions
must be rechecked at session issuance and on media access. This slice does
**not** create MediaSessions or authorize HLS bytes. There is still no
playable VIDAA Recording stream.

### Versioned media-byte path — guarded gateway support (2026-10-10)

The existing `MediaGatewayHttpServer` now recognizes an **additional**
`/api/v1/media/sessions/{sessionId}/hls/{artifact}` path (and its
Recording progressive forms), using the **same** session-bound
`MediaAccessGrantAuthenticator`, active route-lease check, artifact-name
validation, range checking and response handling as the legacy gateway.
Missing/mismatched/ended grants fail before any manifest, segment or stream
byte read. Malformed versioned media paths are rejected at the gateway,
not delegated to another web/static handler. CI now exercises the
versioned manifest/segment path with missing and mismatched credentials,
cross-session grants, invalid paths, method rejection and revocation.

**Important:** This does not grant any Device permission by itself.
A session-specific media grant remains necessary for *each GET*. The
versioned gateway accepts existing MediaAccessGrants, not Device
Authorization headers; issuing a Device-owned session and securely
transporting a short-lived grant to native VIDAA video/HLS requests are
still missing. No TV client URL may be activated before verifying the
HTTPS reverse-proxy prefix and cookie Path/origin on the actual host.

### Versioned media cookie Path builder (2026-10-10)

`MediaAccessCredentialHttp` now offers a **separate, opt-in** secure
`publicV1SessionCookie` / expiry pair alongside the unchanged legacy Web
cookie. They scope `vdr_suite_media` to
`<trusted external mount>/api/v1/media/sessions/{sessionId}/` rather than
`/api/media/sessions/{sessionId}/`. The external mount prefix must come
**only from trusted deployment configuration** (never client, Host or forwarded
request headers); malformed, ambiguous, encoded, traversal, CRLF or
oversized prefixes fail closed. Tests cover root deployment and the
`/vdr-suite` prefix shown in VIDAA's verified HTTPS API root, legacy-path
regression, expiry, malformed credential and cookie-injection cases.

This is **transport preparation, not yet an issuing Public-v1 endpoint**:
no Device can obtain a playable MediaSession through this helper alone.
The actual HTTPS proxy/media route and VIDAA native-video cookie behavior
must be verified on the real host before a Device-issuing handler is enabled.
No media credential is ever embedded into an HLS URL.

### Device-only HTTP authorization at proposed session-control path (2026-10-10)

`SecurityHttpGate` now recognizes POST
`/api/v1/recording-playback-sessions` as a **protected** Device-only
mutation. It requires cryptographically verified, persistently active Device
and Credential identities plus `media.recording.play@backend`; browser
sessions, missing permission, invalid/foreign backend scopes and anonymous
requests are rejected. The existing device-authentication regression now
checks these decisions, including the distinction from `recordings.view`.

**This is NOT an enabled playback endpoint:** no route handler, actual
MediaSession issuance, stop/read operation or TV client binding is registered
by this change. The security gate is the first HTTP defense; the future
handler must re-run `PublicRecordingDevicePlaybackAdmission`, validate the
current native recording and backend, issue a session using Suite-owned
services, return only canonical IDs, use the versioned cookie+gateway and
stop/cleanup safely. Until then a permitted POST remains unhandled and
no capability is advertised.

### Canonical Device session response after activation (2026-10-10)

`PublicRecordingDeviceMediaSessionResponse::afterActivation` now builds a
strict, versioned response for an **already activated, authorized** recording
session. The response exposes only the backend, canonical `rec_...` ID,
opaque `ms_...` session, supported presentation profile, expiry and an
HTTPS-mount-aware `/api/v1/media/sessions/{id}/...` media path; the short-lived
media credential is confined to `Secure; HttpOnly; SameSite=Strict` cookie
transport. It rejects unknown profiles, malformed IDs, expiry, credentials
and mount prefixes without emitting any token, native VDR path or legacy URL.
Regression tests cover HLS, direct TS, fMP4, root and `/vdr-suite` mounts,
missing/injected credentials and no-cache responses.

This is a response-construction primitive, **not an HTTP endpoint**, and does
not authorize or activate sessions by itself. On any response-construction
failure after issuing a bundle, its caller MUST end that bundle; the Device
session creation handler, ownership and revocation checks remain outstanding.

### Proxy cookie rewrite in deployed Suite Nginx (2026-10-10)

The repository's `packaging/nginx/vdr-suite.conf` currently applies
`proxy_cookie_path / /vdr-suite/;`. Response assembly therefore uses
**independent trusted configuration** for external video URL prefix and
internal cookie prefix: behind this nginx proxy use `trustedExternalPrefix`
`/vdr-suite` but empty `trustedCookiePrefix`, allowing nginx to rewrite
an internal `Path=/api/v1/...` exactly once. Without that rewriting proxy,
use `/vdr-suite` for both. Neither prefix may come from client-provided
Host/Origin/forwarded headers. Tests cover both cases. The issuer is not
wired yet and must choose verified deployment configuration, not guess.

### Shared runtime issuer, native identity recheck (2026-10-10)

The legacy RecordingMediaSessionController now provides a distinct
`createDeviceSession` entry point for a **pre-admitted canonical Public ID**.
It rejects client-provided native IDs, re-queries the live VDR service and
checks its `backendNativeId` against the admitted identity before selecting
the existing native source, ffprobe/presentation, MediaSession issuer and
HLS/progressive provisioning. On success it returns the canonical Public-v1
media response rather than legacy URLs/cookies; unsuccessful response
construction revokes the provisioned session. This does **not** register a
Public-v1 HTTP route by itself. Authentication and live Device permissions,
owner registration, revocation on media access, and TV player integration
must be wired separately.

### Every Public-v1 media GET reauthorizes (2026-10-10)

The MediaGateway now differentiates versioned and legacy media paths. A
versioned request cannot obtain HLS manifests, segments, progressive TS or
fMP4 with a MediaAccessGrant alone: a server-supplied
`PublicSessionAuthorizer(sessionId,actorId,backendId)` must also approve.
Missing callback fails closed; no client-supplied URL token can bypass it.
The daemon must next register an ownership/Device identity/revocation checker;
until that wiring, Public-v1 media delivery deliberately returns 403.

### HTTP Device creation, owner binding and stop (2026-10-10)

The daemon now registers POST `/api/v1/recording-playback-sessions` and
POST `/api/v1/recording-playback-sessions/{ms-id}/stop` through a
Device-only `SecurityHttpGate`, an independent PublicRecording admission,
existing native MediaSession issuer, and a bounded runtime Device owner map.
The owner map retains actor/device/credential/backend and re-reads current
persistent Actor, Device, Credential and play grants on *each* HLS manifest,
segment and progressive media request, including attempts to downgrade to a
legacy media path. A server restart invalidates all sessions via the
existing MediaSession recovery. Stopping uses native cleanup and expires the
versioned cookie. The VIDAA HTTP entry remains /vdr-suite/api/v1/... with
trusted static reverse-proxy configuration, never user headers.

This is a first integration candidate; only hosted CI and later real-Hisense
and proxy/media acceptance can prove that actual codecs and the native player
work. No server installation is authorized by this commit.
