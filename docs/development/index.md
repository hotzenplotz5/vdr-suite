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
- [Phase 62 Slice 2S — Browser-Session Lifecycle Outcome Accountability](phase-62-slice-2s-browser-session-outcome-accountability.md)
- [Phase 62 Slice 2T — Browser-Session Issuing-Credential Lifecycle Binding](phase-62-slice-2t-browser-session-issuer-binding.md)
- [Phase 62 Slice 2U — Concurrent Browser-Session Limit](phase-62-slice-2u-browser-session-concurrency-limit.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

## Current markers

```text
Latest completed numbered runtime phase: Phase 61 - Suite Metadata and Genre Platform
Completed hardening: Post-Phase 61 Performance Hardening (B1-B4)
Current runtime phase: Phase 62 - Identity, RBAC and Accountability Foundation
Current accepted Phase 62 state: repository, source CI and real yaVDR runtime accepted through Slice 2U
Accepted implementation/runtime head: 16ff04a4ba371aad32fc4a38bf82f9c0529c532d
Accepted Slice-2U source CI: #6690 / run 30723297375 / all five jobs successful
Documentation-only Slice-2U closeout: this commit; closeout CI pending
Active repository implementation: none selected after Slice 2U runtime acceptance
Installed/running daemon SHA-256: 0e3ec0d57f4471804824247f712c2457015cc22ac9576df60d8d77ed8ddb3134
Installed loader SHA-256: 3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Phase 62 remains active and incomplete. Phase 63-67 runtime has not been
advanced. PR #117 remains open, Draft and unmerged.

## Accepted Slice 2U contract and evidence

```text
Configuration: VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR
Compatibility default: 0 (unlimited)
Configured range: 0..64
Count semantics: effective actor/device/session/browser credential/issuer lifecycle
Transaction: count before insert inside serialized BEGIN IMMEDIATE
Reached limit: HTTP 409 browser_session_limit_reached
Invalid configuration: HTTP 503 browser_session_limit_configuration_invalid
Eviction: never
Schema migration: none
Frontend or route change: none
```

The real yaVDR acceptance proved the first session succeeded, a second
same-actor session was denied without writes, the first session remained usable,
logout released the slot, a replacement session succeeded and revoked-cookie
replay was denied.

```text
Implementation/runtime head:
16ff04a4ba371aad32fc4a38bf82f9c0529c532d

Source CI:
#6690 / run 30723297375 / all five jobs successful

Installed/running daemon SHA-256:
0e3ec0d57f4471804824247f712c2457015cc22ac9576df60d8d77ed8ddb3134

Loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Final service PID:
79316

Runtime report SHA-256:
7c33b06d07beff6b17bad82153ff4a0f1e7c1f5c8d8f972406f8b0b9160f4c89

Configured limit:
1

First session issue:
HTTP 200

Second same-actor issue:
HTTP 409 browser_session_limit_reached
No second lifecycle row created

Existing first session ordinary GET:
HTTP 200

First logout:
HTTP 204
Slot released

Replacement session issue:
HTTP 200

Replacement logout:
HTTP 204

Revoked replacement-cookie replay:
HTTP 401 credential_revoked

Successful issue outcomes:
2

Limit-reached failed outcomes:
1

Successful revoke outcomes:
2

Acceptance lifecycle and source identity:
revoked

Original daemon configuration:
restored

SQLite quick check:
ok

SQLite foreign-key check:
empty

VDR domain mutations:
0

Service state:
active

Automatic rollback:
not required
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2u-20260802T041910Z-16ff04a4ba37/runtime-acceptance-slice2u
```

## Latest runtime-accepted slices and closeouts

- [Phase 62 Slice 2U Runtime-Accepted Closeout Candidate](phase-62-slice-2u-browser-session-concurrency-limit.md)
- [Phase 62 Slice 2T Closeout](phase-62-slice-2t-browser-session-issuer-binding.md)
- [Phase 62 Slice 2S Closeout](phase-62-slice-2s-browser-session-outcome-accountability.md)
- [Phase 62 Slice 2R Closeout](phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Phase 62 Slice 2Q Closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Phase 62 Slice 2P Closeout](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Phase 62 Slice 2O Closeout](phase-62-slice-2o-native-fuzzy-refresh-security-migration.md)
- [Phase 61 Metadata, Genre and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
- [Architecture Source Audit — 2026-07-15](architecture-source-audit-2026-07-15.md)

## Latest accepted Slice 2S evidence

```text
Service PID after install/acceptance: 69610 / 69610
HTTP requests: 5
Login accountability events: 2
Missing-CSRF accountability events: 1
Logout accountability events: 2
Lifecycle accountability events: 5
Operation-succeeded events: 2
Missing-CSRF operation events: 0
Session and credential revocation: passed
Revoked-cookie replay: denied
Accountability: secret-free
Database integrity: yes
Service active: yes
Rollback: not required
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2s-20260801T210333Z-c128867bfbf4/runtime-acceptance-slice2s
```

## Accepted Slice 2T runtime evidence

```text
Runtime head: 55876356e84b3e47e52911529b3f9bfa0e17f191
Source CI: #6666 / run 30719552024 / all five jobs successful
Closeout head: e79e0eb67da75044c4a9afa162c9dab188b026fd
Closeout CI: #6667 / run 30721936576 / all five jobs successful
Installed/running daemon: 34b80de4fd8f55b763c4483f0dcb50ee09e5cdc49de7f6e7c25e01ba50d84269
Runtime report SHA-256: 2ca7fcaefe21c1198e5d8ff88b3e17237b2e72a545780cc14f0200e7dd0ca983
Ordinary GET before issuer invalidation: HTTP 200
Ordinary GET after issuer invalidation: HTTP 401 credential_revoked
Logout after issuer invalidation: HTTP 401 credential_revoked
Logout denied before CSRF: yes
Raw browser lifecycle unchanged before cleanup: yes
Original issuer unchanged: yes
Test browser lifecycle revoked: yes
Revoked-cookie replay denied: yes
VDR domain mutations: 0
Database integrity: yes
Service active: yes
Rollback: not required
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2t-20260801T223353Z-55876356e84b/runtime-acceptance-slice2t
```

No schema migration, cascade, cleanup, route, frontend, permission,
configuration or packaging change was included in Slice 2T.

## Developer references

- [Developer Onboarding](developer-onboarding.md)
- [Architecture Map](architecture-map.md)
- [Build System State](build-system-state.md)
- [GitHub Actions Status Handoff](github-actions-status-handoff.md)
- [Person API](person-api.md)
- [Web Client API Contract Snapshot](web-client-api-contract-snapshot.md)
- [Recording Person Cast Completeness Fix](recording-person-cast-completeness-fix.md)

## Runtime and acceptance references

- [Agent Workflow Rules](../../AGENTS.md)
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

Require all five GitHub Actions jobs for this documentation-only Slice-2U
closeout.

No next implementation slice is selected by this closeout. Perform a fresh
post-2U gap analysis only after full closeout CI.

## Related navigation

- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)
