# VIDAA Device-owned audio and subtitle interoperability — draft

**Source audit:** The VDR-Suite Browser Webfrontend already obtains
`RecordingMediaTrackContract::json` by `track-status`, switches an
HLS audio language by creating a new HLS MediaSession with
`audioTrackId` while preserving the absolute Recording position, and
requests a selected subtitle via `select-subtitle-track`. That
operation converts supported text subtitle formats to `text/vtt`.
`web/frontend/api/recording-track-controls.js` mounts a Blob-backed
`<track>` to the existing HTML video.

**Do not use the Browser `/api/media/sessions` route from VIDAA.**
It has a separate authentication, request and credential contract.
A Device must remain within `/api/v1/recording-playback-sessions`.

## This draft changes only server repository source

- The existing **Device-authenticated, live-grant and Device-owned**
  session GET now includes a **bounded canonical track inventory**.
  The canonical controller performs its actor ownership check again.
  The new `deviceTrackInventory` projection returns the track JSON
  alone; the native Recording ID, credentials, media paths, workspace
  and original Browser session JSON are not copied.
- HLS audio track preference is already parsed by the existing
  `RecordingMediaSessionAudioTrackPreferenceParser` on MediaSession
  creation and selected by `MediaPresentationSelector`. No new
  audio mutation route is necessary.
- A narrowly-scoped Device `POST /api/v1/recording-playback-sessions/
  {ms-id}/subtitle` is recognized by `SecurityHttpGate`, requiring
  Device proof, `media.recording.play` permission, a valid backend
  scope, and live Session ownership from `PublicDevicePlaybackRegistry`.
  Only then is the same canonical `select-subtitle-track` operation
  used; WebVTT responses are limited to 2 MiB and never return a
  Browser cookie. Invalid ids/routes are not accepted.

## Subtitle format limits

The existing `RecordingSubtitleWebVtt::supports` supports
WebVTT, SubRip, ASS and mov-text **text subtitles**. It explicitly
does **not** support converting DVB bitmap subtitles or Teletext to
WebVTT. Those require a distinct, verified delivery strategy;
the UI must show them as non-selectable and must not fake that
they will render.

## Review / release gates

This is **not deployed** and **not an approved release**.
The base backend work branch already has failing
contract-count/doc/architecture CI checks. Do not rebaseline
unrelated guard tests to make this appear green. Before deployment:
1. Compile the touched C++ code with the relevant recording/public-v1
   and security targets.
2. Add and pass Device-only authorization tests including anonymous,
   Browser actor, foreign Device and revoked grant.
3. Verify no path, media secret, HttpOnly cookie or native Recording ID
   appears in GET or subtitle responses.
4. Verify owned HLS audio preference and native WebVTT delivery on a
   completed, index-ready Recording, and unsupported DVB formats.
5. Separately approve daemon installation, backup and any restart.

The installed yaVDR/VDR, frontend metadata/permissions, index files
and Recording content have **not been modified**.
