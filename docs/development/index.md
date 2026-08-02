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
- [Phase 62 Slice 2V — Browser-Session Idle Expiry and Throttled last_seen](phase-62-slice-2v-browser-session-idle-expiry.md)
- [Phase 62 Slice 2W — Browser-Session Terminal Retention Cleanup](phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

## Current markers

```text
Latest completed numbered runtime phase: Phase 61 - Suite Metadata and Genre Platform
Completed hardening: Post-Phase 61 Performance Hardening (B1-B4)
Next strict runtime phase: Phase 62 - Identity, RBAC and Accountability Foundation
Current runtime phase: Phase 62 - Identity, RBAC and Accountability Foundation
Current accepted Phase 62 state: repository, source CI and real yaVDR runtime accepted through Slice 2V
Accepted implementation/runtime head: e84415fadb2587ff744ff8927f1f0113920ece2f
Accepted Slice-2V source CI: #6779 / run 30741293079 / all five jobs successful
Slice-2V closeout head: cf31b2b67f73f12718601ced5468a59a1183adcb
Slice-2V closeout CI: #6799 / run 30742295881 / all five jobs successful
Slice-2V closeout link: https://github.com/hotzenplotz5/vdr-suite/actions/runs/30742295881
Active repository implementation: Slice 2W selected; implementation not started
Installed/running daemon SHA-256: e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60
Installed loader SHA-256: 3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Phase 62 remains active and incomplete. Phase 63-67 runtime has not been
advanced. PR #117 remains open, Draft and unmerged.

## Fully accepted Slice 2V

Slice 2V added:

```text
VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS
0          disabled compatibility default
300..86400 enabled timeout

security_browser_session_credentials.last_seen_at
60-second minimum activity-write interval
```

The guarded real-yaVDR acceptance proved ordinary access before idle expiry,
throttled activity persistence, ordinary and mutation denial with HTTP 401
`session_expired` after expiry, unchanged absolute expiry, replacement logout,
revoked-cookie replay denial, secret-free accountability, lifecycle cleanup,
SQLite integrity, restored configuration and zero VDR domain mutations.

```text
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS
Implementation/runtime head: e84415fadb2587ff744ff8927f1f0113920ece2f
Source CI: #6779 / run 30741293079 / all five jobs successful
Closeout head: cf31b2b67f73f12718601ced5468a59a1183adcb
Closeout CI: #6799 / run 30742295881 / all five jobs successful
Closeout link: https://github.com/hotzenplotz5/vdr-suite/actions/runs/30742295881
Runtime report SHA-256: 0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86
Evidence: /var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25
```

## Selected Slice 2W

Exactly one next slice is selected:

```text
Phase 62 Slice 2W
Browser-Session Terminal Retention Cleanup
```

Selection scope:

- strict optional retention configuration;
- one bounded startup cleanup pass;
- terminal browser verifier plus its own canonical browser session and
  `browser-session` credential only;
- fixed batch bound of 256;
- atomic recheck, secret-free accountability and deletion;
- fail-closed Security Runtime initialization on enabled-policy failure;
- preservation of actor, device, issuer, grants and accountability history.

Implementation has not started. No periodic scheduler, HTTP/API administration,
issuer-cascade cleanup, automatic eviction, frontend, Android or Phase 63-67
work is included.

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

Implement only the selected Slice-2W configuration, repository/service cleanup
transaction, startup integration, focused tests, architecture guard and Make-test
registration.

Do not combine Slice 2W with a periodic scheduler, session administration,
issuer-cascade cleanup, automatic eviction, generic security administration,
Outbox, Android or Phase 63-67 runtime.

## Related navigation

- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)
