# Planning Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)

---

## Purpose

This section contains binding future execution order, domain/implementation dependencies, product acceptance journeys and living gap registers.

It does **not** own volatile operational state. Exact current phase position, active PRs, branch heads and CI checkpoints belong only in [Current State](../CURRENT.md).

## Authoritative planning documents

### Execution order

- [Strict Roadmap](roadmap.md) — binding numbered phase order and completion gates.
- [Phase Map](phase-map.md) — compact phase-number map.
- [Implementation Dependency Map](implementation-dependency-map.md) — dependency order that later runtime work may not bypass.

### Product acceptance

- [Golden User Journeys](golden-user-journeys.md) — vertical user-visible acceptance paths across Timer, multi-backend failure and media playback.

### Architecture and domain dependencies

- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Domain Dependency Map](domain-dependency-map.md)
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [ADR Index](../adr/index.md)

### Product and ecosystem parity

- [VDR Ecosystem Parity and Product Gaps](parity-audit-and-frontend-gap-roadmap.md)
- [Post-Phase-61 Provider Strategy](tvscraper-recording-metadata-roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)

## Planning authority rules

```text
docs/CURRENT.md
  -> volatile operational truth

docs/planning/roadmap.md
  -> binding phase order and completion gates

docs/planning/phase-map.md
  -> compact numbered phase map

docs/planning/*dependency-map.md
  -> stable prerequisite direction

docs/planning/golden-user-journeys.md
  -> vertical product acceptance

ADRs
  -> accepted target decisions, not implementation proof

completed closeouts
  -> historical evidence, not active work prompts
```

No planning index or dependency document should copy an exact active branch head, current PR tip or CI checkpoint from `CURRENT.md`.

## Stable phase dependency chain

The numbered architecture sequence is:

```text
Phase 62 — Identity, RBAC and Accountability
  -> Phase 63 — Backend Agent and Secure Multi-Site Runtime
  -> Phase 64 — Timer Intent and Multi-Backend Orchestration
  -> Phase 65 — Streaming Gateway and Media Sessions
  -> Phase 66 — Broadcast Companion Services: Teletext and HbbTV
  -> Phase 67 — Legacy OSD Compatibility Bridge
  -> Phase 68 — Public API and Client Compatibility Hardening
  -> Phase 69 — Recommendation and Content Knowledge Graph
```

Which of these phases is currently completed, active or next is intentionally not repeated here. See [Current State](../CURRENT.md).

## Current Phase-65 planning anchor

Phase-65 runtime status remains owned by `CURRENT.md`. Stable planning for the active client-playback semantic boundary is now:

- [ADR-0056](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md) — normalized playback presentation/timeline/continuity/failure architecture;
- [Phase 65.D Playback Semantics Consolidation](../development/phase-65d-playback-semantics-consolidation.md) — bounded implementation sequence;
- [Frontend Playback Integration Contract](../development/frontend-playback-integration-contract.md) — production owner/lifecycle proof rules.

These documents do not authorize Phase 66 and do not reopen already accepted Phase-65.D transport/control slices.

## Planning cautions

- A completed phase is not reopened merely because later work reuses or hardens its contracts.
- An accepted ADR defines architecture but does not by itself prove runtime implementation or phase completion.
- A historical slice document may name a once-proposed successor; that does not make the successor currently authorized.
- A broad polished UI may have different product prerequisites from the underlying engine phase and must not silently become an engine completion dependency.
- Later phases may prepare bounded internal contracts only when they do not publish, activate or bypass earlier prerequisites.
- Private RESTfulAPI, SVDRP, Streamdev or SuiteBridge reachability never becomes client/provider authority by accident.

## Completed evidence

Closed implementation evidence remains under `docs/development/` and `docs/development/completed-phases/`. Historical exact SHAs, CI runs, hashes and evidence directories are valid for the bounded candidate they close and should remain there rather than being recopied into current planning pages.

Useful entry points:

- [Completed Phases](../development/completed-phases.md)
- [Phase 62 Final Closeout](../development/phase-62-closeout.md)
- [Phase 63 development/closeout documents](../development/index.md)
- [Phase 64 Final Closeout](../development/phase-64-closeout.md)

## Historical and superseded planning evidence

- [Repository-truth refresh archive](history/repository-truth-refresh-2026-07/README.md)

## Related current documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](../development/current-status.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Phase 65.D Playback Semantics Consolidation](../development/phase-65d-playback-semantics-consolidation.md)

## Back

- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
