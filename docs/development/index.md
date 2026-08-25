# Development Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Purpose

This is a stable navigation page for development contracts and evidence. It does not duplicate active PR tips or transient CI checkpoints. Those volatile facts belong only in [Current State](../CURRENT.md).

## Current orientation

- [Current State](../CURRENT.md)
- [Current Project Status](current-status.md)
- [Current Architecture State](current-architecture-state.md)
- [Architecture Map](architecture-map.md)
- [Strict Roadmap](../planning/roadmap.md)

## Completed and historical evidence

- [Completed Phases](completed-phases.md)
- [Completed Phase Archive](completed-phases/README.md)
- [Phase 64 Final Closeout](phase-64-closeout.md)
- [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](phase-65-recording-startup-progressive-direct.md)
- [Phase 65.C Media Transcode Performance / Output Policy](phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 62 Security Contract Index](phase-62-security-contract-index.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Phase 63 Backend Agent Foundation](phase-63-backend-agent-foundation.md)
- [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md)
- [Phase 63 Durable Command Delivery](phase-63-command-delivery.md)
- [Phase 63 Fenced Native Operation](phase-63-fenced-native-operation.md)
- [Phase 63 Local Provider Ownership](phase-63-local-provider-ownership.md)
- [Phase 63 Local Provider Selection Runtime](phase-63-local-provider-selection-runtime.md)
- [Phase 63 Protected Write Contract](phase-63-protected-write-contract.md)

Historical exact heads, CI runs, hashes and runtime evidence stay in their closeouts. Historical slice documents remain traceability records and do not authorize successor implementation.

Historical static-guard anchors: `phase-62-slice-2i-recording-execution-security-migration.md`, `phase-62-slice-2j-searchtimer-create-security-migration.md`, `phase-62-slice-2k-runtime-acceptance-harness.md`, `phase-62-slice-2l-searchtimer-maintenance-security-migration.md`, `phase-62-slice-2m-safe-post-classification.md`, `phase-62-slice-2n-searchtimer-execution-security-migration.md`, `phase-62-slice-2o-native-fuzzy-refresh-security-migration.md`, `phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md`, `phase-62-slice-2r-browser-session-lifetime-configuration.md`, `phase-62-slice-2s-browser-session-outcome-accountability.md`, `phase-62-slice-2t-browser-session-issuer-binding.md`.

The complete historical Phase-62 navigation is maintained in `phase-62-security-contract-index.md`; the raw anchors above exist only for compatibility with older static guards that still inspect this index directly.

## Timer-orchestration material

Phase-64 Timer-orchestration documents cover `TimerIntent`, `TimerAssignment`, `NativeTimerBinding`, scheduling, native observation/readback, protected operations and controlled reassignment/failover under ADR-0044.

Phase 64 is complete. Use [Phase 64 Final Closeout](phase-64-closeout.md) for accepted evidence and [Current State](../CURRENT.md) for the current repository checkpoint.

## Current media-domain work

Phase 65 - Streaming Gateway and Media Sessions is active.

Accepted bounded verticals/slices are:

- 65.A Existing-Recording playback;
- 65.B Live-TV playback;
- 65.C Recording delivery performance and media output/transcode settings, implemented through the completed-Recording progressive path and the subsequently continued backend output-policy/Web-settings work;
- 65.D.1 Persistent Browser Playback Shell;
- 65.D.2 Recording Playback Controls and Seek.

Phase 65.D Client playback abstraction is active. The old 65.C seek/growing-recording planning label is superseded; truthful capability reporting remains mandatory. Completed-Recording arbitrary time-seek and stop/resume are accepted for the supported D.2 progressive-fMP4 and HLS restart-seek profiles. Remaining Phase-65.D work includes normalized audio/subtitle selection, discontinuity handling and classified playback failures, while growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond the accepted D.2 paths remain explicitly unsupported/deferred. Phase 66 remains blocked until Phase 65 closes and receives its own explicit kickoff.

A reproducible HEVC recording-duration anomaly discovered during this work is recorded separately as a deferred investigation: [HEVC Recording Frame-Rate / VDR Length Investigation](hevc-recording-framerate-investigation.md). It is evidence only, not an accepted architecture decision, and does not block the current subtitle work.

Use ADR-0046, ADR-0053, ADR-0055, the Strict Roadmap, Current State and Golden User Journeys before defining or authorizing further runtime work.

## Developer references

- [Developer Onboarding](developer-onboarding.md)
- [Build System State](build-system-state.md)
- [GitHub Actions Status Handoff](github-actions-status-handoff.md)
- [Web Client API Contract Snapshot](web-client-api-contract-snapshot.md)
- [Person API](person-api.md)

## Placement rules

- volatile operational truth -> `docs/CURRENT.md`
- stable architecture -> `docs/architecture/` and accepted ADRs
- future order and gates -> `docs/planning/`
- completed exact evidence -> historical development closeouts
- completed Phase-62 security slice navigation -> `phase-62-security-contract-index.md`

## Back

- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
