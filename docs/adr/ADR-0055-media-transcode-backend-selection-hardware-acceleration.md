# ADR-0055: Media Transcode Backend Selection and Hardware Acceleration Policy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053: Client Playback Engine and Media Adaptation Strategy](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)

---

## Status

**Accepted**

Date: 2026-08-20

Accepted during Phase 65 after the completed-Recording progressive-fMP4 startup path was accepted on real yaVDR hardware. This ADR refines the existing Phase-65 adaptation architecture. It does not authorize Phase 66 or change the strict phase order.

---

## Context

ADR-0053 defines the media-adaptation preference:

```text
pass-through -> remux/repackage -> transcode
```

That ordering is now proven important in the real Recording browser path. Compatible completed Recordings can start practically immediately when VDR-Suite avoids unnecessary HLS buffering and performs only the transformation that the selected client profile actually needs.

VDR-Suite already contains a server-side `MediaTranscodePolicy`, `vdr-suite-media-calibrate`, typed software encoder presets and a first accepted VAAPI UHD path. The existing calibration profile can record measured software x264 throughput and VAAPI throughput, and auto policy can reject an implementation that cannot sustain the required real-time headroom.

The current implementation, however, mixes several concerns that must remain architecturally separate:

1. choosing the delivery/presentation profile;
2. deciding whether video needs transcoding at all;
3. selecting a video encoder backend when transcoding is required;
4. selecting an x264 preset when the software backend is used;
5. discovering execution-host hardware capability and calibrated performance;
6. exposing an administrator-facing configuration surface.

Hardware acceleration must not become shorthand for HLS. HLS can be a copy/remux fallback with no video encoding, while progressive fMP4 can also require a real video transcode for an incompatible source. Conversely, a compatible H.264/AAC Recording must never be transcoded merely because VAAPI is available or selected in configuration.

The current operator controls are predominantly environment based. VDR-Suite already has an established browser administration pattern for backend-scoped settings: the selected backend owns a settings card, settings are read/written through `/api/backends/<backendId>/settings/...`, browser mutations use the normal session/CSRF boundary, managed values are persisted server-side, and deployment environment values can remain a fallback. The video-transcode policy shall use that product pattern rather than remain a hidden environment-only feature.

---

## Decision

VDR-Suite separates **media adaptation** from **video encoder backend selection**.

The decision chain is:

```text
source + client capabilities
  -> select truthful presentation profile
  -> determine per-track action
       video: omit | copy | transcode
       audio: omit | copy | transcode
  -> only if video == transcode:
       select video encoder backend
       -> auto | software | vaapi
  -> build the protocol-specific command plan
       progressive-fMP4 or HLS
```

The video encoder policy is therefore evaluated only when:

```text
videoAction == Transcode
```

For `Copy` or `Omit`, encoder configuration is irrelevant and must not alter the selected track action.

This ADR supplements ADR-0053. It does not replace least-transformation selection, MediaSession authority, Gateway authorization, provider ownership or truthful seek/range semantics.

---

## Supported administrative modes

The first supported video encoder modes are:

```text
auto
software
vaapi
```

Their meaning is stable across HLS and progressive-fMP4.

### `auto`

VDR-Suite chooses only among encoder implementations that are:

- actually implemented for the requested transformation;
- valid on the current execution host;
- represented by trustworthy calibration evidence for the requested workload when calibration is required;
- able to meet the configured minimum real-time headroom contract.

Missing implementation, missing required calibration, unsupported filtering/deinterlacing or an invalid hardware device makes a backend ineligible for automatic selection.

### `software`

Video transcoding is forced to the software x264 backend.

This selects the backend only. The existing x264 preset policy remains orthogonal: an x264 preset may itself be `auto`/measured or explicitly overridden through the existing preset controls.

A forced software mode may intentionally override automatic hardware preference, but it may not turn a `Copy` profile into a transcode profile.

### `vaapi`

Video transcoding is forced to VAAPI when the exact requested transformation has an implemented and valid VAAPI command plan on the current execution host.

Forced VAAPI does not silently fall back to software. If the transformation or device is unsupported, the transcode plan fails closed and the presentation selector/runtime may choose another already-authorized presentation path only when that path is independently truthful and compatible.

A forced backend is an explicit operator override. It is not required to satisfy the automatic performance threshold, but the UI and diagnostics must expose known calibration evidence and warn when measured throughput is below the normal automatic threshold. Hard capability/command-plan validity can never be overridden.

---

## Automatic selection policy

Automatic backend selection is deterministic and workload aware.

