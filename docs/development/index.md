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
- [Phase 62 Slice 2R — Configurable Absolute Browser-Session Lifetime](phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

## Current markers

```text
Latest completed numbered runtime phase: Phase 61 - Suite Metadata and Genre Platform
Completed hardening: Post-Phase 61 Performance Hardening (B1-B4)
Current runtime phase: Phase 62 - Identity, RBAC and Accountability Foundation
Current accepted Phase 62 state: repository, CI and real yaVDR runtime accepted through Slice 2R
Accepted code/runtime head: d65af5a24688fe4dbf090030226fd45825260060
Accepted source/runtime CI: #6661 / run 30715365583 / all five jobs successful
Active repository implementation: none selected after Slice 2R closeout
Installed daemon SHA-256: 12953babb3a2ce3aebeb99a377f66a94375bf55cf1e839cf8163bf574f4d7660
Installed loader SHA-256: 3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Phase 62 remains active and incomplete. Phase 63-67 runtime has not been
advanced. PR #117 remains open, Draft and unmerged.

## Latest accepted closeouts

- [Phase 62 Slice 2R Closeout](phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Phase 62 Slice 2Q Closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Phase 62 Slice 2P Closeout](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Phase 62 Slice 2O Closeout](phase-62-slice-2o-native-fuzzy-refresh-security-migration.md)
- [Phase 61 Metadata, Genre and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
- [Architecture Source Audit — 2026-07-15](architecture-source-audit-2026-07-15.md)

## Latest accepted Slice 2R evidence

```text
Custom lifetime: 900 seconds
HTTP requests: 5
Persisted remaining lifetime: 900 seconds
Cookie Max-Age: 900
Cookie attributes: HttpOnly, Secure, SameSite=Strict
Ordinary browser GET: passed
Missing-CSRF logout: denied
Logout and lifecycle revocation: passed
Revoked-cookie replay: denied
Lifecycle accountability: complete and secret-free
Original runtime configuration and environment: restored
Database integrity: yes
Service active: yes
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2r-20260801T202314Z-d65af5a24688/runtime-acceptance-slice2r
```

## Accepted Slice 2R contract

```text
VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS
Default: 28800
Allowed: strict decimal 300..86400
```

The same server-owned value controls persisted absolute expiry and cookie
`Max-Age`. Invalid configuration blocks browser-session issuance with HTTP 503
and no `Set-Cookie`. Idle timeout, refresh, cleanup and concurrency remain
explicitly deferred.

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

Let the Slice-2R documentation closeout complete all five CI jobs. Then select
exactly one next bounded Phase-62 slice only after a fresh gap and runtime-safety
review.

## Related navigation

- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)
