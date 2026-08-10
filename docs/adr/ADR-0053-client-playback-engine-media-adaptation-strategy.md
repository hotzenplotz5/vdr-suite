# ADR-0053: Client Playback Engine and Media Adaptation Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)

---

## Status

Proposed

Date: 2026-08-10

---

## Context

ADR-0046 defines the server-side Streaming Gateway, MediaSession, MediaRoute, ProviderStreamLease, MediaAccessGrant and PlaybackConnection boundaries. It deliberately does not choose one streaming protocol, one transcoder, one media-server library or one client playback engine.

That separation is correct, but it leaves several decisions that must be fixed before Phase 65 implementation can accidentally create a second media architecture:

```text
Does VDR-Suite build its own decoder and renderer?
Should Kodi VideoPlayer code become a shared player core?
Does Streamdev become the foundation of every VDR playback path?
Should every client receive the same container and codec profile?
When is remuxing preferred to transcoding?
Who owns persistent resume state versus transient player state?
Where may buffering and future timeshift work execute?
```

VDR-Suite targets browser, television, mobile, desktop and future third-party clients. Those platforms have different decoder, hardware acceleration, audio-output, subtitle, Picture-in-Picture, focus/input and lifecycle facilities.

A universal Suite-owned decoder/rendering core would duplicate mature platform work, increase device-specific hardware-decoder responsibility and couple client evolution to one media implementation.

At the same time, letting every client choose a provider URL or independently decide how VDR bytes are transformed would leak provider details and fragment playback semantics.

This ADR complements ADR-0046. It does not replace the MediaSession or Gateway boundary and does not advance Phase 65 runtime work.

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

### 1. VDR-Suite does not implement a universal decoder/rendering core

VDR-Suite client-common code may define a small playback abstraction for Suite semantics such as:

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

That abstraction does not own:

- elementary-stream decoding;
- hardware-decoder selection;
- video rendering;
- audio-device implementation;
- operating-system media controls;
- platform Picture-in-Picture implementation;
- platform-specific subtitle rendering;
- platform-specific audio focus or lifecycle behavior.

Those responsibilities stay in the platform playback engine.

### 2. Kodi is an architecture reference, not a player-code dependency

Kodi demonstrates the useful separation between a PVR/input integration and the general player used by the application.

VDR-Suite adopts that principle, not Kodi's player implementation.

VDR-Suite will not extract or vendor Kodi VideoPlayer as a shared Suite player core.

A future Kodi integration should obtain authorized VDR-Suite resources and media through Suite contracts and then delegate playback to Kodi's own player facilities.

Kodi-specific PVR/input behavior must not become the public VDR-Suite Media Plane contract.

Direct Kodi source reuse would require a separate dependency, maintenance and licence decision. It is not required by this architecture.

### 3. Platform playback engines are preferred

Each first-party client uses a mature platform-appropriate playback engine behind the Suite playback abstraction.

Examples of the intended direction are:

- browser and browser-based television clients: browser media facilities plus only the compatibility/adaptation layer required for the selected profile;
- Android and Android TV: Android Media3/ExoPlayer or a later equivalent platform-standard engine selected by the client architecture;
- Kodi: Kodi's own playback engine;
- future desktop, Apple or other native clients: an explicitly selected mature native or platform media engine.

The ADR does not freeze library versions and does not require all clients to use the same player implementation.

A new client may choose another mature engine when it satisfies the same MediaSession, capability, security and test contracts.

### 4. Media adaptation follows a least-transformation rule

For one authorized MediaSession, the selected delivery profile follows this preference order:

```text
1. pass-through
2. remux / repackage
3. transcode
```

This is a policy order, not an automatic provider fallback chain.

#### Pass-through

Pass-through is selected when the client can consume the source media safely and current policy, bandwidth and route constraints allow it.

The elementary streams are not decoded and re-encoded merely to normalize clients.

Benefits include:

- original picture and audio quality;
- lower CPU/GPU load;
- lower energy use;
- lower latency;
- fewer media transformations that can fail.

