# ADR-0053: Client Playback Engine and Media Adaptation Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)

---

## Status

Accepted

Date: 2026-08-10

Planning synchronization: 2026-08-13

---

## Context

ADR-0046 defines the server-side Streaming Gateway, MediaSession, MediaRoute, ProviderStreamLease, MediaAccessGrant and PlaybackConnection boundaries. It deliberately does not choose one streaming protocol, one transcoder, one media-server library or one client playback engine.

That separation is correct, but Phase 65 also needs explicit client/player and media-adaptation rules so implementation cannot accidentally create a second media architecture.

VDR-Suite targets browser, television, mobile, desktop and future third-party clients. Those platforms have different decoder, hardware-acceleration, audio-output, subtitle, Picture-in-Picture, focus/input and lifecycle facilities.

A universal Suite-owned decoder/rendering core would duplicate mature platform work and couple every client to one media implementation. Conversely, letting each client choose provider URLs or independently decide how VDR bytes are transformed would leak provider details and fragment authorization, routing and playback semantics.

This ADR complements ADR-0046. It does not replace the MediaSession or Gateway boundary and does not itself authorize Phase-65 runtime work.

The Phase-64/65 planning rule is explicit:

```text
reliable Phase-64 Timer engine
  -> Phase 65 Streaming and Media Sessions
  -> broad polished Timer UI may follow independently
```

The broad Timer UI is not a prerequisite for Phase 65. Completion of the reliable Phase-64 Timer engine is.

---

## Decision

VDR-Suite separates **media delivery and adaptation** from **client playback execution**.

The target flow is:

```text
VDR / Recording / other private source
  -> explicitly owned local StreamProvider
  -> ProviderStreamLease
  -> VDR-Suite media adaptation boundary
  -> Streaming Gateway
  -> selected MediaSession profile
  -> client playback adapter
  -> platform playback engine
  -> platform decoder / renderer / audio output
```

The server decides which authorized media profile is delivered.

The client decides how that selected profile is decoded, rendered and integrated with the local operating system through a platform-appropriate playback engine.

### 1. No universal VDR-Suite decoder/rendering core

VDR-Suite client-common code may define a small semantic playback abstraction such as:

```text
open MediaSession
play
pause
stop
seek
select audio track
select subtitle track
read position
acknowledge discontinuity
report classified playback failure
close MediaSession
```

That abstraction does not own elementary-stream decoding, hardware-decoder selection, video rendering, audio-device implementation, operating-system media controls, platform Picture-in-Picture, platform subtitle rendering or platform audio-focus/lifecycle behavior.

Those responsibilities stay in the platform playback engine.

### 2. Kodi is an architecture reference, not a shared player dependency

Kodi demonstrates the useful separation between PVR/input integration and a general application player. VDR-Suite adopts that principle, not Kodi's player implementation.

VDR-Suite will not extract or vendor Kodi VideoPlayer as a shared Suite player core.

A future Kodi integration obtains authorized resources and media through Suite contracts and delegates playback to Kodi's own playback facilities. Kodi-specific PVR/input behavior does not become the public VDR-Suite Media Plane contract.

### 3. Use mature platform playback engines

Each first-party client uses a mature platform-appropriate playback engine behind the Suite playback abstraction.

Intended directions include:

- browser and browser-based television clients: browser media facilities plus only the compatibility/adaptation layer required for the selected profile;
- Android and Android TV: Android Media3/ExoPlayer or a later equivalent selected by the client architecture;
- Kodi: Kodi's own playback engine;
- future desktop, Apple or other native clients: an explicitly selected mature native or platform media engine.

The ADR does not freeze library versions and does not require all clients to use the same player implementation.

The browser is the initial first-party **product-validation client** for Phase 65 because it exercises the Suite Gateway, same-origin authorization model and the real codec/container constraints that the Web/TV product surfaces must satisfy. This does not make browser playback semantics the universal Media Plane contract.

### 4. Least-transformation media adaptation

For one authorized MediaSession, the selected delivery profile follows this preference order:

```text
1. pass-through
2. remux / repackage
3. transcode
```

This is a transformation preference, not a provider fallback chain.

#### Pass-through

Pass-through is selected when the client can consume the source safely and current policy, bandwidth and route constraints allow it. Elementary streams are not decoded and re-encoded merely to normalize clients.

#### Remux / repackage

Remuxing is selected when the elementary codecs are acceptable but the transport container, segmentation, seek representation or delivery protocol is not.

