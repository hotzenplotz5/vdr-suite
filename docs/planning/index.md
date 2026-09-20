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
- [ADR Index](../adr/index.md) — accepted architectural decisions.

## Stable phase dependency chain

```text
Phase 62 — Identity, RBAC and Accountability [COMPLETED]
  -> Phase 63 — Backend Agent and Secure Multi-Site Runtime [COMPLETED]
  -> Phase 64 — Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 — Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 — Media Home and Browse Experience [COMPLETED]
  -> Phase 67 — Broadcast Companion Services: Teletext and HbbTV [COMPLETED]
  -> Phase 68 — Legacy OSD Compatibility Bridge [NEXT; NOT STARTED]
  -> Phase 69 — Public API and Client Compatibility Hardening
  -> Phase 70 — Recommendation and Content Knowledge Graph
```

Current completed/active/next state belongs only in [Current State](../CURRENT.md).

## Current planning anchor

Phase 66 is completed, including its Golden Home journeys. Later non-numbered Home rebuild/hardening is also complete for the merged accepted scope and is recorded in [Post-Phase-66 Home Rebuild Closeout](../development/post-phase66-home-rebuild-closeout.md).

The next numbered planning boundary is Phase 68:

- [ADR-0054](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md) — Teletext/HbbTV domain-first architecture;
- [Strict Roadmap Phase 67](roadmap.md) — execution sequence and acceptance gate;
- [Golden User Journeys](golden-user-journeys.md) — Teletext and HbbTV journeys.

Phase 67 runtime is completed; accepted planning does not itself start Phase 68 runtime.

## Planning cautions

- A completed phase is not reopened merely because later work reuses or hardens its contracts.
- An accepted ADR defines architecture but does not prove runtime implementation.
- Historical closeouts may preserve once-current future numbering; current order comes from this planning set and `CURRENT.md`.
- Cross-cutting product/admin work does not silently advance the numbered phase.
- Private RESTfulAPI, SVDRP, Streamdev or SuiteBridge reachability never becomes client/provider authority by accident.

## Completed evidence

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
