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
- [Frontend Playback Integration Contract](frontend-playback-integration-contract.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 66 Media Home and Browse Experience](phase-66-media-home-browse-experience.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md)
- [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [ADR-0058 Media Home Architecture](../adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR-0054 Broadcast Companion Services](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [Phase 65 Closeout](phase-65-closeout.md)

## Completed and historical evidence

- [Completed Phases](completed-phases.md)
- [Latest Completed Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md)
- [Post-Phase-66 Home Performance Hardening](post-phase-66-home-performance-hardening.md)
- [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 64 Final Closeout](phase-64-closeout.md)
- [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](phase-65-recording-startup-progressive-direct.md)
- [Phase 65.C Media Transcode Performance / Output Policy](phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 65.D Browser-local Volume/Mute Closeout](phase-65d-browser-volume-mute-closeout.md)
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

Phase 64 is complete. Use [Phase 64 Final Closeout](phase-64-closeout.md) for accepted evidence and [Current State](../CURRENT.md) for current operational status.

## Current media/home position

Phase 65 - Streaming Gateway and Media Sessions is completed. Use [Phase 65 Closeout](phase-65-closeout.md) and Phase-65.D contracts for durable evidence/history. The later accepted Live-TV `ended` stabilization is documented as post-closeout evidence in the Phase-65.D.1 closeout and does not reopen Phase 65.

Phase 66 - Media Home and Browse Experience is completed. The later Home performance, Recording Discovery, Series metadata/artwork and final H0-H5 Home rebuild are completed as non-numbered hardening and do not reopen Phase 66. Use [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md) for the consolidated current Home truth.

The current accepted Recordings 2 detail/presentation layer, including the cinematic Hero, scoped canonical-owner playback prewarm and corrected related-Genre portrait posters, is documented separately in [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md). Native marks/cutting remains separately documented in [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md).

Phase 67 - Broadcast Companion Services: Teletext and HbbTV is next but has not started. ADR-0054 defines its accepted architecture and a separate explicit runtime kickoff is required.

Growing-Recording seek and Live-TV timeshift remain truthful deferred capability work and do not reopen Phase 65.

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
- active bounded implementation contracts -> `docs/development/`
- completed exact evidence -> historical development closeouts
- completed Phase-62 security slice navigation -> `phase-62-security-contract-index.md`
- non-numbered Home/Recording/Live hardening -> dedicated post-phase or post-closeout evidence without inventing a numbered phase

## Back

- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