HLS, fragmented MP4 or another profile may be selected when demonstrated client compatibility requires it. No protocol is mandated for every client.

Remuxing stays outside VDR callbacks and VDR-held locks.

#### Transcode

Transcoding is selected only when a material constraint requires it, for example an unsupported source codec, bandwidth policy, resolution/bitrate limit or a required client profile that cannot be satisfied by pass-through or remuxing.

Transcoding is not the default compatibility strategy. The selected profile records that transcoding occurred so diagnostics can distinguish original/pass-through playback from transformed playback.

### 5. Capability negotiation is a MediaSession input

Clients request playback capabilities and preferences, not provider URLs.

Versioned capability facts may include:

```text
protocol families
containers
video codecs
audio codecs
maximum resolution
maximum bitrate
HDR capability
audio-channel capability
subtitle formats
range support
seek support
low-latency preference
pass-through acceptance
```

The selected profile is the intersection of:

```text
client capability
AND actor policy
AND Gateway capability
AND Agent capability
AND explicitly owned provider capability
AND route capacity
```

A client must tolerate selection of a supported profile different from its first preference.

The client never selects `streamdev`, `suitebridge`, a filesystem path or another private provider through public capability negotiation.

### 6. Streamdev remains a private provider

Streamdev may be an explicitly owned internal StreamProvider when deliberately configured and its capabilities match the route.

It is not the public playback API, the client authorization boundary, a universal VDR-Suite dependency, an implicit fallback or a reason for clients to learn Streamdev URLs or credentials.

Availability alone never authorizes provider use. An active route never silently switches provider.

A future SuiteBridge/VDR-native media provider may be added after source review, bounded native integration, capability contracts and real-VDR acceptance. Its existence does not bypass explicit provider ownership or route selection.

### 7. The VDR plugin is a media-source boundary, not a media server

A VDR-local Suite media capability may perform only the minimum VDR-native work required to establish and close a safe local media source.

It does not own public HTTP/HLS endpoints, TLS or end-user credentials, public MediaSession state, arbitrary client backpressure, remux/transcode work under VDR locks, a global timeshift store or platform decoder/rendering behavior.

Slow network delivery and expensive media processing remain outside VDR callbacks and VDR-held locks.

### 8. Media-processing engines remain replaceable

The adaptation layer exposes narrow pass-through, remux and transcode capabilities rather than making one processing library part of the public contract.

Phase 65 may select FFmpeg/libav-based tooling or another proven implementation behind that boundary after build, packaging, licence, hardware-acceleration, failure-isolation and operational tests.

FFmpeg command-line or library semantics are not exposed to clients, Agents or providers.

### 9. Player state and persistent playback state are distinct

Transient decoder, render-surface, buffer, volume, local focus and temporary UI state remain client-local unless a server media operation requires them.

Persistent user playback state such as Recording resume position, watched/progress state and explicit progress reset is a Suite domain concern when implemented.

Persistent progress is bound to stable Suite media identity and actor scope, not a provider URL, Recording path or player-specific identifier.

### 10. Track, seek and growing-Recording semantics are normalized

MediaSession metadata may expose normalized audio/subtitle tracks and selected/default tracks. Clients select tracks by Suite media-session semantics, not provider-specific PIDs or Streamdev parameters as durable public identifiers.

Seek capability must be truthful. A client must not be offered arbitrary seek when the selected profile or source cannot support it.

A Recording that is still being written is explicitly represented as **growing**. Its seek window, current readable extent and end-of-stream behavior must not pretend that it is a complete immutable file.

The Recording Golden User Journey requires play, supported seek, stop and later durable resume when resume/progress is enabled.

### 11. Timeshift remains explicit later capability

Ordinary live playback does not silently imply timeshift.

Future timeshift requires explicit buffer identity, owner/storage placement, retention/quota, live edge and seek window, reconnect semantics, cleanup, capacity accounting, rights and audit behavior.

The logical contract belongs to the Media Plane. The VDR plugin does not become the global timeshift coordinator.

### 12. Backpressure and cleanup stay outside VDR

The Streaming Gateway and media-worker boundary absorb slow clients, reconnect behavior and protocol buffering.

A slow or disconnected client must not cause a VDR callback to wait for client reads, an unbounded VDR-process queue, an indefinitely retained VDR receiver or remux/transcode work while a VDR lock is held.

ProviderStreamLease expiry, route cleanup and bounded buffering are mandatory.

