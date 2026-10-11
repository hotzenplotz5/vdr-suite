# Public-v1 VIDAA Recording audio and subtitle Device routes (draft)

> **b20261011.9 Device-track candidate (2026-10-11, development only):**
> This isolated branch `work/vidaa-device-track-api-for-b20261011-9`
> extends existing Browser-backed trackStatus, audioTrackId HLS session
> creation and subtitleWebVtt over owned Device Public-v1 routes.
> It also incorporates the bounded, symlink-rejecting named .srt
> sidecar lookup from draft PR #449 and constrains Device WebVTT to
> 1 MiB (matching the paired VIDAA consumer). Both GET /tracks and
> POST /subtitles validate canonical `ms_<32hex>` IDs, while Device
> authentication, live play grant and session ownership remain required.
> **No Index-Updater or Recording index files are modified here.**
> Do not deploy this branch as a replacement for the running backend
> without verifying the actually installed server revision and preserving
> all existing index/duration work. No backend production change is
> authorized by this branch or by successful CI alone.

Review-only branch `work/vidaa-public-v1-track-selection-20261011`,
PR https://github.com/hotzenplotz5/vdr-suite/pull/446 .
**Not merged, not approved for server deployment.**

## Upstream audio/subtitle semantics reused, not duplicated

The first-party Browser frontend already calls
`RecordingMediaSessionController::trackStatus` through
`/api/media/sessions`, parses
`RecordingMediaTrackContract::json` audio/subtitle inventory and
selectability, uses `audioTrackId` when replacing its HLS session,
and requests `select-subtitle-track` as WebVTT with the absolute
stream-base seconds. Text-tract attachment and Blob cleanup live in
`web/frontend/api/recording-track-controls.js`.

VIDAA is a paired Device client and must **not** reuse the Browser
Session API, CSRF, Browser cookies or untrusted media paths.

## Candidate routes

The existing protected, owner-checked Device session namespace is
extended with:

1. `GET /api/v1/recording-playback-sessions/{ms-id}/tracks`:
   `PublicDevicePlaybackRegistry::describeOwned` verifies live
   Device ownership/grant. A first-party
   `RecordingMediaSessionController::trackStatus` query is made
   under the same actor; the response is projected to `tracks`
   plus public session id, backend id and state only. Internal
   Recording/native ID, worker path, grant token, raw media URL
   and cookies never leave this projection.
2. `POST /api/v1/recording-playback-sessions/{ms-id}/subtitles`:
   `SecurityHttpGate` explicitly treats this as a protected
   Device-only `media.recording.play` mutation, validates backend
   scope, and checks live authenticated Device proof. After
   `parseSubtitleTrackSelection` validates the operation and
   the path/session match, `deviceSessions.owned` rechecks
   backend/session owner/grant. The first-party
   `selectSubtitleTrack` returns bounded `text/vtt` or an
   explicit `off` JSON response, never native source files.

Audio track switching does **not** require an additional route.
The existing Device session-create parser accepts a validated
`audioTrackId`. HLS moves to the same absolute indexed position
while selecting the corresponding audio stream in the established
MediaPresentationSelector pipeline. The indexed timeline must
be ready for a nonzero-position resume.

DVB bitmap subtitles, teletext, and unconvertible formats remain
truthfully visible as available-but-unsupported; they are not
silently passed off as WebVTT. Supported WebVTT, SRT, ASS and
mov-text tracks are converted by the existing first-party
recording subtitle pipeline. The Hisense renderer must still
prove actual text track delivery on hardware.

## Security/acceptance

- Anonymous and Browser Session requests must be denied by the
  SecurityHttpGate; these checks are added to the existing C++
  security unit test.
- Ownership, backend scope, revoked grants, stale session ids,
  invalid track IDs, WebVTT content type and maximum payload size
  require focused integration coverage before merge.
- Existing broader branch CI already has unrelated hardcoded
  phase69 Public-v1 contract-count baseline failures.
  Do not rebaseline security inventories solely to force PASS.
- No installed Daemon, recording/index/TS file, media backend,
  Nginx config, pairing secret, ACL or filesystem mode has been
  changed. Production release requires separate review/approval,
  backup, and an exact rollback plan.