The initial implemented policy is:

```text
videoAction != Transcode
  -> no encoder selection

videoAction == Transcode
  -> determine workload and supported implementations
  -> consider calibrated hardware implementations that meet threshold
  -> prefer an eligible accepted hardware implementation
  -> otherwise select a calibrated software x264 preset that meets threshold
  -> if no implementation satisfies the contract, fail closed
```

The default minimum real-time throughput remains the existing calibrated headroom contract unless a later accepted decision changes it. At the time of this ADR that contract is `1.25x`.

Automatic selection must not infer suitability from CPU/GPU model names, PCI IDs, browser user agents or the mere existence of `/dev/dri`.

A hardware backend is not automatically preferable merely because it is hardware. It must first be implemented for the workload and supported by measured evidence. The deterministic preference for an eligible hardware implementation reflects the server objective of preserving CPU capacity once both capability and throughput have been proven.

---

## Workload and implementation support

Backend eligibility is the intersection of:

```text
requested transformation
x implemented command plan
x execution-host capability
x calibration evidence required by auto policy
```

Examples:

### Compatible H.264 + AAC Recording

```text
video = copy
audio = copy
container = progressive fMP4
encoder policy = not consulted
```

No VAAPI or x264 process is selected merely because the setting is `auto` or `vaapi`.

### H.264 + AC3 browser Recording

```text
video = copy
audio = AAC transcode
encoder policy = not consulted
```

Audio-only transcoding does not activate video hardware selection.

### HEVC / MPEG-2 source requiring browser H.264

```text
video = transcode to H.264
encoder policy = consulted
```

The selected delivery profile may be progressive-fMP4 or HLS. That protocol choice does not change the meaning of the backend setting.

### Growing Recording using HLS fallback

If HLS is selected because the growing source cannot truthfully use the completed-Recording fast path, HLS itself does not imply video transcoding. A compatible video track remains copy/remux whenever ADR-0053 and the growing-Recording contract permit it.

---

## VAAPI, QSV, NVENC and VDPAU

The first accepted hardware encoder backend is VAAPI.

```text
VAAPI   -> supported where an implemented command plan and acceptance exist
QSV     -> modeled but unavailable/fail-closed until implemented and accepted
NVENC   -> modeled but unavailable/fail-closed until implemented and accepted
VDPAU   -> not introduced as a VDR-Suite video encoder backend
```

A future QSV or NVENC implementation requires its own command-plan tests, calibration support and real-system acceptance before `auto` may select it or the Webfrontend may present it as an available forced mode.

Enum presence alone is not runtime support.

---

## Calibration and execution-host capability

`vdr-suite-media-calibrate` remains the authority for measured transcode throughput.

Calibration evidence belongs to the host that executes the FFmpeg transcode worker, not merely to the VDR source identity.

This distinction is mandatory for multi-site evolution:

```text
backend/source policy
  != execution-host hardware capability
```

The administrator may choose a policy in the settings of a VDR-Suite backend, but the actual eligibility of VAAPI/x264 is evaluated against the host on which the selected MediaSession transcode worker will execute.

As long as Recording/Live transcode workers run on the Control Plane host, that host's calibration profile and hardware device facts are authoritative. If a later architecture moves transcoding to an Agent/backend host, that execution host must provide its own typed capability/calibration evidence; a Control Plane calibration must never be reused as proof for another machine.

The calibration process remains explicit. VDR-Suite does not benchmark at daemon startup or in the playback hot path.

---

## Configuration model and precedence

The canonical logical setting is a video encoder mode:

```text
auto | software | vaapi
```

The deployment environment remains a backward-compatible default/fallback. The intended deployment default variable is:

```text
VDR_SUITE_MEDIA_VIDEO_ENCODER=auto|software|vaapi
```

The implementation must first verify existing environment/config conventions before introducing the final variable spelling, but the logical precedence defined here is binding:

```text
backend-scoped managed setting
  -> deployment/environment default
  -> built-in default: auto
```

Existing settings such as:

```text
VDR_SUITE_MEDIA_X264_PRESET
VDR_SUITE_MEDIA_X264_STANDARD_PRESET
VDR_SUITE_MEDIA_X264_DEINTERLACE_PRESET
VDR_SUITE_MEDIA_X264_UHD_PRESET
VDR_SUITE_MEDIA_VAAPI_DEVICE
VDR_SUITE_MEDIA_TRANSCODE_PROFILE
```

remain separate concerns and are not silently deleted by this ADR.

In particular:

