# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order. Completed history belongs in [Completed Phases](../development/completed-phases.md); compact numbering belongs in the [Phase Map](phase-map.md).

A roadmap item is not automatically an implementation requirement. New runtime work requires a binding requirement, a concrete accepted-code gap, a real failure/security consequence and the smallest closing change.

## Current verified position

Baseline: `main @ 24b1d7938ddaa15834a8da6323a270761868f4ba`.

```text
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

Completed Phase-63 slice:
Phase 63 Slice 1 - Backend Agent Enrollment and Lease Foundation
PR #137 merged with exact-head CI and real yaVDR acceptance

Current active runtime slice:
Phase 63 Slice 2 - Backend Health Observation Ingestion Runtime
Draft PR #139 implements the first bounded read-only observation domain

Phase 63 is not complete
```

## Completed prerequisites

### Phase 61 — Suite Metadata and Genre Platform

Status: **Completed.**

Persistent backend-scoped Recording/EPG metadata, people relations, canonical Genre assignments, indexed query-only browse paths and frontend integration.

### Post-Phase-61 hardening and platform features

Status: **Completed, non-numbered.**

- Performance Hardening B1-B4;
- VDR Remote and Live Overlay hardening (#110);
- Backend-scoped Global Search (#111);
- Configurable photorealistic VDR Remote (#115).

## Phase 62 — Identity, RBAC and Accountability Foundation

Status: **Completed.**

Goal achieved: production-grade actor identity, scoped server-side authorization, browser-session lifecycle security and append-only accountability.

Accepted runtime includes persistent identity, exact grants and roles, protected central POSTs, browser-session lifecycle and CSRF policy, append-only pre-dispatch evidence, browser lifecycle outcomes and protected mutation success/failure outcomes.

Final runtime evidence:

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

### Compatibility-retirement decision

Legacy Basic compatibility remains transitional and is retained at closeout. Immediate removal is not deployment-ready because `legacy-basic` remains the code default and packaged deployments do not yet mandate migration to `enforced`.

Removal requires a separate future migration contract. This explicit decision satisfies the Phase-62 exit criterion and does not authorize another Phase-62 slice.

## Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: **Active; Slice 1 completed, Slice 2 contract merged and `backend-health` runtime active.**

### Phase 63 Slice 1 — Backend Agent Enrollment and Lease Foundation

Status: **Completed, accepted and merged.**

PR #137 was squash-merged as `a9620179a442155f0860ef3182ca39186ac46a57`. The accepted source head `bba51455552bab0f1a06c680369c508858b2384b` passed VDR-Suite CI #7256 and the guarded real yaVDR acceptance:

```text
PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS
VDR_NATIVE_STATE_UNCHANGED=yes
DAEMON_ACTIVE=yes
AGENT_ACTIVE=yes
```

Slice 1 established Agent enrollment/device identity, protected outbound transport, protocol compatibility, backend generation, Agent-process-instance fencing, heartbeat/lease, read-only capabilities, reconnect reconciliation and credential rotation/revocation/replacement.

Binding closeout: [Phase 63 Slice-1 Closeout](../development/phase-63-slice-1-closeout.md).

### Phase 63 Slice 2 — Read-only Observation and Snapshot Ingestion Foundation

Status: **Active runtime in Draft PR #139; contract merged in PR #138.**

Binding contract: [Phase 63 Observation and Snapshot Ingestion](../development/phase-63-observation-ingestion.md).

Draft PR #139 closes the first accepted-code gap for remote read-state ingestion by implementing the bounded `backend-health` domain on the merged contract. Explicit generation, baseline and sequence semantics prevent stale overwrite, conflicting replay and guessed continuity.

The active implementation provides:

- existing `vdr-suite-agent/1` technical authentication;
- backend, Agent, Agent instance and backend-generation fencing;
- bounded read-only observation domains declared by capabilities;
- complete snapshot generation independent from producer sequence and resource revision;
- complete baseline before changes;
- exact-next sequence acceptance;
- idempotent equivalent replay and conflicting replay rejection;
- explicit `resync-required` on gaps or missing baselines;
- atomic immutable receipt/fact and ingestion-cursor persistence through Suite-owned repositories;
- initial bounded `backend-health` implementation before broader VDR domains.

Snapshot/change ingestion remains read-only. Command/result flow, native execution and provider selection remain later Phase-63 slices with separate contracts.

## Phase 64 — Timer Intent and Multi-Backend Orchestration

Status: **Planned after Phase 63.**

Separate timer intent, assignment and native binding; deterministic scheduling/reconciliation; readback, drift and uncertain-dispatch recovery.

## Phase 65 — Streaming Gateway and Media Sessions

Status: **Planned after Phase 64.**

Authorized short-lived media sessions, Gateway-owned connections, capacity leases, Live/Recording pass-through and private provider routing.

## Phase 66 — Legacy OSD Compatibility Bridge

Status: **Planned after Phase 65.**

Immutable OSD snapshots/deltas, resynchronization, one fenced controller lease and allowlisted input.

## Phase 67 — Public API and Client Compatibility Hardening

Status: **Planned after Phase 66.**

Versioned discovery, compatible errors, resource-specific revisions/preconditions, durable operations where required, pagination and migration aliases.

## Phase 68 — Recommendation and Content Knowledge Graph

Status: **Later vision.**

Requires stable metadata/provenance, actor privacy, stable identities, mature accountability and public API contracts.

## Cross-cutting completion gates

- **Identity gate:** stable Suite identity and explicit native binding.
- **Provider gate:** provider facts carry provenance and never become hidden authority.
- **Mutation gate:** authentication, CSRF where applicable, authorization, required preconditions, dispatch evidence, verification and accountability.
- **Native boundary gate:** no raw VDR pointer/lock crosses async, network or database work.
- **Client gate:** clients consume Suite contracts, never private provider details.
- **Acceptance gate:** focused tests, regressions, build/package validation and real-system proof where runtime behaviour changes.

## Slice-2 hard exclusions

No command inbox, command dispatch, receipts or results; no Timer, Recording, SearchTimer, Remote or configuration mutation; no VDR-native execution; no provider ownership/selection; no public Agent/provider URLs; no TimerIntent or Phase 64; no Streaming Gateway; no OSD runtime; no replacement of direct-adapter `BackendNode.online` authority.

## Exact next action

Stabilize Draft PR #139 on one exact head, obtain all required CI jobs, install that exact head on yaVDR and execute the upgrade-safe `backend-health` acceptance while preserving the existing Agent identity. Keep the PR Draft until explicit approval.

## Related documents

- [Current State](../CURRENT.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
- [Phase Map](phase-map.md)
- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
- [Phase 63 Slice-1 Closeout](../development/phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](../development/phase-63-observation-ingestion.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
