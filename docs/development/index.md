# Development Documentation

## Current implementation truth

- [Current Project Status](current-status.md)
- [Post-Slice-2W New Chat Prompt](phase-62-post-slice-2w-new-chat-prompt.md)
- [Slice 2W Runtime Closeout](phase-62-slice-2w-runtime-closeout.md)
- [Slice 2W Accepted Contract](phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 Runtime Evidence through Slice 2V](phase-62-runtime-evidence.md)
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
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

## Current markers

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Current runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Current accepted Phase 62 state:
repository, source CI and real yaVDR runtime accepted through Slice 2W

Accepted Slice-2W source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Accepted Slice-2W source CI:
#6834 / run 30745952119 / all five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119

Runtime acceptance:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Installed/running daemon SHA-256:
7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571

Installed loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Runtime report SHA-256:
e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea

Next bounded implementation slice:
not yet selected
```

Phase 62 remains active and incomplete. Phase 63-67 runtime has not been
advanced. PR #117 remains open, Draft and unmerged.

## Fully accepted Slice 2W

Slice 2W added one bounded terminal browser-session retention pass during
Security Runtime initialization.

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay
fixed batch size  256
```

The accepted implementation:

- runs after security schema/configuration validation and before
  `securityReady`;
- selects only old terminal verifiers from explicit revocation, absolute expiry
  or enabled idle expiry;
- rechecks, audits and deletes inside one `BEGIN IMMEDIATE` transaction;
- deletes only the verifier plus its own unreferenced canonical session and
  exact-type `browser-session` credential;
- preserves actors, devices, issuers, grants, roles and accountability;
- fails closed and rolls back the whole batch after any enabled-policy failure;
- adds no scheduler, request-path cleanup, issuer cascade or concurrency
  eviction.

The isolated real-yaVDR acceptance proved disabled no-op, forced-failure
rollback, active/within-retention preservation, old terminal deletion,
non-browser and re-referenced-row preservation, exact secret-free accountability,
258 candidates with exactly 256 deterministic deletions, SQLite integrity,
unchanged production database/configuration/loader, removed systemd override,
final active accepted daemon and zero VDR domain mutations.

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313
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

- [Agent Workflow Rules](../../AGENTS.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Recording Action Readiness Audit](recording-action-readiness-audit.md)
- [Phase 55 completed acceptance history](completed-phases/phase-55.md)
- [Phase and documentation guardrails](../../tools/)

## Documentation placement rules

- Current verified state belongs in `docs/CURRENT.md` and current-status documents.
- Accepted slice runtime closeouts belong in `docs/development/`.
- Stable architecture belongs in `docs/architecture/`.
- Future dependency order and open gaps belong in `docs/planning/`.
- Accepted ADRs remain in `docs/adr/` and do not move into completed phases.
- Phase 62 slices are not separate numbered phases.
- Historical phase files remain traceability records and must not be linked as equal current entry points.

## Exact next action

Perform one fresh post-Slice-2W gap analysis and select exactly one smallest
coherent next Phase-62 slice. Document the selection and require all five CI jobs
before implementation.

Do not combine multiple remaining security themes or advance Android, Android TV
or Phase 63-67 runtime.

## Related navigation

- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)
