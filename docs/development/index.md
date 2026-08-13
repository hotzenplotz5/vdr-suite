# Development Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Purpose

This is a stable navigation page for development contracts and evidence. It does not duplicate active PRs, exact branch heads, CI checkpoints or the current phase snapshot. Those volatile facts belong only in [Current State](../CURRENT.md).

## Current orientation

- [Current State](../CURRENT.md)
- [Current Project Status](current-status.md)
- [Current Architecture State](current-architecture-state.md)
- [Architecture Map](architecture-map.md)
- [Strict Roadmap](../planning/roadmap.md)

## Completed and historical evidence

- [Completed Phases](completed-phases.md)
- [Completed Phase Archive](completed-phases/README.md)
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

## Active-domain development material

Current Timer-orchestration documents cover `TimerIntent`, `TimerAssignment`, `NativeTimerBinding`, scheduling, native observation/readback and protected operations under ADR-0044.

The exact currently authorized implementation checkpoint is deliberately not repeated here; see [Current State](../CURRENT.md).

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
