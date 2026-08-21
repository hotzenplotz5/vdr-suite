# Phase 65.D — Media Transcode Backend Policy and Output Settings

Status: **Accepted and closed for the bounded Phase-65.D media-transcode backend policy/settings scope.**

Binding architecture: ADR-0046, ADR-0053 and ADR-0055.

## Purpose

Recording and Live playback must choose a server-side video transformation that
is browser-compatible **and** sustainable in real time. Codec or pixel-format
compatibility alone is not enough: decode cost, deinterlacing, scaling and
high-resolution encode cost can change the server workload substantially.

The server does not infer capability from CPU/GPU model names. Virtualization,
clocking, thermal limits, concurrent work, drivers and encoder/library versions
make that unreliable. VDR-Suite uses typed workloads plus measured throughput.

Phase 65.D also owns the backend-scoped operator/output settings that select the
media-transcode backend for **new** MediaSessions. It does not retarget an active
worker and does not expose arbitrary FFmpeg arguments or raw hardware paths to
the browser.

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

Phase 65.D implements VAAPI as the first hardware video backend. The current
command implementation is intentionally narrower than the type system: it owns
progressive H.264 output transformations with explicit positive dimensions and
does not claim the current deinterlace path.

A selected VAAPI session uses one trusted DRM render node and carries it as one
argv value. The implementation uses the hardware decode/filter/encode path and
H.264 VAAPI output while preserving the selected delivery profile contract.

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
runtime candidates. VDPAU is not introduced.

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
software threshold. Earlier sustained measurements for the required 1080i ->
1080p25 browser transform included:

- x264 `veryfast`: `0.992x`;
- x264 `superfast`: `1.54x`.

The final version-4 profile used during the accepted Phase-65.D runtime gate had
newer measured values in which `veryfast` exceeded the 1.25x threshold. Auto
therefore selected `veryfast` as the highest-quality measured preset meeting the
current threshold, while preserving `bwdif` for the interlaced workload.

## Real yaVDR evidence — UHD HEVC Main10

`/Drama/A_Star_Is_Born/2026-04-21.19.16.1-0.rec` exposed the unsafe UHD software
fallback. Earlier sustained measurements showed that tested x264 variants could
not sustain real time while VAAPI had substantial headroom. The accepted
Phase-65.D run again selected the real VAAPI path for UHD and sustained playback
far beyond the previous approximately 32-second HTTP cutoff.

The actual worker used hardware VAAPI decode/filter/encode with `h264_vaapi` and
`scale_vaapi`, and no silent x264 fallback was observed. The stream remained
stable without the previous Broken Pipe / mux / trailer / close-file failure.

This evidence is why automatic encoder selection requires measured real-time
headroom and why calibrated VAAPI is permitted only where its hard transformation
contract is implemented.

## Accepted Phase-65.D closeout evidence

PR #208 completed the bounded media-transcode backend policy/settings vertical on
the exact accepted candidate:

```text
accepted_65d_candidate=85478311b9af6c027a25980272a2acde551e5508
source_ci_workflow=VDR-Suite CI
source_ci_run_number=7976
source_ci_run_id=32415860281
source_ci_result=PASS
merge_pr=208
merge_commit=8716bbe9f1ab8ebd4cdf597d620419ef0fcf098a
```

Real yaVDR acceptance established all of the following on the production
VDR/SuiteBridge/media path:

- Auto UHD Recording selected VAAPI and sustained long playback without the prior Broken Pipe cutoff;
- Auto interlaced HD selected `libx264 -preset veryfast` plus `bwdif` from valid calibration evidence;
- backend-scoped Software selection survived page reload and daemon restart;
- forced Software Recording and Live paths used x264 rather than VAAPI;
- changing the backend setting did not retarget an already active worker;
- forced VAAPI UHD used `h264_vaapi` with no x264 fallback;
- forced VAAPI Live failed closed because the current Live deinterlace transform is unsupported by the VAAPI command path;
- the failed Live MediaSession persisted `state=failed` with terminal reason `forced_vaapi_transformation_unsupported` and started no FFmpeg fallback worker;
- switching the same Live path to forced Software restored playback with x264 + `bwdif`;
- returning to Auto restored the calibrated x264 Live path;
- the daemon remained active throughout the acceptance sequence.

Phase 65.D is therefore closed for its bounded output-policy/settings scope. The
next planned Phase-65 vertical is 65.E Client playback abstraction; Phase 66 is
not started by this closeout.
