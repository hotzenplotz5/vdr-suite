# Phase 65 — Media Transcode Performance Policy

## Purpose

Recording playback must choose a server-side video transformation that is both
browser-compatible and sustainable in real time. A codec or pixel-format match
alone is not sufficient: deinterlacing and high-resolution transcoding can
change the amount of CPU work substantially.

The server must not infer performance from a CPU model name. Virtualization,
clocking, thermal limits, concurrent work and encoder/library versions all make
that unreliable. Instead, VDR-Suite uses a typed workload plus optional measured
throughput samples.

## Session-stable decision

The transcode policy is resolved once, before the FFmpeg worker starts. The
selected encoder backend and preset remain stable for the complete MediaSession.
VDR-Suite does not switch x264 presets in the middle of an HLS/MSE session.

This preserves segment continuity and keeps the browser transport independent of
server performance decisions.

## Workloads

The current software policy distinguishes:

- `standard`: ordinary video fallback/transcode at HD-or-lower source size;
- `deinterlace`: interlaced source video transformed to progressive output;
- `uhd-source`: source video larger than 1920x1080 and transformed to the client
  target size.

The presentation selector classifies the workload. The transcode policy chooses
the encoder implementation and preset.

## Software x264 presets

Supported software presets are:

- `superfast`
- `veryfast`
- `faster`
- `fast`

`auto` is the default policy mode. When measured samples exist, auto chooses the
slowest/highest-compression preset that still reaches at least **1.25x** measured
real-time speed for that workload.

The quality-order search is:

1. `fast`
2. `faster`
3. `veryfast`
4. `superfast`

If no suitable measurement exists, the conservative compatibility defaults are:

- `deinterlace` -> `superfast`
- `standard` -> `veryfast`
- `uhd-source` -> `veryfast`

These fallbacks are intentionally server-safe defaults, not claims that every
machine has the same performance.

## Calibration

The installed operator tool is:

```text
vdr-suite-media-calibrate
```

Run it outside active playback when the server is in a representative operating
state:

```bash
sudo vdr-suite-media-calibrate
sudo systemctl restart vdr-suite-daemon
```

The tool benchmarks representative synthetic `standard`, `deinterlace` and
`uhd-source` software-x264 workloads for all supported presets and writes:

```text
/var/lib/vdr-suite/media-transcode-performance.conf
```

Example format:

```text
version=1
deinterlace.superfast=1.540
deinterlace.veryfast=0.992
deinterlace.faster=0.810
deinterlace.fast=0.690
```

The daemon reads this profile when the media runtime is created. Calibration is
never run automatically during playback or daemon startup.

## Operator overrides

The global mode is controlled by:

```text
VDR_SUITE_MEDIA_X264_PRESET=auto
```

Accepted values are:

```text
auto
superfast
veryfast
faster
fast
```

Optional workload-specific overrides take precedence over the global setting:

```text
VDR_SUITE_MEDIA_X264_STANDARD_PRESET
VDR_SUITE_MEDIA_X264_DEINTERLACE_PRESET
VDR_SUITE_MEDIA_X264_UHD_PRESET
```

The performance-profile path can be overridden with:

```text
VDR_SUITE_MEDIA_TRANSCODE_PROFILE
```

An invalid override is ignored and falls back to the typed auto/default policy;
arbitrary FFmpeg arguments are never accepted through these settings.

## Real yaVDR evidence

The Phase-65 interlaced Recording acceptance case
`/SciFi/Der_Wüstenplanet/2015-12-11.18.41.5-0.rec` exposed the need for this
policy. The source is H.264 1920x1080 with top-field-first interlacing and also
contains damaged/inconsistent H.264/transport data.

For the required 1080i -> 1080p25 browser compatibility transform on the tested
yaVDR system:

- x264 `veryfast` measured `0.992x` and had no useful HLS headroom;
- x264 `superfast` measured `1.54x` and provided a viable real-time reserve.

That evidence justifies the conservative deinterlace fallback for an
uncalibrated server while still allowing a faster calibrated server to select
`veryfast`, `faster` or `fast` automatically.

## Hardware encoder boundary

The profile model already distinguishes encoder backends for software x264,
VAAPI, Intel QSV and NVENC. Only software x264 is implemented in this slice.
Unimplemented hardware backends fail closed in the command builder.

A later hardware-acceleration implementation can extend the same server-side
policy boundary without changing browser capabilities, presentation selection or
mid-session HLS behavior.
