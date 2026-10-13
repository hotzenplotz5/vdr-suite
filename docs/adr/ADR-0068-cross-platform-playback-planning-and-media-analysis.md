# ADR-0068: Cross-Platform Playback Planning and Recording Media Analysis

## Status

**Proposed — architecture decision for review, not runtime acceptance.**

Date: 2026-10-13

## Context

Phase 65 established the MediaSession/Streaming Gateway and accepted ADR-0046, ADR-0053, ADR-0055 and ADR-0056. The VIDAA HEVC experiments reveal a gap between nominal codec capability and reliably playable output. Two HEVC recordings can need different HLS segmentation: Medicus II has approximately 1 s video keyframe spacing, while Solo UHD Main10 has approximately 10.4 s spacing and was playable with the legacy segmenter. Switching all HEVC streams to keyframe-aligned fragments is therefore not safe.

The desired user journey is one **Play** action on VIDAA, web, future Tizen/webOS/Android TV/Kodi clients; the user never chooses FFmpeg flags, HEVC profiles or HLS segment boundaries.

This ADR **extends and constrains** accepted ADR-0053, not a second media-server architecture. Existing Phase 65 acceptance is not rescinded.

## Decision

Separate three responsibilities in the existing authorized Media Plane:

1. **Recording Media Analyzer (backend)** records validated stream metadata and bounded media structure evidence: codec/profile/level, pixel format and HDR, resolution/frame rate, audio and subtitle tracks, keyframe/GOP timing and random-access suitability. Its outputs are explicitly versioned and keyed to the established source fingerprint/extent and backend scope. A changed or growing source invalidates completed-recording evidence. No unchecked packet flag is deemed independently validated HEVC IDR/CRA.
2. **Playback Planner (backend)** combines the exact source descriptor/evidence, client capability report, delivery constraints, server-supported transform/encoder capacity, authorization, and failure history to select an explicit `MediaPresentationProfile` / transport adaptation. Rank viable paths **direct play → remux/video copy with suitable segmentation/audio conversion → video transcode**, subject to measured compatibility, policy and availability. These are preferences, not promises; unsupported plans fail closed with actionable reasons. Plan decisions and their reason codes are observable without leaking provider internals or source secrets.
3. **Client Playback Adapter (platform-specific)** uses that authorized playback contract with the appropriate playback engine, reports verified capabilities and playback health and handles focus, buffering, seeking, track changes and termination. VIDAA MSE, Tizen, webOS, Android TV, browser and Kodi adapters differ; clients do not choose arbitrary backend paths or execute server-side FFmpeg policy. A generic claim of 'HEVC supported' alone is insufficient to imply HLS/MSE compatibility.

### Binding behavior

- One Play action requests an authorized MediaSession; the backend selects media presentation transparently and returns an enforceable playback contract.
- Media analyzer evidence is reused across clients, **not** recomputed on each Play when unchanged and already verified. Bounded analysis outside playback-critical paths is preferred; synchronous probes need measured latency/resource budgets and explicit fallback.
- Cache keys bind recording/backend identity, source fingerprint, probe/schema versions and relevant profile context; never authorize media access through cache residency. No arbitrary URLs, no cross-user grant reuse.
- The planner distinguishes video codec support from actual packaging, transport, audio, random-access and buffering constraints. Per-recording GOP decisions must not create global HEVC toggles.
- TV capability reporting is versioned and conservative; unknown data retains an independently supported legacy path or explicit error, not speculative transcoding.
- Progress, seek, audio switching and recovery are first-class playback contracts, not optional afterthoughts. A failure can trigger a bounded, authorized alternate presentation with the same session/ownership security invariants, without uncontrolled restart loops.
- VDR native recording identity, Agent/provider authority, fencing, grants and existing platform authorization remain binding. No extra direct provider URL or secondary auth system.
- Dynamic HDR/DRM codec-specific licensing and platform decoder restrictions are not assumed solved by this ADR.

## Migration and delivery boundary

The current HEVC trial branch contains bounded ffprobe/GOP experiments and an operator-scoped opt-in. They are **experimental evidence**, not acceptance of an automatic universal planner or a proven Medicus II playback fix. Solo must remain unmodified by this trial; a real VIDAA test of Medicus II and non-regression is required before enabling any new policy in production.

Incremental implementation reuses the existing `MediaSourceDescriptor`, `MediaPresentationSelector`, `MediaTranscodePolicy`, MediaSession and normalized playback contract wherever adequate; first audit actual accepted code before adding types or persistence. Avoid a second profile planner, speculative transcoder rewrite or needless full builds.

## Acceptance gates

1. **Contract and gap audit:** Map real current source metadata, presentation logic, client capabilities and cache lifetime to these boundaries; document only proven gaps.
2. **Source-scoped analyzer:** Attest read-only bounded probes, actual HEVC IDR/CRA random access, source fingerprint before/after and cache invalidation on mutation, growth or probe-version change.
3. **Planner:** Deterministic compatibility selection across codec, container, audio, GOP, transport and server resources, with fail-closed fallback; unit + integration coverage including Medicus short GOP and Solo long GOP.
4. **Client contracts:** VIDAA playback start, buffer progress, seek and audio track reporting; record separate portability tests for at least one other playback-engine family before calling selection 'cross-platform accepted'.
5. **Real playback:** Medicus II smooth on physical VIDAA with video copy and no Solo regression; verify representative other device/software combinations later. Distinguish source integration, CI green and *actual* TV acceptance.
6. **Operational safety:** bounded latency/CPU/memory during concurrent viewing; cached results; no production binary overwrite; reversible rollout; source and security boundaries unchanged.

## Consequences

The server learns media properties once and makes playback decisions for multiple device families, while maintaining device-native playback engines. This simplifies future TV app development but does not eliminate required platform-specific integration and testing.

## Related architecture and planning

- [ADR-0046](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0055](ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [ADR-0056](ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [Cross-Platform Playback Productization Workstream](../development/post-phase69-playback-platform-workstream.md)
- [Strict Roadmap](../planning/roadmap.md)