### 13. Failure stays classified and visible

Media failures follow the same product rule as Golden User Journey 5:

```text
failure
  -> classified Suite-visible state
  -> no silent provider switch
  -> no hidden unsafe recovery
  -> deterministic cleanup or bounded reconciliation
  -> understandable client/operator result
```

A disconnected route, expired grant, provider-generation mismatch, unsupported profile and player/decoder failure remain distinguishable where the distinction changes recovery or diagnostics.

---

## Client Playback Contract

A future shared client contract stays small and semantic.

Illustrative shape:

```text
PlaybackSession
  mediaSessionId
  selectedProfile
  capabilities
  availableTracks
  liveOrRecording
  growing
  seekWindow
  expiresAt

PlaybackController
  open(session)
  play()
  pause()
  stop()
  seek(position)
  selectAudio(track)
  selectSubtitle(track)
  state()
  close()
```

This is an architectural shape, not a frozen public SDK.

Browser JavaScript, Android, Kodi and future client APIs may differ while preserving the same Suite semantics.

---

## Phase 65 Implementation Direction

ADR-0046 remains authoritative for the server-side MediaSession/Gateway boundary. Phase 65 uses **coherent vertical product slices**, not a long chain of mechanically tiny intermediate PRs.

### Gate before Phase 65

No Phase-65 runtime work is authorized until the reliable Phase-64 Timer engine satisfies its completion gates.

The broad polished Timer UI is explicitly **not** part of that gate and may be completed later once account/backend access management is ready.

### First coherent vertical proof — Recording playback

The first media product proof should establish one complete authenticated path:

```text
Recording detail
  -> authorized MediaSession
  -> explicit MediaRoute / ProviderStreamLease
  -> Gateway
  -> selected least-transformation profile
  -> first-party browser playback adapter
  -> picture + sound
  -> stop / deterministic cleanup
```

This proof must not expose a provider URL to the client. It must prove that pass-through does not re-encode when the browser can consume the source. If browser compatibility demonstrates that only packaging is unsuitable, remux/repackage may be added without jumping directly to transcode.

### Second coherent vertical proof — Live TV

The live path then proves:

```text
channel / EPG selection
  -> authorized MediaSession
  -> live provider lease
  -> Gateway
  -> browser playback
  -> picture + sound
  -> channel change
  -> old route/lease/receiver released deterministically
```

A slow or disconnected client must not retain unbounded VDR resources.

### Recording semantics after basic playback

Add truthful seek/range behavior and growing-Recording handling only after the basic vertical path works. Persistent resume/progress follows only with stable Suite identity and actor ownership.

### Compatibility escalation

Add remux only for demonstrated container/protocol compatibility need. Add transcoding only as a separately observable and capacity-governed capability after pass-through/remux behavior is proved. Add timeshift only as a later explicit buffer/session capability.

This sequencing is a product-risk order, not an authorization to bypass ADR-0046 security, provider-ownership, route-fencing or real-VDR gates.

---

## Testing and Acceptance

Phase-65 implementation is not accepted from component CI alone. Golden User Journeys 1, 2 and the media portion of Journey 5 are product acceptance requirements as their capabilities land.

### Server/media tests

Tests must prove:

- a pass-through-capable client receives pass-through without re-encoding;
- remux is selected only when codecs are acceptable but packaging/protocol is not;
- transcode is not selected when a lower-transformation valid profile exists;
- unsupported capability requests fail or select an explicitly supported alternative without provider leakage;
- provider identity is not client-selectable;
- an active route does not silently switch provider;
- disconnect, revocation and route replacement release bounded provider resources;
- slow-client behavior does not block VDR callbacks or retain unbounded queues;
- track and seek advertisement is truthful;
- growing Recordings expose a truthful readable/seek extent;
- timeshift is not advertised before its explicit buffer contract exists.

### First-party browser acceptance

The initial browser adapter must prove on real media:

- actual **picture and sound**, not merely an HTTP 200 or manifest fetch;
- MediaSession expiry/revocation handling;
- stop and lifecycle cleanup;
- no provider URL construction in browser code;
- classified playback failure visible through Suite semantics;
- supported seek for Recordings once seek is enabled;
- live channel change with deterministic replacement/cleanup once live playback is enabled.

Codec/container support must be tested with representative VDR material. A browser limitation is evidence for remux or, only when necessary, transcode; it is not a reason to force all clients into one universal profile.