#### Remux / repackage

Remuxing is selected when the elementary codecs are acceptable to the client but the transport container, segmentation, seek representation or delivery protocol is not.

Possible profiles may include transport-stream pass-through, HLS or fragmented-MP4-style delivery, but this ADR does not mandate one protocol for every client.

Remuxing must remain outside VDR callbacks and VDR-held locks.

#### Transcode

Transcoding is selected only when required by a material constraint such as:

- unsupported source video or audio codec;
- bandwidth policy;
- maximum resolution or bitrate;
- remote-site capacity policy;
- a required client profile that cannot be satisfied by pass-through or remuxing.

Transcoding is not the default compatibility strategy.

The selected profile records that transcoding occurred so diagnostics and clients can distinguish original/pass-through playback from transformed playback.

### 5. Capability negotiation is a MediaSession input

Clients request playback capabilities and preferences, not provider URLs.

The capability contract may include versioned facts such as:

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

The Control Plane and Gateway select the profile from the intersection already required by ADR-0046:

```text
client capability
AND actor policy
AND Gateway capability
AND Agent capability
AND explicitly owned provider capability
AND route capacity
```

A client must tolerate the server selecting a supported profile different from its first preference.

The client never selects `streamdev`, `suitebridge`, a filesystem path or another private provider as part of this public capability negotiation.

### 6. Streamdev remains a private provider, not the platform foundation

Streamdev may remain a useful explicitly owned internal StreamProvider where it is deliberately configured and its capabilities match the requested route.

It is not:

- the public playback API;
- the client authorization boundary;
- the universal VDR-Suite streaming dependency;
- an implicit fallback when another provider fails;
- a reason for clients to learn Streamdev URLs or credentials.

Provider ownership and selection remain governed by the explicit local-provider ownership contract established before Phase 65. Availability alone never authorizes provider use, and an active route does not silently switch providers.

A future SuiteBridge/VDR-native media provider may be added after source review, bounded native integration, capability contracts and real VDR acceptance. Its existence does not bypass explicit provider ownership or route selection.

### 7. The VDR plugin is a media source boundary, not a media server

A future VDR-local Suite media capability may perform only the minimum VDR-native work required to establish and close a safe local media source.

It may expose bounded native facts or a bounded local stream handle to the Agent/provider boundary.

It does not own:

- public HTTP/HLS endpoints;
- TLS or end-user credentials;
- public MediaSession state;
- network backpressure from arbitrary clients;
- remuxing or transcoding while VDR locks are held;
- a global timeshift store;
- platform decoder or renderer behavior.

Slow network delivery and expensive media processing must remain outside VDR callbacks and VDR-held locks as required by ADR-0046.

### 8. Media-processing engines remain replaceable

The media adaptation layer exposes narrow pass-through, remux and transcode capabilities rather than making one processing library part of the public contract.

Phase 65 may select FFmpeg/libav-based tooling or another proven implementation behind that boundary after build, packaging, licence, hardware-acceleration, failure-isolation and operational tests.

This ADR does not require FFmpeg and does not expose FFmpeg command-line or library semantics to clients, Agents or providers.

### 9. Player state and persistent playback state are distinct

Transient player state is client-local unless a server media operation requires it.

Examples:

```text
current decoder state
render surface
local buffering
volume
local focus state
temporary UI visibility
```

Persistent user playback state is a Suite domain concern when implemented.

Examples include:

```text
recording resume position
watched/progress state
last completed position
explicit user reset of progress
```

Persistent progress must be bound to stable Suite media identity and actor scope rather than a provider URL, Recording path or player-specific identifier.

A network timeout while writing persistent progress must follow the future public API revision/idempotency rules where necessary; it must not cause playback to depend on a player-private database as the only authority.

Phase 65 may establish the media semantics; stable independent-client API exposure remains aligned with ADR-0048 and Phase 67.

### 10. Track selection is normalized above provider details

MediaSession metadata may expose normalized available audio and subtitle tracks and the selected/default tracks.

