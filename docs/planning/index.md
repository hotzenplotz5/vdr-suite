# Planning Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)

## Purpose

This section contains binding future execution order, product acceptance journeys and living gap/dependency registers. It does not own volatile operational state.

## Authoritative planning documents

- [Strict Roadmap](roadmap.md) — binding numbered phase order and completion gates.
- [Phase Map](phase-map.md) — compact phase-number map.
- [Golden User Journeys](golden-user-journeys.md) — vertical user-visible acceptance.
- [Implementation Dependency Map](implementation-dependency-map.md) — stable prerequisite ordering.
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md) — living accepted-code gap register.
- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md) — frontend/product parity gap register.
- [Platform Productization Roadmap](platform-productization-roadmap.md) — federated MultiBackend sharing, permissioned pure clients, first-party living-room/output client and Debian/Ubuntu release packaging.
- [ADR Index](../adr/index.md) — accepted architectural decisions.

## Stable phase dependency chain

```text
Phase 62 — Identity, RBAC and Accountability [COMPLETED]
  -> Phase 63 — Backend Agent and Secure Multi-Site Runtime [COMPLETED]
  -> Phase 64 — Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 — Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 — Media Home and Browse Experience [COMPLETED]
  -> Phase 67 — Broadcast Companion Services: Teletext and HbbTV [COMPLETED]
  -> Phase 68 — Legacy OSD Compatibility Bridge [COMPLETED]
  -> Phase 69 — Public API and Client Compatibility Hardening [COMPLETED]
  -> Phase 70 — Recommendation and Content Knowledge Graph [NOT STARTED]
```

Current completed/active/next state belongs only in [Current State](../CURRENT.md).

## Current planning anchor

Phase 66 is completed, including its Golden Home journeys. Later non-numbered Home rebuild/hardening is also complete for the merged accepted scope and is recorded in [Post-Phase-66 Home Rebuild Closeout](../development/post-phase66-home-rebuild-closeout.md).

The completed numbered planning boundary is now Phase 69; 69.A-F are accepted. Phase 70 is the next strict numbered phase but is not started and requires its own accepted runtime ADR:

- [ADR-0047](../adr/ADR-0047-legacy-osd-compatibility-bridge.md) — completed Legacy OSD compatibility architecture;
- [Phase 68 Closeout](../development/phase-68-closeout.md) — accepted 68.A-G runtime evidence;
- [Golden User Journeys](golden-user-journeys.md) — Journey 10 accepted;
- [Phase 69 Kickoff](../development/phase-69-public-api-kickoff.md) — Phase-69 runtime progress and inventory guard;
- [Phase 69.C Closeout](../development/phase-69c-closeout.md) — accepted revision/precondition/idempotency and productive Timer CREATE evidence;
- [Phase 69.D Closeout](../development/phase-69d-closeout.md) — accepted collection envelope, keyset pagination and federated partial-result evidence;
- [Phase 69.E Closeout](../development/phase-69e-closeout.md) — accepted compatibility/deprecation policy, complete retained-route classification and explicit client-fallback debt boundary.
- [Phase 69 Closeout](../development/phase-69-closeout.md) — completed 69.F client hardening and numbered Phase-69 acceptance evidence.

For successor planning, re-read live `main`, CURRENT, the Strict Roadmap and the Phase-69 closeout. Do not start Phase 70 until its required runtime ADR is accepted.

The active cross-cutting productization line is [Post-Phase-69 Multiuser Productization Workstream](../development/post-phase69-multiuser-workstream.md): MU.0-MU.7 are complete; MU.6B Public Account item/revision and MU.6C public lifecycle mutation are completed, MU.6D Atomic Account CREATE + durable idempotency merged as PR #411, and MU.7 Backend access / permission grant administration passed REAL YAVDR acceptance and merged as PR #413. MU.8 is in progress through the MU.8B Session Revoke implementation candidate. MU.8A merged as PR #415; real yaVDR acceptance and credential revoke remain pending.

## Planning cautions

- A completed phase is not reopened merely because later work reuses or hardens its contracts.
- An accepted ADR defines architecture but does not prove runtime implementation.
- Historical closeouts may preserve once-current future numbering; current order comes from this planning set and `CURRENT.md`.
- Cross-cutting product/admin work does not silently advance the numbered phase.
- Private RESTfulAPI, SVDRP, Streamdev or SuiteBridge reachability never becomes client/provider authority by accident.

## Completed evidence

- [Phase 68 Closeout](../development/phase-68-closeout.md)
- [Completed Phases](../development/completed-phases.md)
- [Post-Phase-66 Home Rebuild Closeout](../development/post-phase66-home-rebuild-closeout.md)
- [Phase 66 Closeout](../development/phase-66-closeout.md)
- [Phase 65 Closeout](../development/phase-65-closeout.md)
- [Phase 64 Final Closeout](../development/phase-64-closeout.md)
- [Phase 62 Final Closeout](../development/phase-62-closeout.md)

## Related current documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](../development/current-status.md)
- [Current Architecture State](../development/current-architecture-state.md)

## Back

- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
