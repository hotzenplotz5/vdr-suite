# Development Documentation

## Current implementation truth

- [Current Project Status](current-status.md)
- [Current Architecture State](current-architecture-state.md)
- [Phase 62 Security Identity Foundation — Slice 1](phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Persistent Identity Lifecycle — Slice 2](phase-62-security-identity-foundation-slice-2.md)
- [Phase 62 Slice 2H — Channel Move Security Migration](phase-62-slice-2h-channel-move-security-migration.md)
- [Phase 62 Slice 2I — Recording Execution Security Migration](phase-62-slice-2i-recording-execution-security-migration.md)
- [Phase 62 Slice 2J — SearchTimer Create Security Migration](phase-62-slice-2j-searchtimer-create-security-migration.md)
- [Phase 62 Slice 2K — Runtime Acceptance Harness](phase-62-slice-2k-runtime-acceptance-harness.md)
- [Phase 62 Slice 2L — SearchTimer Maintenance Security Migration](phase-62-slice-2l-searchtimer-maintenance-security-migration.md)
- [Phase 62 Slice 2M — Explicit Safe POST Classification](phase-62-slice-2m-safe-post-classification.md)
- [Phase 62 Slice 2N — SearchTimer Execution Security Migration](phase-62-slice-2n-searchtimer-execution-security-migration.md)
- [Phase 62 Slice 2O — Native Fuzzy Operator Refresh Security Migration](phase-62-slice-2o-native-fuzzy-refresh-security-migration.md)
- [Phase 62 Slice 2P — Query-Scoped Cache Refresh Security Migration](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Phase 62 Slice 2Q — Global Native Fuzzy Stale-Probe Deletion Security Migration](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

## Current markers

```text
Latest completed numbered runtime phase: Phase 61 - Suite Metadata and Genre Platform
Completed hardening: Post-Phase 61 Performance Hardening (B1-B4)
Current runtime phase: Phase 62 - Identity, RBAC and Accountability Foundation
Current accepted Phase 62 state: repository, CI and real yaVDR runtime accepted through Slice 2Q
Accepted code/runtime head: 88ec36076d7e5114df0a3a186cc6fbd52bb2baac
CI: #6655 / run 30713953331 / all five jobs successful
Installed daemon SHA-256: 9f60daaf7d772abe7c6ad55388cb9bb7e8afe8f6679fbf749aa9103143a41d07
Installed loader SHA-256: 3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Phase 62 remains active and incomplete. Phase 63-67 runtime has not been
advanced. PR #117 remains open, Draft and unmerged.

## Latest closeouts

- [Phase 62 Slice 2Q Closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Phase 62 Slice 2P Closeout](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Phase 62 Slice 2O Closeout](phase-62-slice-2o-native-fuzzy-refresh-security-migration.md)
- [Phase 61 Metadata, Genre and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
- [Architecture Source Audit — 2026-07-15](architecture-source-audit-2026-07-15.md)

## Latest Slice 2Q evidence

```text
Tests: 32 passed, 0 failed
HTTP requests: 25
Authorization scope: *
Snapshot source: direct SQLite
Freshness boundary: 604800 seconds
Real stale-probe deletions: 0
Delete guard removed: yes
Snapshot unchanged: yes
Database integrity: yes
Service active: yes
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T191156Z-88ec36076d7e/runtime-acceptance-slice2q
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

- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Recording Action Readiness Audit](recording-action-readiness-audit.md)
- [Phase 55 completed acceptance history](completed-phases/phase-55.md)
- [Phase and documentation guardrails](../../tools/)

## Documentation placement rules

- Current verified state belongs in `docs/CURRENT.md` and current-status documents.
- Completed implementation belongs in this section and the completed-phase archive.
- Stable architecture belongs in `docs/architecture/`.
- Future dependency order and open gaps belong in `docs/planning/`.
- Accepted ADRs remain in `docs/adr/` and do not move into completed phases.
- Phase 62 slices are not separate numbered phases.
- Historical phase files remain traceability records and must not be linked as equal current entry points.

## Exact next action

Let the Slice-2Q documentation closeout complete its five-job CI, then perform a
fresh bounded POST inventory audit before selecting one next Phase 62 slice.

## Related navigation

- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)