Clients select tracks by Suite media-session semantics, not by provider-specific PIDs, Streamdev parameters or player-private identifiers as a durable public contract.

When the chosen delivery profile preserves multiple tracks, final local track rendering remains a player responsibility.

When remux or transcode is required to satisfy track compatibility, that transformation is part of the selected media profile and route.

### 11. Timeshift remains an explicit later capability

This ADR keeps the ADR-0046 rule that ordinary live playback does not silently imply timeshift.

Future timeshift requires explicit:

- buffer identity;
- owner and storage placement;
- retention and quota;
- live edge and seek window;
- reconnect behavior;
- cleanup;
- route and capacity accounting;
- rights and audit semantics.

The logical client contract belongs to the Media Plane. Physical buffering may be placed in a bounded Agent/site-local media worker or another governed media component when that minimizes unnecessary WAN traffic and preserves route ownership.

The VDR plugin does not become the global timeshift coordinator.

### 12. Backpressure belongs outside VDR

The Streaming Gateway and media worker boundary absorb slow clients, reconnect behavior and protocol-level buffering.

A slow or disconnected client must not cause:

- a VDR callback to wait for client reads;
- an unbounded in-memory queue in the VDR process;
- an indefinitely retained VDR receiver;
- remux/transcode work while a VDR lock is held.

ProviderStreamLease expiry, route cleanup and bounded buffering remain mandatory.

---

## Client Playback Contract

A future shared client contract should remain small and semantic.

Illustrative shape:

