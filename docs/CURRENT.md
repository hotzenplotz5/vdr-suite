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
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](adr/index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Current branch authority: main
Current merged main baseline:
a9620179a442155f0860ef3182ca39186ac46a57

Latest merged bounded runtime slice:
Phase 63 Slice 1 - Backend Agent Enrollment and Lease Foundation
PR #137 - Add backend agent enrollment and lease foundation
Accepted source head: bba51455552bab0f1a06c680369c508858b2384b
Accepted tree: 575f49a197cda9ad02da4035b437ee1c32bed2d6
Merge commit: a9620179a442155f0860ef3182ca39186ac46a57
CI: VDR-Suite CI #7256 / 31001478896, all five jobs successful
Real yaVDR acceptance: PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS

Active numbered runtime slice:
Phase 63 Slice 2 - Read-only Observation and Snapshot Ingestion Foundation
Draft PR #138 - Define read-only agent observation ingestion contract
Branch: agent/phase63-observation-ingestion-contract
State: Draft contract/closeout; runtime implementation not yet included

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

## Active Phase 63 Slice 2

Draft PR #138 defines the binding contract for read-only Observation and Snapshot Ingestion before runtime implementation begins.

The contract separates and fences:

- authenticated Backend and Agent identity;
- Agent process instance;
- backend generation;
- observation domain;
- complete snapshot generation;
- producer sequence;
- resource revision.

It requires a complete baseline, exact-next change sequencing, idempotent equivalent replay, conflicting replay rejection and explicit `resync-required` on gaps or missing baselines. Accepted receipt/fact evidence and the ingestion cursor must commit atomically through Suite-owned repositories.

The initial implementation domain is deliberately bounded to `backend-health`. Adding recordings, timers, EPG or channels requires explicit identity and complete-snapshot semantics and is not automatic scope.

## Current security position

Phase-62 identity, exact backend-scoped authorization, fixed Admin/Read-only roles, browser-session lifecycle, CSRF, fail-closed central mutation classification and append-only accountability remain authoritative.

Agent credentials remain distinct technical identities. Observation endpoints accept no browser/user credential as Agent authentication. Backend generation, Agent instance, observation domain, snapshot generation and producer sequence are validated server-side. Raw bootstrap/runtime secrets, verifiers, Authorization headers, provider credentials, private URLs and secret-bearing process state are excluded from normal output and accountability context.

## Current work boundary

- Phase 62 is complete.
- Phase 63 Slice 1 is merged and accepted.
- Phase 63 Slice 2 is the active bounded contract and next runtime implementation target.
- Phase 63 is not complete.
- No command inbox/results, VDR-native mutation, provider ownership/selection, public provider URLs, TimerIntent/Phase-64, Streaming Gateway or OSD runtime belongs in Slice 2.
- Existing direct-adapter `BackendNode.online` authority is not replaced by Agent lifecycle or observations.
- TVScraper remains an unchanged upstream dependency; Suite code writes no TVScraper-owned database or cache.

## Exact next action

Stabilize Draft PR #138 on one exact head with the Slice-1 closeout, Slice-2 Observation and Snapshot Ingestion contract, fail-closed contract checker and updated architecture/status documentation. Obtain all required CI jobs before considering the subsequent bounded runtime-implementation PR.
