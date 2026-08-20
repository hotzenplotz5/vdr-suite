# Phase 65 — Media Transcode Performance Policy

## Purpose

Recording and Live playback must choose a server-side video transformation that
is browser-compatible **and** sustainable in real time. Codec or pixel-format
compatibility alone is not enough: decode cost, deinterlacing, scaling and
high-resolution encode cost can change the server workload substantially.

The server does not infer capability from CPU/GPU model names. Virtualization,
clocking, thermal limits, concurrent work, drivers and encoder/library versions
make that unreliable. VDR-Suite uses typed workloads plus measured throughput.

## Session-stable decision

The encoder policy is resolved once for every new MediaSession, before its FFmpeg
worker starts. The selected encoder backend, hardware device and software preset
remain stable for the complete session. Changing a backend setting does not
retarget a running Recording or Live worker.

This preserves stream continuity and keeps browser transport independent of
later operator changes.

## Backend-scoped managed selection

The backend-scoped managed encoder setting exposes exactly three values:

- `auto`
- `software`
- `vaapi`

Configuration precedence is deterministic:

```text
backend-scoped managed setting
  -> VDR_SUITE_MEDIA_VIDEO_ENCODER deployment environment
  -> built-in auto
```

The managed setting is persisted in the VDR-Suite SQLite database by `backend_id`.
Clearing it does not write an implicit `auto`; it returns control to the deployment
value. The Web UI therefore also offers a distinct **Deployment default** choice.

The authenticated settings resource is:

```text
GET  /api/backends/<backendId>/settings/media-transcode
POST /api/backends/<backendId>/settings/media-transcode
```

The read model is intentionally sanitized. It exposes the effective mode,
configuration source, calibration presence, real-time threshold and typed VAAPI
eligibility diagnostics. It does not expose a DRM path, process environment,
raw FFmpeg command or arbitrary encoder parameters.

The POST body accepts only the typed managed mode or `clearManagedOverride` plus
the normal backend/operation identifiers. Browser mutations use the existing
CSRF and accountability boundary. Backend scope comes from the route, not from
a caller-controlled body value. Separate permissions own read and modify access:

```text
backend.settings.media-transcode.read
backend.settings.media-transcode.modify
```

A persistence/readback failure is fail-closed. A required transcode must not
silently fall back to a different deployment policy because the managed setting
could not be resolved.

## Workloads

The current policy distinguishes:

- `standard`: ordinary video fallback/transcode at HD-or-lower source size;
- `deinterlace`: interlaced source video transformed to progressive output;
- `uhd-source`: source video larger than 1920x1080 and transformed to the client
  target size.

The presentation selector classifies the workload. The transcode policy chooses
the encoder implementation.

## Encoder mode semantics

### `auto`

`auto` is strict. A required video transcode starts only if an implemented
encoder backend has valid calibration evidence for the requested workload and
reaches at least **1.25x** measured real-time throughput.

Current automatic candidates are:

1. eligible measured VAAPI for a transformation implemented by the VAAPI path;
2. measured x264 preset meeting the same threshold.

If no candidate meets the contract, the presentation is unavailable. Auto never
starts a known-underpowered video worker and hopes browser buffering will hide
it.

### `software`

`software` explicitly selects the existing x264 implementation. The separate
x264 preset policy still applies. This operator mode may therefore use the typed
software-preset fallback when calibration does not select a preset:

- `deinterlace` -> `superfast`
- `standard` -> `veryfast`
- `uhd-source` -> `veryfast`

That preset fallback is **not** evidence that software is eligible for encoder
mode `auto`; it is an explicit forced-software behavior.

### `vaapi`

`vaapi` explicitly requests the implemented hardware path. It may override only
the 1.25x performance threshold. It never overrides a hard capability boundary:
if the render node is unavailable or the requested transformation is not
implemented by the VAAPI command path, the session fails closed. There is no
silent x264 fallback.

## Software x264 presets

Supported software presets are:

- `superfast`
- `veryfast`
- `faster`
- `fast`

The x264 preset mode defaults to `auto`. When trusted measured samples exist it
chooses the slowest/highest-compression preset that still reaches at least
**1.25x** for the workload. The quality-order search is:

1. `fast`
2. `faster`
3. `veryfast`
4. `superfast`

Operator controls remain separate from encoder-backend selection:

```text
VDR_SUITE_MEDIA_X264_PRESET
VDR_SUITE_MEDIA_X264_STANDARD_PRESET
VDR_SUITE_MEDIA_X264_DEINTERLACE_PRESET
VDR_SUITE_MEDIA_X264_UHD_PRESET
```

## VAAPI backend

Phase 65 implements VAAPI as the first hardware video backend. The current
command implementation is intentionally narrower than the type system: it owns
progressive H.264 output transformations with explicit positive dimensions and
does not claim the current deinterlace path.

