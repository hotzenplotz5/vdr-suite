# Phase 65 — Media Transcode Performance Policy

## Purpose

Recording playback must choose a server-side video transformation that is both
browser-compatible and sustainable in real time. A codec or pixel-format match
alone is not sufficient: source decoding, deinterlacing and high-resolution
transcoding can change the amount of server work substantially.

The server must not infer performance from a CPU/GPU model name. Virtualization,
clocking, thermal limits, concurrent work, drivers and encoder/library versions
all make that unreliable. VDR-Suite instead uses a typed workload plus measured
throughput samples.

## Session-stable decision

The transcode policy is resolved once, before the FFmpeg worker starts. The
selected encoder backend, hardware device and software preset remain stable for
the complete MediaSession. VDR-Suite does not switch implementations in the
middle of an HLS/MSE session.

This preserves segment continuity and keeps the browser transport independent of
server performance decisions.

## Workloads

The current policy distinguishes:

- `standard`: ordinary video fallback/transcode at HD-or-lower source size;
- `deinterlace`: interlaced source video transformed to progressive output;
- `uhd-source`: source video larger than 1920x1080 and transformed to the client
  target size.

The presentation selector classifies the workload. The transcode policy chooses
the encoder implementation.

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

For `standard` and `deinterlace`, the historical conservative software fallback
remains when no measured preset reaches the minimum:

- `deinterlace` -> `superfast`
- `standard` -> `veryfast`

For `uhd-source`, auto mode is deliberately stricter. A UHD transform that is
known to run below real time can never be repaired by a finite browser buffer.
UHD auto therefore selects only an implementation measured at **>=1.25x**. If
neither a supported hardware backend nor x264 reaches that threshold, the
presentation is unavailable instead of starting a stream that is guaranteed to
fall behind.

An explicit `VDR_SUITE_MEDIA_X264_UHD_PRESET` remains an operator override and
can force the software path when needed for diagnosis.

## VAAPI UHD backend

Phase 65 implements VAAPI as the first hardware video backend. It is currently
restricted to progressive `uhd-source` -> H.264 transcodes. Standard and
interlaced/deinterlace workloads retain their established x264 paths.

A selected VAAPI session uses one explicitly configured DRM render node and keeps
decode, scaling and H.264 encode on the hardware path:

```text
-init_hw_device vaapi=va:/dev/dri/renderD128
-filter_hw_device va
-hwaccel vaapi
-hwaccel_device va
-hwaccel_output_format vaapi
...
-c:v h264_vaapi
-qp 22
-vf scale_vaapi=w=1920:h=1080:format=nv12
-force_key_frames expr:gte(t,n_forced*4)
```

The hardware input options are emitted before the FFmpeg input. The HLS output
keeps the same four-second forced-keyframe and independent-segment contract as
software transcodes.

The default device is:

```text
/dev/dri/renderD128
```

It can be overridden by the trusted server environment:

```text
VDR_SUITE_MEDIA_VAAPI_DEVICE=/dev/dri/renderD128
```

A VAAPI device is not selected merely because FFmpeg lists `h264_vaapi`. Auto
selection requires a calibration result for `uhd-source.vaapi` that meets the
same 1.25x minimum as software. Version 4 also persists the render node in
`vaapi.device`, so the daemon uses the device on which the measurement was made.
An explicit environment override takes precedence over that persisted path.

QSV and NVENC remain represented by the typed backend enum but are not enabled
by this Phase-65 follow-up; their command plans still fail closed.

## Calibration

The installed operator tool is:

```text
vdr-suite-media-calibrate
```

Run it outside active playback when the server is in a representative operating
state. Calibration is an explicit operator action; it is never started by the
daemon.

The **version 4** profile extends the v3 software measurements with optional
hardware-backend throughput and the hardware device used for that measurement.
The daemon continues to accept existing **version 3** profiles for
backwards-compatible software policy, while versions 1 and 2 remain
intentionally ignored.

Measurements include source decode, the selected workload transform and encode.
For a real Recording reference they also include the normal AAC stereo audio
fallback when an audio stream is present.

Generated compressed fixtures use the short `--seconds` duration (default: six
media seconds). Real source references use the separate `--real-seconds`
duration, defaulting to **30 media seconds**, because Phase-65 acceptance showed
that a damaged Recording startup can dominate a short speed result even when the
sustained transform is viable.

Real source references skip **15 seconds** of recording pre-roll by default
before the timed sample begins. This is an operator calibration control only;
actual Recording playback still starts at the requested media position.

Representative real Recording sources can be supplied directly:

```bash
sudo vdr-suite-media-calibrate \
  --deinterlace-source '/srv/vdr/video.00/SciFi/Der_Wüstenplanet/2015-12-11.18.41.5-0.rec' \
  --uhd-source '/srv/vdr/video.00/Drama/A_Star_Is_Born/2026-04-21.19.16.1-0.rec'
```

