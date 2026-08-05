# VDR-Suite Current State

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 63 Slice-1 Closeout](development/phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](development/phase-63-observation-ingestion.md)
- [Phase 63 Backend Agent Foundation](development/phase-63-backend-agent-foundation.md)
- [Phase 63 Backend Agent Runtime Acceptance](development/phase-63-backend-agent-runtime-acceptance-runbook.md)
- [Manual Recording Cast Feature](development/manual-recording-cast-search.md)
- [Post-Phase-62 Security Review](development/post-phase-62-security-review.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](adr/index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Current branch authority: main
Current merged main baseline:
24b1d7938ddaa15834a8da6323a270761868f4ba

Latest merged bounded contract slice:
Phase 63 Slice 2 - Read-only Observation and Snapshot Ingestion Foundation
PR #138 - Define read-only agent observation ingestion contract
Accepted source head: 0207c0cbc01f167139b5d6483680f9a280c05160
Merge commit: 24b1d7938ddaa15834a8da6323a270761868f4ba
CI: VDR-Suite CI #7275 / 31006387349, all five jobs successful
Runtime change: none; contract and guards only

Active numbered runtime slice:
Phase 63 Slice 2 - Backend Health Observation Ingestion Runtime
Draft PR #139 - Add backend health observation ingestion runtime
Branch: agent/phase63-backend-health-ingestion-runtime
State: Draft runtime implementation; exact-head CI and real yaVDR acceptance pending

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
Phase 63 Slice 2; Phase 63 is not complete

Phase 64-67 runtime:
not advanced
```

## Completed cross-cutting platform features

- VDR Remote and Live Overlay hardening (#110)
- Backend-scoped Global Search (#111)
- Configurable photorealistic VDR Remote (#115)
- Manual Recording metadata assignment (#135)
- Manual selected-movie cast ingestion and search integration (#136)

## Completed Phase 63 Slice 1

PR #137 established and proved the secure Agent lifecycle foundation:

- controlled one-time enrollment into an existing Backend;
- persistent technical Agent actor/device/credential identity;
- exact `vdr-suite-agent/1` protocol compatibility;
- Agent-instance and backend-generation fencing;
- heartbeat/lease and online/stale/offline state;
- bounded read-only capability publication;
- reconnect and restart reconciliation;
- credential rotation, revocation and replacement enrollment;
- protected HTTPS transport and local state;
- systemd/package/install contracts;
- redacted administration and guarded real-yaVDR acceptance.

Exact real-system evidence:

```text
PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS
HEAD=bba51455552bab0f1a06c680369c508858b2384b
CONTROL_PLANE_URL=https://192.168.178.38/vdr-suite
CREDENTIAL_GENERATION=2
VDR_NATIVE_STATE_UNCHANGED=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
EVIDENCE=/var/backups/vdr-suite-phase63-20260805T114111Z-bba51455552b
```

The merged Agent remains read-only. Slice 1 implemented no snapshots, commands, VDR-native execution or provider selection.

## Active Phase 63 Slice 2 runtime

PR #138 merged the binding read-only Observation and Snapshot Ingestion contract. Draft PR #139 implements its first bounded runtime domain, `backend-health`, without command or VDR mutation paths.

The runtime separates and fences:

- authenticated Backend and Agent identity;
- Agent process instance;
- backend generation;
- observation domain;
- complete snapshot generation;
- producer sequence;
- resource revision.

It requires a complete baseline, exact-next change sequencing, idempotent equivalent replay, conflicting replay rejection and explicit `resync-required` on gaps or missing baselines. Accepted receipt/fact evidence and the ingestion cursor must commit atomically through Suite-owned repositories.

The implementation persists immutable receipts and one atomic ingestion cursor, accepts complete baseline plus exact-next changes, acknowledges equivalent replay idempotently, rejects conflicting replay and returns `resync-required` on gaps or missing baselines. The Agent persists protected lineage and a pending envelope before transport so an ambiguous response retries the exact same observation.

Adding recordings, timers, EPG or channels requires explicit identity and complete-snapshot semantics and is not automatic scope.

## Current security position

Phase-62 identity, exact backend-scoped authorization, fixed Admin/Read-only roles, browser-session lifecycle, CSRF, fail-closed central mutation classification and append-only accountability remain authoritative.

Agent credentials remain distinct technical identities. Observation endpoints accept no browser/user credential as Agent authentication. Backend generation, Agent instance, observation domain, snapshot generation and producer sequence are validated server-side. Raw bootstrap/runtime secrets, verifiers, Authorization headers, provider credentials, private URLs and secret-bearing process state are excluded from normal output and accountability context.

## Current work boundary

- Phase 62 is complete.
- Phase 63 Slice 1 is merged and accepted.
- Phase 63 Slice 2 runtime is active in Draft PR #139.
- Phase 63 is not complete.
- No command inbox/results, VDR-native mutation, provider ownership/selection, public provider URLs, TimerIntent/Phase-64, Streaming Gateway or OSD runtime belongs in Slice 2.
- Existing direct-adapter `BackendNode.online` authority is not replaced by Agent lifecycle or observations.
- TVScraper remains an unchanged upstream dependency; Suite code writes no TVScraper-owned database or cache.

## Exact next action

Stabilize Draft PR #139 on one exact head with full CI and the upgrade-safe real yaVDR acceptance path. Preserve the existing active Agent identity, prove `backend-health` baseline/change/replay/gap/restart semantics and keep the PR Draft until explicit approval.