### Other first-party client tests

Each later client adapter proves, as applicable, play/pause/seek mapping, track selection, reconnect/discontinuity handling, platform lifecycle cleanup, provider privacy and stable progress identity.

### Real VDR acceptance

The first VDR-native live media source must prove:

- expected temporary receiver/resource allocation only;
- no Timer, Recording, channel-list, setup or filesystem mutation;
- no network wait, remux or transcode under VDR locks;
- client disconnect, Gateway revocation, channel change and shutdown release the source;
- provider URLs, credentials and local paths remain private;
- rollback leaves no stale public listener or retained receiver.

Recording playback acceptance must additionally prove truthful behavior for at least one completed Recording and one growing Recording once growing playback is implemented.

---

## Alternatives Considered

### Make Streamdev the VDR-Suite streaming architecture

Rejected. Streamdev may be an internal explicitly owned provider, but it does not own VDR-Suite authorization, MediaSession lifecycle, client compatibility or multi-site routing.

### Extract Kodi VideoPlayer and use it everywhere

Rejected. This would import Kodi-specific player, rendering, platform and maintenance assumptions into every Suite client.

### Build a new VDR-Suite decoder and renderer

Rejected. VDR-Suite's differentiating responsibility is media orchestration and a stable multi-client Media Plane, not reimplementing hardware-decoder, audio-output and rendering stacks.

### Transcode every stream to one universal profile

Rejected. It wastes compute, increases latency, can reduce quality and enlarges the failure/capacity surface when lower-transformation delivery is valid.

### Mandate HLS for every client

Rejected. HLS may be an important compatibility profile, especially for browsers and televisions, but it is not inherently the optimal profile for every client.

### Let each client choose provider and transformation path

Rejected. This leaks topology and credentials, fragments policy and prevents central route/capacity enforcement.

### Put timeshift directly into the VDR plugin

Rejected as the general architecture. Timeshift is a Media Plane resource with quota, seek-window, reconnect and cleanup semantics.

---

## Consequences

Positive:

- no cross-platform Suite codec/rendering engine;
- platform hardware acceleration and lifecycle integration remain available;
- Kodi knowledge is reused architecturally without importing Kodi internals;
- Streamdev stays replaceable and private;
- pass-through preserves source quality and minimizes resource use;
- remux/transcode become explicit capability and capacity decisions;
- browser, TV and native clients can receive different compatible profiles through one MediaSession model;
- product acceptance is vertical and user-visible rather than inferred from transport success;
- persistent resume/progress can remain consistent across devices;
- future timeshift remains additive.

Trade-offs:

- each client family needs a playback adapter and platform-specific acceptance;
- capability negotiation must handle imperfect device codec support;
- browser/TV compatibility may require multiple media profiles;
- remux/transcode workers add operational complexity when enabled;
- cross-device resume introduces an actor-scoped persistent domain.

---

## Non-Goals

This ADR does not select final public MediaSession endpoint paths, one mandatory protocol, one HLS/DASH/WebRTC implementation, one FFmpeg version, codec ladders, DRM, offline download, final timeshift storage, final progress endpoint schema or a Kodi add-on API surface.

---

## Acceptance Criteria

The architecture decision is satisfied only when implemented slices preserve all of the following:

- authorized selected media profiles never expose private provider details;
- first-party client-common code does not implement a universal codec/rendering stack;
- at least one production client delegates decode/rendering to its platform playback engine;
- the initial browser product proof reaches real picture + sound through Suite-owned contracts;
- Live TV channel change releases/replaces the previous route, lease and VDR resource deterministically;
- Recording playback exposes truthful supported seek and growing state;
- pass-through is preferred over remux, and remux over transcode, whenever the lower-transformation profile is valid;
- transcoding is explicit, observable and capacity-governed rather than the default;
- Streamdev is never the public client contract or implicit provider fallback;
- provider ownership and route fencing remain explicit;
- VDR callbacks and locks do not own network backpressure, remux or transcode work;
- persistent resume/progress, when implemented, is actor-scoped and bound to stable Suite media identity;
- timeshift remains unavailable until its explicit buffer/session contract is implemented and tested;
- Golden User Journeys 1, 2 and applicable failure behavior from Journey 5 pass with required real-client/real-VDR evidence.

Acceptance of this ADR is not Phase-65 runtime completion.

---

## Related Decisions and Product Gates

- [Golden User Journeys](../planning/golden-user-journeys.md)
- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)