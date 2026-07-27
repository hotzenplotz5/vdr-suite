# Development Documentation

## Current implementation truth

- [Current Project Status](current-status.md)
- [Current Architecture State](current-architecture-state.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

## Latest closeouts

- [Phase 61 Metadata, Genre and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
- [Architecture Source Audit — 2026-07-15](architecture-source-audit-2026-07-15.md)

Current markers:

```text
Latest completed numbered runtime phase: Phase 61 - Suite Metadata and Genre Platform
Completed hardening: Post-Phase 61 Performance Hardening (B1-B4)
Completed platform features: Remote/Live Overlay (#110), Global Search (#111)
Next strict runtime phase: Phase 62 - Identity, RBAC and Accountability Foundation
```

## Developer references

- [Developer Onboarding](developer-onboarding.md)
- [Architecture Map](architecture-map.md)
- [Build System State](build-system-state.md)
- [GitHub Actions Status Handoff](github-actions-status-handoff.md)
- [Person API](person-api.md)
- [Web Client API Contract Snapshot](web-client-api-contract-snapshot.md)
- [Recording Person Cast Completeness Fix](recording-person-cast-completeness-fix.md)

## Runtime and acceptance references

- [Real VDR Acceptance Plan](real-vdr-acceptance-plan.md)
- [Recording Action Readiness Audit](recording-action-readiness-audit.md)
- [Live Acceptance Evidence](live-acceptance-evidence.md)
- [Phase and documentation guardrails](../../tools/)

## Documentation placement rules

- Current verified state belongs in `docs/CURRENT.md` and current-status documents.
- Completed implementation belongs in this section and the completed-phase archive.
- Stable architecture belongs in `docs/architecture/`.
- Future dependency order and open gaps belong in `docs/planning/`.
- Accepted ADRs remain in `docs/adr/` and do not move into completed phases.
- Historical phase files remain traceability records and must not be linked as equal current entry points.

## Related navigation

- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)