A source argument may point either to a media file or to a VDR `.rec` directory;
for a Recording directory the first five-digit `.ts` segment is used.

Available real-source overrides are:

```text
--standard-source
--deinterlace-source
--uhd-source
```

Other relevant controls are:

```text
--real-seconds 30
--real-start 15
--vaapi-device /dev/dri/renderD128
--no-vaapi
```

When VAAPI is not disabled, the selected render node exists, and FFmpeg exposes
`h264_vaapi`, the calibrator additionally benchmarks the UHD VAAPI path. A failed
hardware probe is reported but does not discard the completed software
calibration; without a passing hardware sample UHD auto will still fail closed
unless a software preset reaches the threshold.

When no real source is supplied:

- `standard` uses a generated compressed 1080p H.264 fixture;
- `deinterlace` uses a generated compressed 1080i H.264 fixture;
- `uhd-source` uses a generated compressed 2160p HEVC fixture when `libx265` is
  available.

The calibrator writes:

```text
/var/lib/vdr-suite/media-transcode-performance.conf
```

Example v4 schema (illustrative values):

```text
version=4
# deinterlace.source=real:/path/to/interlaced.rec/00001.ts
deinterlace.superfast=1.600
deinterlace.veryfast=1.300
deinterlace.faster=1.100
deinterlace.fast=0.900
# uhd-source.source=real:/path/to/uhd.rec/00001.ts
uhd-source.superfast=0.700
uhd-source.veryfast=0.600
uhd-source.faster=0.500
uhd-source.fast=0.400
uhd-source.vaapi=2.500
vaapi.device=/dev/dri/renderD128
```

After reviewing the generated profile, restart the daemon to load it:

```bash
sudo systemctl restart vdr-suite-daemon
```

Calibration is never run automatically during playback or daemon startup.

## Profile compatibility

The daemon accepts:

- **version 3**: trusted sustained software measurements from the prior Phase-65
  policy;
- **version 4**: the same software schema plus optional hardware-backend samples
  and `vaapi.device`.

Versions 1 and 2 remain ignored:

- version 1 omitted source decode cost;
- version 2 added decode cost but allowed short real-source samples whose damaged
  startup region could dominate the final speed.

A v3 profile without a >=1.25x UHD software result will now make UHD auto fail
closed. Re-run the v4 calibrator on hardware-capable systems to enable a measured
VAAPI path.

## Operator overrides

The global x264 mode is controlled by:

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

The performance-profile path and VAAPI render node can be overridden with:

```text
VDR_SUITE_MEDIA_TRANSCODE_PROFILE
VDR_SUITE_MEDIA_VAAPI_DEVICE
```

Arbitrary FFmpeg arguments are never accepted through these settings. The VAAPI
device is carried as one argv value and the policy/command builder restrict it
to a `/dev/dri/` path.

## Real yaVDR evidence — interlaced HD

The Phase-65 interlaced Recording acceptance case
`/SciFi/Der_Wüstenplanet/2015-12-11.18.41.5-0.rec` exposed both the playback and
software-calibration requirements.

For the required 1080i -> 1080p25 browser compatibility transform on the tested
yaVDR system, a 30-second real-source benchmark measured:

- x264 `veryfast`: `0.992x`, without useful HLS headroom;
- x264 `superfast`: `1.54x`, with viable real-time reserve.

That evidence established the sustained 1.25x policy and the v3 real-source
sampling rules. It also remains the reason deinterlace keeps its proven software
path in this follow-up.

## Real yaVDR evidence — UHD HEVC Main10

`/Drama/A_Star_Is_Born/2026-04-21.19.16.1-0.rec` exposed the unsafe UHD software
fallback. The source is HEVC Main10, 3840x2160. Runtime selected x264 `veryfast`
with a 1920x1080 target. The HLS segments themselves were healthy at about
2.71 Mbit/s and exactly four seconds, but FFmpeg's paced-source lag grew past
eight seconds while the browser forward buffer drained.

Direct sustained software measurements on that exact source showed:

- 1080p x264 `veryfast`: `0.281x`;
- 1080p x264 `superfast`: `0.356x`;
- 720p x264 `veryfast`: `0.391x`;
- 720p x264 `superfast`: `0.468x`.

No software resolution/preset combination tested could sustain real time.

The same source on the host's Intel UHD Graphics 605 using
`/dev/dri/renderD128` measured the 4K HEVC -> 1080p H.264 VAAPI path at
**3.825x**. A complete paced 40-second HLS reproduction then completed with
`EXIT=0`, H.264 1920x1080 + AAC, `#EXT-X-INDEPENDENT-SEGMENTS`, and ten
consecutive `4.004s` fMP4 segments.

This evidence is why Phase 65 now permits calibrated VAAPI for progressive UHD
Recording transcodes and why UHD auto fails closed when no implementation meets
the minimum real-time threshold.