- video encoder mode chooses the backend;
- x264 preset settings choose software encoder quality/speed when software is selected;
- the VAAPI device identifies a private execution-host device;
- the transcode profile identifies calibration evidence.

A migration must preserve existing deployments that rely on these variables.

---

## Backend settings Webfrontend

The primary administrator-facing control belongs in the existing **Backend settings** area of the Webfrontend.

The UI must operate on the currently selected backend and follow the established backend-settings security/runtime pattern.

The first UI surface contains at least:

```text
Video-Transcoding

Encoder:
  Deployment default
  Automatic (recommended)
  Software (x264)
  VAAPI
```

`Deployment default` is not a fourth runtime encoder mode. It removes the managed backend override and exposes the deployment/environment/default value again.

The UI should additionally present bounded diagnostic state useful for an administrator, for example:

- effective encoder mode;
- whether the value is managed or inherited;
- calibration profile present/valid;
- minimum real-time threshold;
- software calibration status for known workloads;
- VAAPI implemented/available/calibrated/suitable state;
- a human-readable reason when a backend is unavailable;
- warning state when a forced backend has measured performance below the automatic threshold.

The UI must not expose raw secret material or turn local device paths into a general client contract.

The VAAPI render-device path remains private execution-host configuration. The browser may show a bounded status such as `VAAPI device configured/available`; it does not need the raw `/dev/dri/...` path to administer the normal encoder policy.

---

## Backend settings API and persistence

The implementation should follow the existing backend settings API pattern with a dedicated media-transcode settings resource, conceptually:

```text
GET  /api/backends/<backendId>/settings/media-transcode
POST /api/backends/<backendId>/settings/media-transcode
```

The exact route spelling may be adjusted to existing API naming conventions during implementation, but the contract must remain backend scoped.

A GET returns a sanitized snapshot containing enough typed state to render the UI and explain the effective decision. A POST may set a managed mode or clear the managed override.

The managed non-secret mode belongs in normal durable VDR-Suite configuration persistence, not a root-only secret file.

The settings mutation must preserve the existing security boundary:

- authenticated browser session;
- backend-scoped administrative authorization;
- CSRF protection;
- route/backend identity agreement;
- normal accountability/audit handling;
- fail-closed validation of unsupported values.

Raw FFmpeg arguments, arbitrary device paths, codec strings or command fragments are never accepted from the browser.

---

## Configuration lifetime

Encoder policy is resolved when a new MediaSession/transcode worker is provisioned.

Changing the backend setting does not rewrite or mutate an already active worker.

```text
settings change
  -> affects subsequently provisioned MediaSessions
  -> existing active MediaSession keeps its resolved profile/backend
```

This preserves session determinism and avoids mid-stream encoder replacement.

The Webfrontend must state this truthfully. A daemon restart should not be required merely to apply a managed policy to future sessions unless the implementation proves an unavoidable process-level constraint.

---

## Protocol independence and HLS fallback

HLS remains a compatibility/risk fallback where the selected media semantics require it. It is not the hardware-acceleration mode.

The following are separate decisions:

```text
Why is HLS selected?
  -> delivery/source/client semantics

Does video require transcoding?
  -> track compatibility/adaptation

Which encoder performs that transcode?
  -> this ADR
```

Therefore both the HLS builder and progressive-fMP4 builder must consume the same resolved video encoder decision for equivalent video-transcode workloads.

A protocol-specific builder may still reject a transformation that it cannot implement safely. It must not silently select another backend or invent a different adaptation class.

---

## Failure and fallback semantics

### Auto mode

`auto` may fall through the ordered set of eligible implementations because fallback is part of the automatic policy.

Example:

```text
VAAPI missing/uncalibrated/too slow
  -> eligible calibrated x264 exists
  -> select x264
```

If no implementation satisfies the automatic real-time contract, the transcode profile is unavailable. A higher-level selector/runtime may choose another truthful compatible presentation only if one independently exists.

### Forced mode

A forced backend is not silently substituted.

```text
forced vaapi + unsupported transformation
  -> fail closed
  -> do not silently run x264

forced software
  -> use software path
  -> do not silently switch to VAAPI
```

This makes administrator intent observable and debuggable.

---

## Observability

The resolved MediaSession diagnostic model must make the backend decision explainable without exposing provider-native paths or secret configuration.

At minimum, internal/test diagnostics should distinguish:

```text
video action
requested encoder mode
effective encoder backend
transcode workload
software preset when applicable
configuration source: managed | environment | default
calibration decision / threshold result
fallback or rejection reason
```

The Webfrontend may expose a safe subset of these facts in Backend settings.

