# Playback Fast Start — guarded recording HLS experiment (2026-10-12)

## Status and evidence boundary

**Candidate only.** This branch adds an opt-in FFmpeg recording-start experiment and
unit tests. Default recording HLS, live HLS, Device session authentication,
media grants, seek contracts and the already tested Hisense b20261011.24
HLS-fMP4/MSE route remain unchanged. A GitHub CI pass is not a measured
time-to-first-picture improvement or a real-TV decoder acceptance.

The experiment is selected only if the **server process environment** contains
the exact value `VDR_SUITE_HLS_FAST_START_EXPERIMENT=1`. Absence, `0`,
`true` and any other value retain the previously accepted FFmpeg command.
This is an operator-controlled trial gate, **not** a new Public-v1 parameter,
client authority or automatic device-codec claim. Do not turn it on on an
unverified production installation.

## Why the original path waits

The Recording HLS builder reads `input.ffconcat` with `-re`, a four-second
HLS segment target, an eight-entry rolling playlist and deletion of expired
segments. The Recording runtime does not issue a ready Session until a
manifest and the first segment (and fMP4 init file) exist. The VIDAA MSE
adapter must then load the manifest, init segment and segments, append them
to a SourceBuffer, and wait for the decoder. The client also has a silent
detail-view prewarm after 700 ms.

Without bounded ingest, removing `-re` can cause FFmpeg to rapidly
generate and delete the entire eight-entry sliding window before the TV
fetches it. Therefore the experiment **preserves `-re`**.

## Experimental command changes (recordings only)

- Add `-readrate_initial_burst 4` as an FFmpeg *input* option **before**
  `-re -f concat ... -i input.ffconcat`. Only the start has a bounded
  head start; the long-lived playlist retains real-time pacing.
- When the video stream is copied, additionally set `-hls_init_time 1`
  to target an earlier initial HLS segment. The normal `-hls_time 4`,
  eight-item playlist, deletion/temporary-file flags, stream maps and
  copy/transcode decisions are unchanged.
- For transcoded video, retain the proven four-second
  `-force_key_frames` cadence and `independent_segments` contract.
  An initial ingest burst alone is tested; shorter transcoded GOPs are
  a separate hardware-dependent decision.
- Live socket input never receives a startup burst or initial segment
  override. Progressive delivery is unchanged.

**Caveat:** FFmpeg may cut stream-copy segments in places the decoder
cannot start until the next keyframe. `hls_init_time` is a target, not
a guaranteed first-frame timestamp. Neither faster first-segment creation
nor successful playback has been proven on the installed yaVDR. Unsupported
FFmpeg flags cause a controlled session failure rather than a transparent
fallback; use the accepted path unless tested.

## Acceptance requirements before turning on

1. Read-only host preflight: inspect the installed `/usr/bin/ffmpeg` version
   and `ffmpeg -h full` for `readrate_initial_burst`, and
   `ffmpeg -h muxer=hls` for `hls_init_time`. Also confirm the exact
   installed backend commit before altering its service configuration.
2. Establish a baseline at the TV with the opt-in **off** using the same
   authorized recording, codec, and picture/sound test. Record
   OK-to-playing, Device CREATE-to-ready, first manifest, first segment
   and first decoded frame (where platform permits).
3. In an isolated, explicitly authorized trial only, compare the same
   source with the opt-in **on**, including 5+ minute uninterrupted playback,
   initial segment decodability, source changes, audio sync, seek and
   cleanup. Do not infer timing improvements from smaller playlist
   target durations alone.
4. Any failure, missing FFmpeg option, extra churn, decode stall or
   unsupported device means **keep the flag unset**. No release default
   change until concrete real-host and real-TV evidence exists.

## Source and regression checks

- `core/media/src/FfmpegHlsCommandBuilder.cpp`: exact bounded argv.
- `core/media/src/RecordingMediaSessionRuntime.cpp`: opt-in at Recording
  HLS provisioning only.
- `core/media/tests/test_ffmpeg_hls_command_builder.cpp`: ordinary
  recording, opt-in copy, opt-in transcode and unchanged live input.
- `make test-phase65-ffmpeg-hls-command-builder` and
  `make test-phase65-recording-media-session-runtime` are established
  Makefile targets; CI provides the authoritative candidate verdict.

Related: [Device playback source contract](public-v1-device-recording-playback.md)
and [Public API](../../api/device-recording-playback.md).
