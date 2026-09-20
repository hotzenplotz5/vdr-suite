# ADR-0062: First-Party Living-Room Output Client

## Navigation

- [ADR Index](index.md)
- [Platform Productization Roadmap](../planning/platform-productization-roadmap.md)
- [ADR-0046 Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053 Client Playback Engine and Media Adaptation Strategy](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [ADR-0061 Client Identity and Permission Profiles](ADR-0061-client-identity-permission-profiles.md)

---

## Status

Accepted architecture / implementation pending.

Date: 2026-09-20

---

## Context

The browser has proved VDR-Suite's first-party product and media semantics, but the long-term living-room product also needs a native television/output path with robust remote input, hardware decode/rendering and VDR integration.

A naive in-process VDR output plugin could accidentally duplicate MediaSession ownership, talk directly to private providers, or perform blocking network/media work inside VDR.

---

## Decision

VDR-Suite will build a **first-party living-room client** that consumes Suite domain and playback semantics.

A VDR output plugin is a supported integration/hosting route, but it remains subordinate to the same Suite client contract.

```text
VDR-Suite domain APIs / stable client contract
  -> living-room application state
  -> canonical Suite MediaSession + MediaPlaybackContract
  -> platform-appropriate playback engine
  -> hardware decode/render/audio
  -> television UI / remote input
```

### Ownership

The client does not create a second server/media authority, read/write Suite SQLite directly, construct private RESTfulAPI/SVDRP/SuiteBridge/provider calls for ordinary behavior, select providers outside Suite policy or fork Recording/EPG/Timer identity.

### VDR output-plugin boundary

If implemented as or with a VDR output plugin, the in-process plugin owns only the minimum VDR/output integration needed for presentation and input.

It must not perform unbounded network work under VDR locks/callbacks, become the Streaming Gateway, become an alternate authorization service, persist independent Suite business state or bypass MediaSession/MediaAccessGrant semantics.

A companion process is allowed when process isolation is the safer owner of network, player and render lifecycle.

### Playback engine and hardware

The client reuses a mature Linux/platform playback stack rather than creating a new Suite codec engine.

The first Linux/yaVDR implementation must evaluate and document VAAPI on Intel Gemini Lake/UHD 605, DRM/KMS vs Wayland/X11, audio/A-V sync, deinterlacing/refresh, subtitle/Teletext/HbbTV composition, suspend/resume/display hotplug and remote input.

Legacy Nvidia GT 210/VDPAU is not the primary design target; it is later compatibility scope.

### Television domains

Teletext and HbbTV remain their existing Suite domains. Phase-68 Legacy OSD remains a separate compatibility domain. The client composes them rather than collapsing them into opaque OSD pixels.

### Stable-contract gate

Prototype work may use controlled first-party internal contracts, but a supported independently packaged living-room client rollout depends on Phase 69 Public API/Client Compatibility Hardening.

---

## Acceptance

The first supported living-room release requires real hardware proof for boot/start, Live TV picture/sound, channel replacement cleanup, Recording play/pause/seek/resume, track selection, Home/EPG/search, Teletext, HbbTV, remote input, silent prewarm, VAAPI on the reference Intel system where supported, classified network/server recovery, provider privacy and exactly one MediaSession owner.

---

## Consequences

The project gains a native living-room path without making VDR itself the network/media orchestration layer. Output-plugin/render technology remains replaceable behind stable Suite semantics.