A playback failure caused by an unavailable forced backend must not appear as a generic unexplained media error when a safe reason code can be returned/logged.

---

## Security and trust boundary

Client capability input may influence presentation selection only through the existing typed capability model. It may not select arbitrary FFmpeg encoders, filters, devices or command-line arguments.

The browser settings API accepts only allowlisted administrative modes.

Hardware device discovery and calibration evidence are server/execution-host facts.

Provider-native Recording paths, private live sockets and render-device paths remain private implementation data.

Remote backend reachability does not make remote hardware authoritative for a transcode worker executing elsewhere.

---

## Consequences

### Positive

- compatible Recording playback keeps the accepted fast copy/remux path;
- HLS remains available without being conflated with transcoding;
- hardware acceleration becomes predictable and administrable;
- automatic selection is evidence based rather than machine-name based;
- software and hardware fallback behavior is explicit;
- first-party administrators can configure the policy in the product UI;
- environment settings remain usable for deployment automation and backward compatibility;
- future QSV/NVENC support has a clear acceptance gate;
- multi-site execution-host capability remains correctly separated from source/backend identity.

### Costs

- a managed backend settings service/API/UI must be added;
- current policy wiring must be audited so Recording, Live and HLS/progressive paths do not bypass the same selection rules;
- runtime diagnostics need to retain requested/effective policy facts;
- calibration and backend availability need typed snapshot data suitable for the UI;
- forced-mode failures must be surfaced clearly rather than hidden by automatic fallback.

---

## Implementation direction

The first implementation slice after this ADR should remain within Phase 65 and must not start Phase 66.

It should:

1. audit every current `MediaTranscodePolicy` construction/application site;
2. identify HLS, Recording progressive-fMP4 and Live progressive-fMP4 paths that consume or bypass the policy;
3. introduce the typed encoder mode without changing `Copy`/`Omit` track actions;
4. preserve existing x264 preset and calibration behavior;
5. implement deterministic `auto|software|vaapi` semantics;
6. add backend-scoped managed persistence and API following the existing settings pattern;
7. add a Webfrontend Backend-settings card for the selected backend;
8. show effective mode and bounded calibration/backend availability diagnostics;
9. apply setting changes only to newly provisioned sessions;
10. regression-test protocol independence and audio-only transcode behavior;
11. run real yaVDR acceptance for copy/remux, software transcode, automatic selection and VAAPI where available;
12. confirm HLS remains a fallback/compatibility path and the accepted fast completed-Recording startup does not regress.

QSV/NVENC runtime implementation, VDPAU encoding and Phase-66 work are outside this slice.

---

## Acceptance criteria

ADR-0055 is implemented only when all of the following are true:

- `Copy` and `Omit` video actions never invoke or get changed by encoder backend selection;
- audio-only transcoding never activates the video encoder policy;
- `auto`, `software` and `vaapi` have deterministic tested semantics;
- automatic VAAPI selection requires implemented workload support and sufficient accepted calibration evidence;
- automatic software fallback uses the existing calibrated x264 policy;
- no eligible real-time backend fails closed rather than starting a known-unsustainable automatic transcode;
- forced VAAPI never silently falls back to x264;
- forced software never silently changes to VAAPI;
- HLS and progressive-fMP4 consume the same resolved backend policy where equivalent video transcoding is required;
- HLS selection alone does not trigger video transcoding;
- QSV/NVENC remain unavailable until separately implemented and accepted;
- VDPAU is not exposed as an encoding backend;
- the Webfrontend exposes the policy in the selected backend's settings;
- managed settings use authenticated, CSRF-protected, backend-authorized persistence and can be reset to deployment default;
- raw hardware paths and FFmpeg arguments are not browser-controlled;
- active MediaSessions remain stable across a settings change and new sessions use the new policy;
- the completed-Recording progressive-fMP4 copy/remux startup path retains its real-system low-latency behavior;
- real yaVDR acceptance proves at least one actual video-transcode path and the expected fallback/fail-closed behavior.

---

## Non-goals

This ADR does not:

- make HLS the default Recording playback protocol;
- replace ADR-0053 least-transformation selection;
- add generic user-agent/browser-brand routing;
- expose provider-native media paths;
- make raw FFmpeg configuration a public API;
- implement QSV or NVENC by declaration;
- add VDPAU as an encoder backend;
- define Phase-66 Teletext/HbbTV behavior;
- complete the remaining Phase-65.C seek/growing-Recording contracts merely by defining encoder policy.

---

## Back

- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Documentation Index](../index.md)
