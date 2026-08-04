# Development Documentation

## Current implementation truth

- [Current Project Status](current-status.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

## Current markers

```text
Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 repository state:
completed and merged through PR #117

Completed hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
none; Phase 63 is planned but not started

Final historical Phase 62 runtime acceptance:
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS

Accepted runtime head:
4762583d5b5170866838ed9f03b928adbf39f99e

Accepted source CI:
#6884 / run 30752351218 / all five jobs successful

Installed/running daemon SHA-256:
488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5

Runtime report SHA-256:
bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf

Durable evidence:
/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

PR #117 is merged as `f9e5f88bc223a2ce8a30fdbf4596893b34bc1551`. Phase 62 is completed. Phase 63-67 runtime has not been advanced.

## Completed Phase 62 result

Phase 62 delivered persistent identity, exact actor/backend authorization, fixed roles, protected central mutations, browser-session lifecycle and CSRF policy, append-only authorization evidence and protected mutation success/failure outcomes. The final isolated runtime pass proved exact protected HTTP 200 and HTTP 500 outcome pairs, production-database isolation, cleanup, removed systemd override and active final service.

Legacy Basic remains explicitly transitional. Removal requires a separate future deployment-migration contract and does not reopen Phase 62.

## Completed post-Phase-62 work

- PR #118: TVScraper classification and refresh corrections.
- PR #123: public-base-path-safe EPG artwork resolution.
- PR #132: guarded external series-artwork fallback, TVmaze/TMDB providers, secure backend settings, deterministic provider identity and poster/cover preference.
- `96b97378` and `2d04a963`: channel-detail artwork/text layout correction and regression coverage.

PR #132 was merged as `441e5febf7d3ab0121a585ce1176a8e5a7c67ce0`. Its final feature head passed VDR-Suite CI #6982 with all five jobs successful. Real yaVDR operation proved persisted TMDB fallback assets and browser delivery.

The security impact and evidence boundary are recorded in [Post-Phase-62 Security Review](post-phase-62-security-review.md). The historical Phase-62 acceptance remains the completion evidence for its accepted candidate, not a byte-for-byte acceptance of later daemon builds.

## Historical Phase 62 contract index

These completed contracts remain architecture and test traceability anchors. They are not active implementation prompts.

- [Slice 1 — Security Identity Foundation](phase-62-security-identity-foundation-slice-1.md)
- [Slice 2 — Persistent Identity Foundation](phase-62-security-identity-foundation-slice-2.md)
- [Slice 2F — Minimal Role Model](phase-62-slice-2f-minimal-role-model.md)
- [Slice 2G — Timer CRUD Security Migration](phase-62-slice-2g-timer-crud-security-migration.md)
- [Slice 2H — Channel Move Security Migration](phase-62-slice-2h-channel-move-security-migration.md)
- [Slice 2I — Recording Execution Security Migration](phase-62-slice-2i-recording-execution-security-migration.md)
- [Slice 2J — SearchTimer Create Security Migration](phase-62-slice-2j-searchtimer-create-security-migration.md)
- [Slice 2K — Runtime Acceptance Harness](phase-62-slice-2k-runtime-acceptance-harness.md)
- [Slice 2L — SearchTimer Maintenance Security Migration](phase-62-slice-2l-searchtimer-maintenance-security-migration.md)
- [Slice 2M — Safe POST Classification](phase-62-slice-2m-safe-post-classification.md)
- [Slice 2N — SearchTimer Execution Security Migration](phase-62-slice-2n-searchtimer-execution-security-migration.md)
- [Slice 2O — Native Fuzzy Refresh Security Migration](phase-62-slice-2o-native-fuzzy-refresh-security-migration.md)
- [Slice 2P — Query Cache Refresh Security Migration](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Slice 2Q — Native Fuzzy Stale-Probe Delete Security Migration](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Slice 2R — Browser-Session Lifetime Configuration](phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Slice 2S — Browser-Session Outcome Accountability](phase-62-slice-2s-browser-session-outcome-accountability.md)
- [Slice 2T — Browser-Session Issuer Binding](phase-62-slice-2t-browser-session-issuer-binding.md)
- [Slice 2U — Browser-Session Concurrency Limit](phase-62-slice-2u-browser-session-concurrency-limit.md)
- [Slice 2V — Browser-Session Idle Expiry](phase-62-slice-2v-browser-session-idle-expiry.md)
- [Slice 2W — Browser-Session Retention Cleanup](phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Slice 2W Runtime Closeout](phase-62-slice-2w-runtime-closeout.md)
- [Slice 2X — Protected Mutation Response Outcomes](phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2X Runtime Acceptance Runbook](phase-62-slice-2x-runtime-acceptance-runbook.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)

## Developer references

- [Developer Onboarding](developer-onboarding.md)
- [Architecture Map](architecture-map.md)
- [Build System State](build-system-state.md)
- [GitHub Actions Status Handoff](github-actions-status-handoff.md)
- [Web Client API Contract Snapshot](web-client-api-contract-snapshot.md)

## Runtime and acceptance references

- [Agent Workflow Rules](../../AGENTS.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)

## Documentation placement rules

- Current verified state belongs in `docs/CURRENT.md`, the Handoff and current-status documents.
- Historical runtime hashes stay in their accepted closeouts.
- Post-closeout security impact belongs in `post-phase-62-security-review.md` until superseded by newer evidence.
- Stable architecture belongs in `docs/architecture/`.
- Future dependency order and open gaps belong in `docs/planning/`.
- Accepted ADRs remain separate from runtime completion.
- Historical slice files remain traceability records and must not be treated as active prompts.

## Exact next action

Complete the bounded route-derived audit-scope hardening and dedicated settings-mutation security tests, refresh post-Phase-62 evidence, and define Phase 63 only under a separate approved contract.

## Related navigation

- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)