```text
PlaybackSession
  mediaSessionId
  selectedProfile
  capabilities
  availableTracks
  liveOrRecording
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

The browser JavaScript wrapper, Android client library, Kodi add-on and future clients may expose different language-specific APIs while preserving the same Suite semantics.

---

## Phase 65 Implementation Direction

ADR-0046 remains the authoritative Phase 65 server sequence. This ADR adds the following constraints to that implementation:

1. prove authenticated Recording pass-through without a client-visible provider URL;
2. prove authenticated live pass-through and deterministic receiver cleanup;
3. define versioned client playback capability input and selected-profile output;
4. prove that pass-through does not invoke a decoder/re-encoder;
5. add remux only for a demonstrated client/container compatibility requirement;
6. implement at least one first-party player adapter against the MediaSession contract without provider knowledge;
7. add normalized track/seek/discontinuity behavior required by that client;
8. add persistent resume/progress only with stable Suite identity and actor ownership;
9. add transcoding only as a separately capacity-governed capability after pass-through/remux proof;
10. add timeshift only as a later explicit buffer/session slice.

No item in this sequence authorizes Phase 65 work before Phase 64 is complete.

---

## Testing and Acceptance

### Server/media tests

Tests must prove:

- a pass-through-capable client receives a pass-through profile without re-encoding;
- remux is selected only when the source codecs are acceptable but delivery packaging is not;
- transcode is not selected when a lower-transformation valid profile exists;
- an unsupported requested profile fails or selects an explicitly supported alternative without provider leakage;
- provider identity is not client-selectable;
- an active route does not silently switch to another provider;
- disconnect and grant revocation release bounded provider resources;
- slow-client behavior does not block VDR callbacks or retain unbounded queues;
- track and seek capability advertisement is truthful;
- timeshift is not advertised before its buffer contract exists.

### Client tests

Each first-party client player adapter must prove, as applicable:

- MediaSession expiry and revocation handling;
- play/pause/seek state mapping;
- audio/subtitle track mapping;
- reconnect and explicit discontinuity handling;
- decoder/player failure classification without leaking provider details;
- platform lifecycle cleanup;
- no provider URL construction in client code;
- persistent progress uses stable Suite resource identity when that feature exists.

### Real VDR acceptance

The first VDR-native media source must prove:

- expected temporary receiver/resource allocation only;
- no Timer, Recording, channel-list, setup or filesystem mutation;
- no network wait, remux or transcode under VDR locks;
- client disconnect, Gateway revocation and service shutdown release the source;
- provider URLs, credentials and local paths stay private;
- rollback leaves no stale public listener or retained receiver.

---

## Alternatives Considered

### Make Streamdev the VDR-Suite streaming architecture

Rejected. Streamdev may be an internal explicitly owned provider, but it does not own VDR-Suite authorization, MediaSession lifecycle, client compatibility or multi-site routing.

### Extract Kodi VideoPlayer and use it everywhere

Rejected. It would import Kodi-specific player, rendering, platform and maintenance assumptions into every Suite client and duplicate platform integration work.

### Build a new VDR-Suite decoder and renderer

Rejected. VDR-Suite's differentiating responsibility is VDR/media orchestration and a stable multi-client Media Plane, not reimplementing hardware-decoder, audio-output and rendering stacks for every platform.

### Transcode every stream to one universal profile

Rejected. It needlessly consumes compute, increases latency, can reduce quality and creates a larger failure/capacity surface when the client can already play the original streams.

### Mandate HLS for every client

Rejected. HLS may be an important remux/transcode profile, especially for browser or television compatibility, but capable clients may benefit from lower-overhead direct pass-through and future clients may require other profiles.

### Let each client choose the provider and transformation path

Rejected. That leaks topology, credentials and backend quirks, fragments policy and makes route/capacity decisions impossible to enforce centrally.

### Put timeshift directly into the VDR plugin

Rejected as the general architecture. Timeshift is a Media Plane resource with quota, seek-window, reconnect and cleanup semantics and must not turn VDR callbacks into network/storage coordination points.

---

## Consequences

Positive:

- VDR-Suite avoids building and maintaining a cross-platform codec/rendering engine;
- clients gain platform hardware acceleration, media controls and lifecycle integration through mature playback engines;
- Kodi knowledge is reused architecturally without importing Kodi internals;
- Streamdev stays replaceable and private;
- pass-through preserves source quality and minimizes resource use when possible;
- remux and transcode become explicit capability/capacity decisions;
- browser, TV and native clients can receive different compatible profiles through one MediaSession model;
- persistent resume/progress can remain consistent across devices;
- future timeshift remains additive without contaminating the VDR plugin boundary.

Trade-offs:

- each client family needs a playback adapter and platform-specific acceptance;
- capability negotiation must handle imperfect and device-specific decoder support;
- browser/TV compatibility may require more than one media profile;
- remux/transcode workers add operational complexity when enabled;
- cross-device resume introduces a persistent actor-scoped domain that must be versioned and reconciled correctly.

---

## Non-Goals

This ADR does not select:

- final public MediaSession endpoint paths;
- one mandatory protocol for every client;
- one fixed HLS/DASH/WebRTC implementation;
- one FFmpeg version or media-processing library;
- one desktop or Apple playback library;
- codec ladders or encoder presets;
- DRM;
- offline media download;
- final timeshift storage implementation;
- final public progress/resume endpoint schema;
- a Kodi add-on API surface.

---

## Acceptance Criteria

This decision is implemented only when:

- the Suite server delivers an authorized selected media profile without exposing private provider details;
- first-party client-common playback code does not implement a universal codec/rendering stack;
- at least one production client delegates decode/rendering to its selected platform playback engine;
- pass-through is preferred over remux and remux over transcode whenever the lower-transformation profile is valid under policy and capacity;
- transcoding is explicit, observable and capacity-governed rather than the default;
- Streamdev is never required as the public client contract or implicit provider fallback;
- provider ownership and route fencing remain explicit;
- VDR callbacks and locks do not perform network backpressure, remux or transcode work;
- track, seek and discontinuity capabilities are truthfully negotiated;
- persistent resume/progress, when implemented, is actor-scoped and bound to stable Suite media identity;
- timeshift remains unavailable until its explicit buffer/session contract is implemented and tested;
- hosted CI and required real-client/real-VDR acceptance pass for the implemented slice.

Acceptance of this ADR is not Phase 65 runtime completion.

---

## Related Decisions

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