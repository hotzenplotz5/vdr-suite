# VIDAA: protected Recording audio tracks and WebVTT — historical draft

**Current repository status (2026-10-11):** The consolidated R2 source was merged into `main` at `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`. The original branch/provenance notes below are **historical** and must not be read as a claim that this code is still unmerged. This source checkpoint alone does **not** prove which daemon binary is installed on yaVDR, nor successful real Hisense audio, SRT/WebVTT or STOP/cleanup acceptance. A deployment/restart still needs separate explicit authorization.

## Source parity (2026-10-11)

The existing VDR-Suite Browser implementation
`web/frontend/api/recording-track-controls.js` uses:
- the canonical `RecordingMediaSessionController::trackStatus`, which
  returns `RecordingMediaTrackContract::json()` (IDs, language/label,
  selected/default, codec, supported/unsupported reasons).
- for HLS audio, it **replaces** the recording MediaSession using
  `audioTrackId`, retaining absolute position and play/pause state.
- for text subtitles, it requests canonical
  `select-subtitle-track`, receives `text/vtt`, and mounts a Blob
  `<track>` on its video element.
- supported conversions from
  `core/media/src/RecordingSubtitleWebVtt.cpp`:
  WebVTT, SubRip, ASS, MovText. **DVB bitmap and Teletext subtitles
  are not presently WebVTT convertible** by that code; do not offer
  them as working browser-text subtitles.

VIDAA b20261011.5 still relied exclusively on
`HTMLMediaElement.audioTracks`/`textTracks`, which may be empty for
the selected audio-only HLS-fMP4 MSE output even when the source
Recording has multiple tracks.

## New strictly Device-owned routes (NOT deployed)

The authenticated Public-v1 Device recording-session controller
already provided create, GET and STOP. This draft adds:

- `GET /api/v1/recording-playback-sessions/ms_.../tracks`
  - `SecurityHttpGate` permits Device-authenticated reads only.
  - `PublicDevicePlaybackRegistry::describeOwned` checks the live
    Device/session/play grant for the caller, not merely the ID.
  - Calls the existing owned internal
    `RecordingMediaSessionController::trackStatus` and emits only
    the *canonical tracks subtree* plus public Device Session ID,
    backend ID and state, **never** native Recording ID, sources,
    workstation paths, media cookies or Browser-only URLs.
  - No new VDR file access or subtitle extraction here.

- `POST /api/v1/recording-playback-sessions/ms_.../subtitles`
  - `SecurityHttpGate` explicitly marks this a protected Device-only
    mutation and enforces `media.recording.play` for the backend.
  - The corresponding ID and scope must match the fully parsed,
    validated `select-subtitle-track` request.
  - `PublicDevicePlaybackRegistry::owned` checks Device ownership.
  - Reconstructs the internal request using **only** validated
    fields. `RecordingMediaSessionController` already authorizes
    actor ownership, validates supported subtitle formats and
    calls the unchanged WebVTT converter.
  - Returns either the existing `text/vtt` result or the
    `off` JSON status. No direct Browser/session token reuse.
  - `Set-Cookie` is stripped from this response.

For audio, the existing Device-created MediaSession already accepts
`audioTrackId`, so **no new Device audio mutation route** is needed.
VIDAA stops the prior session first, starts the chosen track at a
bounded index-backed absolute position, then queries the new
Session's actual selected track. If selection is unconfirmed, STOP
the replacement rather than claiming success.

## Gating

This branch is based on the **separate unfinished indexed-duration
backend draft**. Neither draft is approved, merged or installed.
The VIDAA counterpart is the uninstalled `b20261011.6` development
branch `work/vidaa-device-tracks-ui-20261011`.

Required checks before server modification:
- targeted C++ compile and SecurityHttpGate ownership/Browser denial,
- Device GET must reject foreign/deleted sessions and native identifiers,
- Device subtitle POST must reject foreign/grant-revoked sessions,
- WebVTT extraction, offset and MIME on a real completed TS recording,
- HLS audio replacement confirms requested track and absolute position,
- real Hisense plays generated WebVTT `<track>` and cleans up Blob URLs,
- no lost STOP on cancellation, no second playback worker.
- baseline CI of the parent currently fails unrelated public contract
  inventory/count guards, which must not be weakened.

Do not install on yaVDR, change any permissions/ownership, edit index
files, start/stop services, or deploy the new client until the backend
has passed a separate explicit review and the user authorizes it.
