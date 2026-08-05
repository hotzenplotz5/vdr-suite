# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order. Completed history belongs in [Completed Phases](../development/completed-phases.md); compact numbering belongs in the [Phase Map](phase-map.md).

A roadmap item is not automatically an implementation requirement. New runtime work requires a binding requirement, a concrete accepted-code gap, a real failure/security consequence and the smallest closing change.

## Current verified position

Baseline: `main @ a125b702a6d3a7fe510a94c84dc1930d3b17a4c5`.

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

Current active runtime phase:
Phase 63 Slice 1 in Draft PR #137; Phase 63 is not complete
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
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

Evidence:

- [Current State](../CURRENT.md)
- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](../development/phase-62-slice-2x-runtime-closeout.md)
- [Phase 62 Gap Matrix](phase-62-security-identity-gap-matrix.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)

### Compatibility-retirement decision

Legacy Basic compatibility remains transitional and is retained at closeout. Immediate removal is not deployment-ready because `legacy-basic` remains the code default and packaged deployments do not yet mandate migration to `enforced`.

Removal requires a separate future migration contract. This explicit decision satisfies the Phase-62 exit criterion and does not authorize another Phase-62 slice.

### Deferred work

Audit HTTP products, generic security administration, native/service credential lifecycle without a concrete consumer, universal revision/idempotency infrastructure, transactional Outbox, Android and Android TV work are not required to close Phase 62.

## Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: **Active; Slice 1 in Draft PR #137.**

The first bounded slice implements Agent enrollment/device identity, protected outbound transport, protocol compatibility, backend generation, heartbeat/lease, read-only capability publication, reconnect reconciliation, credential rotation/revocation and persistence/accountability foundations.

Binding contract: [Phase 63 Backend Agent Foundation](../development/phase-63-backend-agent-foundation.md). Real-system gate: [Phase 63 Backend Agent Runtime Acceptance](../development/phase-63-backend-agent-runtime-acceptance-runbook.md).

Snapshot/change ingestion, command/result flow, provider selection, VDR-native execution and later-phase work remain explicitly unimplemented. Phase 63 is not complete when Slice 1 closes.

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

## Exact next action

Stabilize Draft PR #137 on one exact final head, obtain all required CI jobs and complete the guarded real yaVDR Slice-1 acceptance harness. Keep the PR Draft until the user explicitly approves readiness; do not advance into snapshots, commands or provider selection inside this slice.
