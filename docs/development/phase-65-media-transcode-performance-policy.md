# Phase 65 — Media Transcode Performance Policy

## Purpose

Recording playback must choose a server-side video transformation that is both
browser-compatible and sustainable in real time. A codec or pixel-format match
alone is not sufficient: source decoding, deinterlacing and high-resolution
transcoding can change the amount of CPU work substantially.

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

`auto` is the default policy mode. When trusted measured samples exist, auto
chooses the slowest/highest-compression preset that still reaches at least
**1.25x** measured real-time speed for that workload.

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
state. Calibration is an explicit operator action; it is never started by the
daemon.

The **version 2** calibration format measures source decode plus the selected
workload transform plus x264 encode. The calibrator therefore no longer feeds
raw `lavfi` frames directly into the encoder as the authoritative performance
sample.

By default, it first creates short compressed reference clips and then decodes
those clips during each benchmark. For a workload where an operator has a
representative real Recording, that source can be supplied directly:

```bash
sudo vdr-suite-media-calibrate \
  --deinterlace-source '/srv/vdr/video.00/SciFi/Der_Wüstenplanet/2015-12-11.18.41.5-0.rec'
```

A source argument may point either to a media file or to a VDR `.rec` directory;
for a Recording directory the first five-digit `.ts` segment is used.

Available real-source overrides are:

```text
--standard-source
--deinterlace-source
--uhd-source
```

When no real source is supplied:

- `standard` uses a generated compressed 1080p H.264 fixture;
- `deinterlace` uses a generated compressed 1080i H.264 fixture;
- `uhd-source` uses a generated compressed 2160p HEVC fixture when `libx265` is
  available; otherwise that workload is omitted and the daemon retains its
  conservative UHD fallback.

The calibrator writes:

```text
/var/lib/vdr-suite/media-transcode-performance.conf
```

Example format:

```text
version=2
# deinterlace.source=real:/srv/vdr/video.00/SciFi/Der_Wüstenplanet/2015-12-11.18.41.5-0.rec/00001.ts
deinterlace.superfast=1.540
deinterlace.veryfast=0.992
deinterlace.faster=0.810
deinterlace.fast=0.690
```

The daemon accepts only the decoded-source **version 2** profile format. Earlier
`version=1` profiles are intentionally ignored and therefore fall back to the
conservative workload defaults. This is a safety boundary: Phase-65 real yaVDR
acceptance showed that the original raw-frame benchmark could materially
overestimate the capacity of the real Decode -> Deinterlace -> Encode path.

After reviewing the generated profile, restart the daemon to load it:

```bash
sudo systemctl restart vdr-suite-daemon
```

Calibration is never run automatically during playback or daemon startup.

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
`/SciFi/Der_Wüstenplanet/2015-12-11.18.41.5-0.rec` exposed both the playback and
calibration requirements. The source is H.264 1920x1080 with top-field-first
interlacing and also contains damaged/inconsistent H.264/transport data.

For the required 1080i -> 1080p25 browser compatibility transform on the tested
yaVDR system:

- x264 `veryfast` measured `0.992x` on the real Recording source and had no useful
  HLS headroom;
- x264 `superfast` measured `1.54x` on the same real source and provided a viable
  real-time reserve.

The original raw-frame calibrator produced materially more optimistic numbers
for the same machine (`deinterlace veryfast=1.54x`). That mismatch is why
version 1 profiles are rejected and calibration now includes compressed-source
decode or an explicitly supplied real Recording.

This evidence justifies the conservative deinterlace fallback for an
uncalibrated server while still allowing a faster, correctly calibrated server
to select `veryfast`, `faster` or `fast` automatically.

## Hardware encoder boundary

The profile model already distinguishes encoder backends for software x264,
VAAPI, Intel QSV and NVENC. Only software x264 is implemented in this slice.
Unimplemented hardware backends fail closed in the command builder.

A later hardware-acceleration implementation can extend the same server-side
policy boundary without changing browser capabilities, presentation selection or
mid-session HLS behavior.