A selected VAAPI session uses one trusted DRM render node and carries it as one
argv value. The implementation uses the hardware decode/filter/encode path and
H.264 VAAPI output while preserving the existing HLS keyframe/segment contract.

The default private device is:

```text
/dev/dri/renderD128
```

An operator may override it with:

```text
VDR_SUITE_MEDIA_VAAPI_DEVICE=/dev/dri/renderD128
```

The settings API/Web UI never accepts or returns that path. Device values remain
an operator/deployment concern and are restricted to `/dev/dri/` by policy.

QSV and NVENC remain represented by the typed backend enum but are not selectable
by this Phase-65 implementation. Calibration entries for them cannot make them
runtime candidates.

## Calibration

The installed operator tool is:

```text
vdr-suite-media-calibrate
```

Calibration is an explicit operator action and is never run during daemon startup
or playback. Run it outside active playback in a representative server state.

Measurements include source decode, the selected workload transform and encode.
For a real Recording reference they also include the normal AAC stereo fallback
when an audio stream is present.

Generated compressed fixtures use the short `--seconds` duration. Real Recording
references use `--real-seconds`, defaulting to **30 media seconds**, and skip
**15 seconds** of Recording pre-roll by default before the timed sample. Playback
itself is unaffected by this calibration-only pre-roll.

Representative sources can be supplied directly:

```bash
sudo vdr-suite-media-calibrate \
  --deinterlace-source '/srv/vdr/video.00/SciFi/Der_Wüstenplanet/2015-12-11.18.41.5-0.rec' \
  --uhd-source '/srv/vdr/video.00/Drama/A_Star_Is_Born/2026-04-21.19.16.1-0.rec'
```

A source argument may be a media file or VDR `.rec` directory; for a Recording
directory the first five-digit `.ts` segment is used.

Relevant controls include:

```text
--standard-source
--deinterlace-source
--uhd-source
--real-seconds 30
--real-start 15
--vaapi-device /dev/dri/renderD128
--no-vaapi
```

When VAAPI is enabled, the selected render node exists and FFmpeg exposes
`h264_vaapi`, the calibrator additionally benchmarks the supported VAAPI path.
A failed hardware probe does not discard completed software measurements.

The profile is written to:

```text
/var/lib/vdr-suite/media-transcode-performance.conf
```

Example version-4 schema:

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

Profile compatibility is:

- **version 3**: sustained software measurements;
- **version 4**: the same software schema plus optional hardware-backend samples
  and `vaapi.device`;
- **versions 1 and 2**: intentionally rejected because their source-cost/startup
  sampling contracts are not trustworthy enough for the current policy.

The deployment profile path can be overridden with:

```text
VDR_SUITE_MEDIA_TRANSCODE_PROFILE
```

Arbitrary FFmpeg arguments are never accepted through any of these settings.

## Deployment encoder default

The deployment-level backend default is:

```text
VDR_SUITE_MEDIA_VIDEO_ENCODER=auto
```

Accepted values are exactly:

```text
auto
software
vaapi
```

A backend-scoped Web/API selection has precedence for new sessions. Clearing it
restores this deployment value. Running sessions are never mutated in place.

## Real yaVDR evidence — interlaced HD

The Phase-65 interlaced Recording acceptance case
`/SciFi/Der_Wüstenplanet/2015-12-11.18.41.5-0.rec` established the sustained
software threshold. For the required 1080i -> 1080p25 browser transform on the
tested yaVDR host, a 30-second real-source benchmark measured:

- x264 `veryfast`: `0.992x`;
- x264 `superfast`: `1.54x`.

That evidence established the 1.25x policy and remains the reason the explicitly
forced software deinterlace path uses its proven `superfast` fallback.

## Real yaVDR evidence — UHD HEVC Main10

`/Drama/A_Star_Is_Born/2026-04-21.19.16.1-0.rec` exposed the unsafe UHD software
fallback. The HEVC Main10 3840x2160 source produced healthy four-second HLS
segments, but the x264 worker could not generate them in real time and the browser
forward buffer drained.

Sustained measurements on that source showed:

- 1080p x264 `veryfast`: `0.281x`;
- 1080p x264 `superfast`: `0.356x`;
- 720p x264 `veryfast`: `0.391x`;
- 720p x264 `superfast`: `0.468x`.

No tested software variant could sustain real time. The same source on the host's
Intel UHD Graphics 605 using `/dev/dri/renderD128` measured the 4K HEVC -> 1080p
H.264 VAAPI path at **3.825x**. A complete paced 40-second HLS reproduction then
completed with H.264 1920x1080 + AAC, independent segments and ten consecutive
approximately four-second fMP4 segments.

This evidence is why automatic encoder selection now requires measured real-time
headroom and why calibrated VAAPI is permitted where its hard transformation
contract is implemented.
