# Client codec evidence v1 — shared contract for VIDAA, Android TV, Kodi and later clients

Status: implementation slice for exact *internal* evidence narrowing only. No public API, database, persistence, playback changes, automatic device profile or hardware certification in this PR.

## Why not a Plex-like per-device codec list?

The observed Hisense 43A6K/VIDAA 9 hardware gives us separate facts:

- JavaScript canPlayType reports H.264/HEVC/Main10/AAC/AC-3 MP4 as "probably" and MediaSource.isTypeSupported reports compatible MP4 MIME strings as supported. **These are runtime reports, not decoded picture+sound.**
- Native `hls-ts` failed to start with `NotSupportedError` although the authorized MediaSession and HLS master playlist succeeded.
- Existing app-owned HLS/fMP4/MSE playback succeeded with a user-selected recording; subsequently normal recording playback, relative seek and direct time jump passed on the real TV in VIDAA b20261011.24. The user confirmed audio output too.
- A teletext subtitle was listed but not selectable; this is subtitle delivery/selection evidence, **not** a video codec failure. The VIDAA track UI currently only enables selected supported WebVTT/SRT tracks.

The precise *output* codec, profile, bit depth, HDR signal, resolution, audio codec/channel count and stream-copy/transcode decision for that recording still need correlated, trusted backend MediaSession/output metadata before we call a codec tuple **verified**. The client declaring H.264/AAC as requested capabilities does not prove source-HEVC Direct Play.

## Two-stage architecture for every client

1. **Client adapter**: a thin local engine probe reports native/external/MSE MIME/container evidence and optional successful playback observations; no vendor switch in the core selector. Device model, firmware, runtime and adapter version scope hints and results. No provider paths, cookies, DRM secrets, tokens or raw ffmpeg arguments.
2. **Server evidence service (future)**: validates schema, authenticated Device identity, scoped grants, report bounds, firmware/version scope and freshness; correlates protected MediaSession observations with trusted output metadata before granting verified codec capabilities. A client may submit a claimed success, but the claim alone is never the authority. An empty or stale/unknown report grants no new codec.
3. **Existing MediaPresentationSelector**: receives capabilities for **one eligible output-engine/path combination**, evaluates source streams from the existing bounded recording ffprobe, and returns pass-through/remux/transcode where policy permits.
4. **Player runtime**: applies only the selected authorized profile, observes real picture+sound, startup failure, buffering, seek and subtitle/audio selection separately. Do not silently retry an incompatible transport. A future explicit fallback must be bounded, visible, policy-checked and independently authorized.

A device may maintain many observations; each one represents an exact transport/segment-container/video-codec/audio-codec/engine combination. Never union the observed lists into a cross-product. E.g. a verified H.264/AAC HLS-fMP4/MSE route **does not** verify HEVC/AC-3 HLS-fMP4/MSE, native HLS-TS, or progressive MP4.

## Initial internal C++ slice

`MediaCapabilityEvidence.h/.cpp` provides `MediaPlaybackPathEvidence` and `narrowToVerifiedPath`. The caller must pass **server-accepted, corroborated evidence**, not raw untrusted request JSON. It refuses profile-hint, runtime-reported, unknown, failed, picture-only, sound-only, engine mismatch, container mismatch, codec mismatch, unsupported advertised codec, unknown evidence dimensions/channels and unconfirmed HDR output. The result restricts capabilities to exactly one path and to independently observed video/audio limits. It does not infer HTTP Byte Range support from a successful time-seek.

It is deliberately **not wired** to the Device session request handler, the selector runtime, the live-TV socket, ffprobe or transcoding. This avoids changing deployed behavior before the complete service-side trust and hardware tests exist. The pure selector unit tests prove that a verified H.264/AAC fMP4/MSE path remuxes H.264 while transcoding incompatible AC-3 audio, and transcodes HEVC video rather than claiming unverified HEVC copy.

## Future versioned report shape (design, not live endpoint)

A portable adapter should eventually send bounded, allowlisted objects like:

```json
{
  "schemaVersion": 1,
  "engine": "mse",
  "path": {"protocol": "hls", "container": "fmp4",
           "videoCodec": "h264", "audioCodec": "aac"},
  "runtime": {"deviceProfile": "hisense-43a6k-vidaa9",
              "firmwareVersion": "exact-device-version",
              "clientVersion": "exact-app-version"},
  "evidence": {"grade": "runtime-reported",
               "pictureObserved": false, "soundObserved": false,
               "hdrOutputObserved": false},
  "limits": {"testedVideoWidth": null, "testedVideoHeight": null,
             "testedAudioChannels": null}
}
```

These example placeholders **are not** the current Hisense's measured output facts. The server must bind any higher-trust result to the authenticated Device and a reviewed media-output/engine probe; the client must not submit a self-asserted final authorization grade. Prefer short-lived evidence keyed to device/firmware/app/engine/codec profile; invalidate after upgrades or significant decoder errors. An administrator can review exceptions, without enabling transcoding policy changes from profile text.

## Minimum sample suite for a newly added client

- H.264 SDR + AAC stereo over HLS/fMP4 (baseline engine-specific picture/sound/start/seek)
- H.265 Main 8 + AAC, then Main10 + AAC (prove actual decoding and limit, not only MIME)
- H.264 + AC-3 / E-AC-3 and H.265 + AC-3 (prove sound and pass-through/downmix)
- progressive MP4 and native HLS-TS separated from MSE/fMP4
- HDR10/HLG output signaling separately, without inferring HDR display from HDR source
- subtitles separated by delivery/format: WebVTT/SRT vs DVB bitmap vs teletext, not forced by video capability

Test assets must be independently licensed and bounded; tests should run only on user request or documented first-pairing consent and must never probe live-TV sockets or write VDR indexes.

## Integration gates

- Resolve existing ffprobe source metadata and output stream proof under the authorized MediaSession, not from user-provided filenames or raw source paths.
- Keep `MediaTranscodePolicy` and existing Device grants authoritative. No new public endpoint or production selection changes until verified C++ parser schemas, rate/resource limits, OpenAPI, negative auth tests, hardware experiments and rollback exist.
- The separate backend PR #463 contains bounded ffprobe HDR/bitrate/color metadata work; integrate its tested source facts before claiming profile/bit-depth/HDR compatibility. Do not merge it by implication of this independent slice.
