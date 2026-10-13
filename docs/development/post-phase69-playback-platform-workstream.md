# PP — Cross-Platform Playback Productization

Status: **Planned cross-cutting intermediate workstream**. This is not a completed numbered phase and does not renumber Phase 70.

Architecture: [ADR-0068](../adr/ADR-0068-cross-platform-playback-planning-and-media-analysis.md), extending accepted ADR-0053.

## Scope

Backend analyzer, shared playback planner, platform-specific playback adapters. One Play action without manual codec/segment controls. Reuse Phase-65 MediaSession, MediaPresentationSelector, TranscodePolicy, Gateway, device capability contracts and source fingerprints. Do not build duplicate media infrastructures.

## Proposed slices and gates

- **PP.0 — Baseline audit:** compare existing accepted media implementations and platform capabilities; prove gaps before changes.
- **PP.1 — Source-bound media analyzer:** codec/audio/HEVC GOP and independently validated random-access semantics; bounded read-only probing.
- **PP.2 — Media evidence cache:** fingerprint- and probe-version-bound results, explicit invalidation for changed/growing recordings; no unbounded scan on playback start.
- **PP.3 — Playback planner:** deterministic supported direct/remux/transcode choices including container, transport, audio and GOP; reason codes and safe fallback; Medicus and Solo regression examples.
- **PP.4 — Client capability contract:** conservative versioned VIDAA, Tizen, webOS, Android TV, Kodi and web adapters with platform-specific playback engines.
- **PP.5 — Playback lifecycle:** startup, health/buffering, seek, audio switching and bounded failure recovery without losing grant/provider ownership.
- **PP.6 — Real-device acceptance:** Medicus II smooth on physical VIDAA, Solo non-regression, then separate acceptance for another TV software family.
- **PP.7 — Rollout/docs:** tested support matrix, diagnostic evidence, safe deployment and rollback guidance.

## Implementation guardrails

Current Medicus II pilot is separate and may finish before PP implementation starts. No new global HEVC toggle. Existing auth and VDR owner/fencing remain authoritative. Do not claim source merge/CI as real-TV acceptance. No duplicated full daemon builds or uncontrolled leftover candidate directories. PP implementation begins only after ADR review and kickoff.

References: [ADR-0068](../adr/ADR-0068-cross-platform-playback-planning-and-media-analysis.md), [Roadmap](../planning/roadmap.md), [Phase Map](../planning/phase-map.md), [HEVC Pilot](hevc-hls-keyframe-segmentation-trial-2026-10-13.md).

## Framerate / Timing diagnostic gate

During PP.1 measure source r_frame_rate and avg_frame_rate separately from actual packet PTS/duration cadence, DTS/order and discontinuities (constant/variable frame rate). During PP.3 and PP.5 inspect output fMP4 sample timing, HLS segment boundaries, playback clock, buffering and presented/dropped-frame telemetry from the real platform. A successful HEVC GOP analyzer does **not** prove smooth 24-fps presentation. Medicus II metadata approximately 24 fps and anecdotal 14-fps perceived display remain separate facts. No synthetic FPS filter or framerate rewrite until an observed timing fault is demonstrated. Solo non-regression remains binding.
