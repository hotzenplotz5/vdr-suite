# Development Documentation

## Current implementation truth

- [Current Project Status](current-status.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Slice 2X Runtime Closeout](phase-62-slice-2x-runtime-closeout.md)
- [Slice 2X Accepted Contract](phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2X yaVDR Runbook](phase-62-slice-2x-runtime-acceptance-runbook.md)
- [Slice 2W Runtime Closeout](phase-62-slice-2w-runtime-closeout.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phase Archive](completed-phases/README.md)

## Current markers

```text
Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Completed hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active runtime phase:
none; Phase 63 is planned but not started

Final Phase 62 runtime acceptance:
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS

Accepted runtime head:
4762583d5b5170866838ed9f03b928adbf39f99e

Accepted source CI:
#6884 / run 30752351218 / all five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30752351218

Installed/running daemon SHA-256:
488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5

Installed loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Runtime report SHA-256:
bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf

Durable evidence:
/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

Phase 62 is completed. Phase 63-67 runtime has not been advanced. PR #117 remains open, Draft and unmerged.

## Completed Phase 62 result

Phase 62 delivered:

- persistent actor, device, session and credential identity;
- Legacy Basic compatibility, optional Managed Basic and browser sessions;
- strict credential precedence and persistent lifecycle resolution;
- exact actor/backend grants and fixed Admin/Read-only roles;
- complete central POST protection or explicit Safe POST classification;
- browser-session issue/logout, CSRF, absolute/idle expiry, issuer binding, concurrency and terminal retention cleanup;
- append-only pre-dispatch authorization evidence;
- browser lifecycle outcomes;
- protected mutation success/failure outcomes with continuous non-secret context;
- rollback-safe real-yaVDR acceptance tooling.

The final isolated runtime pass proved one protected HTTP 200 `operation.succeeded` event pair and one deterministic HTTP 500 `operation.failed` event pair. The production database remained unchanged during the scenario, the temporary systemd override was removed and the normal service remained active.

## Compatibility-retirement decision

Legacy Basic remains explicitly transitional. It is retained because `legacy-basic` remains the code default and packaged deployments do not yet mandate migration to `enforced`.

Removal requires a separate future deployment-migration contract and does not reopen Phase 62.

## Historical Phase 62 contract references

The completed contracts remain linked because architecture guards use them as traceability anchors:

- [Slice 2I — Recording Execution Security Migration](phase-62-slice-2i-recording-execution-security-migration.md)
- [Slice 2J — SearchTimer Create Security Migration](phase-62-slice-2j-searchtimer-create-security-migration.md)
- [Slice 2K — Runtime Acceptance Harness](phase-62-slice-2k-runtime-acceptance-harness.md)

The complete Slice 1 through Slice 2W records remain in this directory as historical evidence, not active implementation prompts.

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

- Current verified state belongs in `docs/CURRENT.md` and current-status documents.
- Accepted runtime closeouts belong in `docs/development/`.
- Stable architecture belongs in `docs/architecture/`.
- Future dependency order and open gaps belong in `docs/planning/`.
- Accepted ADRs remain separate from runtime completion.
- Historical slice files remain traceability records and must not be treated as active prompts.

## Exact next action

Require final all-green documentation CI, then obtain explicit repository-owner approval before updating PR #117 metadata, marking it Ready for Review or merging it. Define Phase 63 in a new bounded contract only after PR #117 disposition.

## Related navigation

- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)
