# Shared client capability evidence — internal first slice

Status: **pure C++ model and regression checks only** (2026-10-12).
This does not change the active API, `MediaPresentationSelector`, codec policy,
the VIDAA app, VDR recordings, the installation or any MediaSession.

## Why this exists

A real Hisense VIDAA 9 43A6K/MTK9602 report and user test established
HLS-fMP4 through MSE **with picture, sound, scrubbing and direct time jumps**.
Native HLS-TS failed with `NotSupportedError` after the master playlist
returned HTTP 200 and the protected MediaSession was created. A separate
progressive MP4 playback trial also failed. Teletext subtitles were reported
in track metadata but could not be selected in the client.

These facts belong to a **player engine + protocol + container + output
video/audio codec** combination, not to an entire TV model or an entire codec.
The working MSE Device request currently advertises **H.264/AAC**;
there is no user-verified HEVC Main10, HDR display, direct original AC-3
audio, or direct-play/zero-transcode conclusion.

## Existing authoritative components

- `MediaSourceDescriptor` + bounded `FfprobeRecordingSource` own source facts.
- `MediaPresentationSelector` selects direct, remux or transcode paths.
- `MediaTranscodePolicy`, authenticated Device API and scoped MediaSession
  control actual media production and access.
- `ClientMediaCapabilities` is an untrusted client claim, not device proof.
- See `docs/development/phase-65-media-capability-negotiation-contract.md`
  and `docs/development/media-capability-evidence-design.md` on current main.

## This slice: pure, fail-closed engine-scoped filter

`core/media/include/MediaCapabilityEvidence.h` defines
`MediaPlaybackEngine` (native, MSE, external), `MediaEvidenceGrade`
(unknown, profile hint, runtime reported, verified playback, observed failure),
and `MediaPlaybackPathEvidence` for one combination. `narrowToVerifiedPath`
intersects exactly that combination with the **existing** advertised
`ClientMediaCapabilities`; it never unions separate observations into
invented combinations.

- Grade alone is insufficient: both picture and sound must have been observed.
- A false, runtime-reported or profile-hint result never promotes support.
- Engine, protocol, container, video codec and audio codec must all match.
- Positive MSE seeking does **not** grant HTTP byte-range support.
- Positive HDR input/probe does **not** grant HDR *output*.
- Width/height/channels are bounded by both advertised limits and the
  observed presentation, never expanded by user-provided observations.
- Native TS, HEVC and AC-3 do not become verified by the known H.264/AAC
  MSE/fMP4 success.

The function currently has **no caller in active API/session negotiation**.
`VerifiedPlayback` is an internal classification, not a credential,
autonomous recommendation or remotely accepted declaration. The backend must
not trust a client-supplied claim of success without correlation to the
authorized Device identity, active MediaSession, server-reported *actual*
output profile, real picture/sound outcome, firmware/runtime version and
trusted source fingerprint. It cannot generalize the coarse `MediaCodec::H265`
type across HEVC Main vs Main10, HDR, bit-depth or profile/level differences.

The focused test target is `make test-phase65-media-capability-evidence`,
also wired into the normal `test-fast` graph. This test is deterministic,
read-only and requires no live VDR, tuner, network media or FFmpeg workload.

## Future clients use the same core, only a new adapter

On Kodi, Android TV, another VIDAA, web or a native desktop player, the
client-specific adapter can collect local SDK hints and bounded playback
outcomes. It sends a later **versioned authenticated** capability-evidence
report; the server performs existing authorization checks and compares it
with trusted source metadata. Source probes, provider paths, FFmpeg worker
arguments and authorization are never delegated to a client adapter.

Proposed next ordered slices (not implemented here):

1. Typed/validated evidence v1 with strict size limits, per-field provenance,
   authorization and versioned OpenAPI schemas; optional source fingerprint
   and app/firmware-specific scoping. Pure read-only report first.
2. Add video profile, bit-depth, chroma/HDR-transfer, audio channels and
   subtitles to accurately decide HEVC Main/Main10, AAC/AC-3, Teletext and
   WebVTT. Unknown stays unknown.
3. Correlate a real protected MediaSession's **actual** selected
   videoAction/audioAction and codec/packaging to player outcomes. Distinguish
   compatible MSE remux from H.264/AAC transcoded output and true direct play.
4. Integrate a scoped, bounded override/selection input to the **existing**
   `MediaPresentationSelector`; no new selector or hidden automatic fallback.
5. Hardware acceptance on the real VIDAA TV and a second *different* client
   before enabling automatic selection for broad model families.

No background probing, autoplay, second ffprobe hot-path consumer, recording
index generation, privilege changes, backend service deploy or user setup
changes are part of this slice.
