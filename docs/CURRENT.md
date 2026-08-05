# VDR-Suite Current State

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 63 Backend Agent Foundation](development/phase-63-backend-agent-foundation.md)
- [Phase 63 Backend Agent Runtime Acceptance](development/phase-63-backend-agent-runtime-acceptance-runbook.md)
- [Manual Recording Cast Feature](development/manual-recording-cast-search.md)
- [Post-Phase-62 Security Review](development/post-phase-62-security-review.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](adr/index.md)
- [Documentation Index](index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Current branch authority: main
Current merged main baseline:
a125b702a6d3a7fe510a94c84dc1930d3b17a4c5

Latest merged bounded feature:
PR #136 - Add manual recording cast ingestion and search integration
Final source head: eb7afa4e6cc5998614ae28b06a1c0c75e85bea41
Merge commit: a125b702a6d3a7fe510a94c84dc1930d3b17a4c5
CI: VDR-Suite CI #7228 / 30981621649, all five jobs successful
Real yaVDR acceptance: completed before merge

Active numbered runtime slice:
Phase 63 Slice 1 - Backend Agent Enrollment and Lease Foundation
Draft PR #137 - Add backend agent enrollment and lease foundation
Branch: agent/phase63-backend-agent-foundation
State: Draft; implementation and stabilization in progress

Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Previous completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 63 Slice 1; Phase 63 is not complete

Phase 64-67 runtime:
not advanced
```

## Completed post-Phase-62 work

The merged platform includes VDR Remote and Live Overlay hardening (#110), Backend-scoped Global Search (#111), TVScraper classification fixes, EPG and series artwork resolution, secure backend-scoped provider settings, manual Recording metadata assignment from PR #135 and manual movie-cast ingestion/search integration from PR #136.

PR #136 keeps provider access backend-only, persists selected movie and cast atomically, distinguishes empty cast from provider failure, deduplicates canonical Suite people through provider-qualified identities, preserves reassignment/withdrawal history and keeps normal Recording/search GETs provider-free and set-based.

## Active Phase 63 Slice 1

Draft PR #137 introduces the smallest secure Agent lifecycle foundation:

- controlled one-time enrollment into an existing Backend binding;
- persistent technical Agent actor/device/credential identity;
- exact `vdr-suite-agent/1` protocol compatibility;
- backend-generation fencing per accepted Agent process instance;
- monotone heartbeat/lease and derived online/stale/offline Agent state;
- bounded read-only capability publication;
- restart/reconnect reconciliation;
- Agent-initiated credential rotation with persisted lost-response recovery;
- revocation and replacement enrollment while retaining revoked history;
- append-only Phase-62 accountability and central authorization reuse;
- outbound HTTPS Agent runtime, protected 0600 identity state, systemd-owned 0700 state storage and package/install staging;
- a local redacted status/revocation utility plus a fail-closed real-yaVDR acceptance harness covering lease transitions, credential rotation, revocation, replacement and VDR-native state fingerprints.

The slice executes no VDR operation. It does not implement commands, results, snapshots, provider selection, streaming, OSD, Timer orchestration or any Phase-64 work. Agent lease state does not overwrite existing direct-adapter `BackendNode.online` authority.

Binding contract: [Phase 63 Backend Agent Foundation](development/phase-63-backend-agent-foundation.md). Real-system execution: [Phase 63 Backend Agent Runtime Acceptance](development/phase-63-backend-agent-runtime-acceptance-runbook.md).

## Final Phase 62 runtime evidence

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
configuration_sha256=8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

This remains historical evidence for the accepted Phase-62 candidate. Later daemon changes do not reopen Phase 62 and are not covered byte-for-byte by that fingerprint.

## Current security position

Phase-62 identity, exact backend-scoped authorization, fixed Admin/Read-only roles, browser-session lifecycle, CSRF, fail-closed central POST classification and append-only accountability remain authoritative.

Agent enrollment is administrator-authorized and pre-bound to an existing Backend. Runtime Agent authentication resolves a technical Agent actor distinct from user/browser credentials. Backend binding, credential generation, Agent instance and backend generation are checked server-side. Raw bootstrap/runtime secrets, verifiers, Authorization headers, provider credentials, private URLs and secret-bearing process state are excluded from normal output and accountability context.

## Compatibility-retirement decision

Legacy Basic compatibility remains explicitly transitional. `enforced` mode is the fail-closed target. Retirement requires a separate deployment-migration contract and is not unfinished Phase 62.

## Current work boundary

- Phase 62 is complete.
- PR #136 is merged into the current `main` baseline.
- Draft PR #137 is the only active Phase-63 slice.
- PR #137 remains Draft until exact-head CI and real yaVDR acceptance are complete and the user explicitly approves readiness.
- No VDR-native mutation, provider selection, snapshot ingestion, command execution or later-phase runtime belongs in Slice 1.
- TVScraper remains an unchanged upstream dependency; Suite code writes no TVScraper-owned database or cache.

## Exact next action

Publish the guarded administration/runtime-acceptance stabilization for Draft PR #137, obtain all required VDR-Suite CI jobs on the new exact head, then execute the single documented yaVDR harness while proving existing VDR-native and direct-adapter state remains unchanged.
