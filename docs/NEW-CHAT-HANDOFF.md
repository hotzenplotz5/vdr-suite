# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. A new chat alone is not a changed repository or runtime fingerprint. Do not repeat accepted Phase-62 analysis, CI or real-runtime acceptance without a directly relevant change.

## Canonical reading

- [Current State](CURRENT.md)
- [Current Project Status](development/current-status.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md)
- [Phase 62 Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](adr/index.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Completed Phases](development/completed-phases.md)
- [Agent Workflow Rules](../AGENTS.md)

## Stable project position

```text
Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Previous completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active runtime phase:
none; Phase 63 is planned but not started

Phase 63-67 runtime:
not advanced
```

## Active workstream

```text
Repository: hotzenplotz5/vdr-suite
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Remote branch: phase-62-security-identity-foundation
Pull request: #117
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
PR state: open, Draft, unmerged, mergeable
```

PR #117 must not be marked Ready, merged, auto-merged, rebased, force-pushed or have Base, title, body, reviewers or other review/merge metadata changed without explicit repository-owner approval.

PR #118 is the separate paused TVScraper workstream.

## Final installed Phase 62 runtime

**VERIFIED on 2026-08-02:**

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
installed_daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
configuration_sha256=8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

The temporary Slice-2X systemd override was removed and the normal service was active. Do not rerun this acceptance without a relevant daemon, outcome-accountability, routing-order, database-isolation, systemd-entrypoint or harness change.

## Completed Phase 62 result

Phase 62 provides persistent identity, exact scoped authorization, fixed roles, browser-session lifecycle and CSRF policy, complete central POST classification, append-only allow/deny accountability, lifecycle outcomes and protected mutation success/failure outcomes.

The runtime acceptance proves exact HTTP 200 `operation.succeeded` and HTTP 500 `operation.failed` event pairs with actor, decision, operation, request and correlation continuity and no secret persistence.

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and is intentionally retained. Immediate removal is not ready because `legacy-basic` remains the code default and packaged deployments do not yet require migration to `enforced`.

This is the explicit Phase-62 retirement decision. Removal requires a separate future deployment-migration contract and is not an unclosed Phase-62 slice.

## Rejected and deferred work

Do not reopen Phase 62 merely to add audit products, generic security administration, native/service credential lifecycle without a concrete consumer, universal revision/idempotency infrastructure, transactional Outbox, Android, Android TV or Phase 63-67 runtime.

## Exact next action

1. Verify all five jobs green on the final Phase-62 closeout documentation head.
2. Ask for explicit approval before changing PR #117 metadata, marking Ready for Review or merging.
3. Begin Phase 63 only with a new bounded contract after PR #117 disposition.

## Credential and secret restrictions

Never print, store or commit Authorization headers, plaintext passwords, password hashes, cookies, CSRF tokens, raw session/verifier secrets, secret-bearing login responses or process environments.
