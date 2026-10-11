# Public-v1 Device Recording playback — first hardware candidate

**Status:** implemented as a branch-only candidate; no installed real-TV
acceptance. The original audit and security rationale are in
[the playback gap record](public-v1-device-recording-playback-gap.md).

## Trust and source of truth

VDR-Suite provides an authenticated Device control plane over
`/api/v1/recording-playback-sessions`, backed by the existing
`RecordingMediaSessionController`, `MediaSessionIssuanceService`, HLS
and progressive streaming runtimes. Public IDs are `rec_<32 lowercase hex>`
bound to a particular authorized backend. The server resolves one targeted
Recording through `PublicRecordingPlaybackTargetResolver`, validates its
native source internally, and never exposes native VDR paths.

A Device credential (`Authorization: VDR-Suite-Device <credential>`) and
a currently resolved `media.recording.play` grant for that backend are
mandatory. Browser cookies, Basic Auth and `recordings.view` alone cannot
create/stop sessions. Every public media GET additionally rechecks the
active Device, credential, owner, backend and current play-grant state,
besides the existing MediaAccessGrant + active route lease. The same
check protects a Device session if an attacker requests the *old* media
namespace. A revoked or restarted session fails closed. A bounded in-memory
owner registry allows at most four sessions per actor and 128 total; after
daemon restart existing sessions are terminated by native recovery.

## Start a recording

`POST /api/v1/recording-playback-sessions`

JSON body, supplied only after an explicit user action in the TV UI:

```json
{
  "backendId": "default",
  "recordingId": "rec_0123456789abcdef0123456789abcdef",
  "capabilities": {
    "protocols": ["hls", "progressive"],
    "containers": ["mpeg-ts", "fmp4", "mp4"],
    "videoCodecs": ["h264"],
    "audioCodecs": ["aac"],
    "supportsByteRanges": true,
    "maxVideoWidth": 1920,
    "maxVideoHeight": 1080,
    "maxAudioChannels": 2
  }
}
```

The capabilities above are an **illustrative shape**, never defaults that
the server or VIDAA may invent. VIDAA obtains its reported formats via
the native `video.canPlayType` interface and observed screen limits.

A successful `201` response has `Cache-Control: no-store`, a scoped
`Secure; HttpOnly; SameSite=Strict` `vdr_suite_media` session cookie,
and `mediaSession` fields `id`, `state=ready`, `backendId`, canonical
`recordingId`, `presentationProfileId`, `mediaPath` and `expiresAt`.
The media path is rooted at
`/vdr-suite/api/v1/media/sessions/{sessionId}/...`, restricted to
HLS `hls/master.m3u8`, protected progressive `recording/stream.ts` or
`recording/stream.mp4`, depending on the selected profile.
Every media fragment remains behind the Gateway.

The deployed Nginx configuration strips `/vdr-suite` and rewrites the
internal media cookie path exactly once with `proxy_cookie_path`. Nothing
may embed a media token into the URL, manifest, localStorage or logs.

## Stop and ownership

`POST /api/v1/recording-playback-sessions/{sessionId}/stop`

```json
{
  "operation": "stop",
  "backendId": "default",
  "sessionId": "ms_0123456789abcdef0123456789abcdef"
}
```

The Device and current grant must still own the target session.
The native RecordingMediaSessionRuntime performs process/lease cleanup;
the owner binding is released and the versioned media cookie expires.
An unknown or foreign session is never stopped by another Device.

## Device-only status read

`GET /api/v1/recording-playback-sessions/{ms-id}` is read-only and
requires the same verified Device principal that owns the active session.
The server reads the current persisted Session, active Device/Credential
identities and backend playback grant before returning
`mediaSession.id`, `state=ready`, `backendId` and
`presentationProfileId`. This does **not** return a native Recording ID,
MediaAccessGrant, provider URL, VDR path or HLS token. Unknown, stale,
revoked or foreign sessions return no session detail; no Browser session
or anonymous caller can inspect status. Results are `no-store`.
Session termination/restart is reported as unavailable, never as a
synthetic playable session. This is a read of server state, **not**
persistent timeline position or Resume management.

## Failure and limits

Malformed/publicly unknown IDs, unsupported source formats, missing
permissions, revoked credentials, unavailable cache/source and media
decoder limitations fail explicitly. The HLS media grant is a scoped
session bearer with a maximum six-hour issuance window and a configured
five-minute media-access idle timeout. The actual runtime profile/format
depends on the real Hisense decoder and source recording.

This first-play candidate intentionally **does not** implement server-driven
seek replacement, persistent progress, subtitles,
audio-track selection or live-TV playback. These remain later slices; no
legacy Web media endpoint is used as a fallback.

## Test and deployment boundary

The R2 GitHub Action runs Recording ID, Device/Browser security, session
registry, Cookie, Gateway manifest/segment and daemon build checks.
VIDAA Node CI covers contract validation, same-origin media paths, and
existing browsing/navigation regressions. A fully playable picture with
sound cannot be proven by CI.

**No production deployment has happened.** To perform the first real test,
follow existing backed-up deployment/rollback runbooks, first verify HTTPS
cookie path and exact installed assets, then select one existing completed
recording on Hisense and verify picture/sound, pause, resume, STOP/BACK
cleanup and that a revoked grant denies further segments. Never reset
pairing or replace production services as part of documentation updates.
