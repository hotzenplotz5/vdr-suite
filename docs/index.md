# VDR-Suite Documentation

## Navigation

- [README](../README.md)
- [Current State](CURRENT.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Architecture Decision Records](adr/index.md)

---

## Purpose

This index is a stable navigation page. It deliberately does **not** duplicate active PR numbers, branch heads, transient CI runs, current phase tips or other volatile operational facts.

For exact current project state, always use [Current State](CURRENT.md). For binding future execution order, use the [Strict Roadmap](planning/roadmap.md).

## Start here

- [Current State](CURRENT.md) — sole repository authority for volatile operational status.
- [New Chat Handoff](NEW-CHAT-HANDOFF.md) — mandatory entry point for a new VDR-Suite work session.
- [Current Project Status](development/current-status.md) — stable narrative context around the current platform direction.
- [Current Architecture State](development/current-architecture-state.md) — durable implemented ownership/capability summary.
- [Phase 65 Closeout](development/phase-65-closeout.md) — completed Streaming/MediaSession/playback boundary and final acceptance evidence.
- [Phase 66 Closeout](development/phase-66-closeout.md) — completed Media Home/Browse boundary and Golden acceptance evidence.
- [Post-Phase-66 Home Performance Hardening](development/post-phase-66-home-performance-hardening.md) — bounded non-numbered Home hardening evidence.
- [ADR-0058 Media Home, Responsive Browse and Preview Experience](adr/ADR-0058-media-home-responsive-browse-preview.md) — accepted Phase-66 architecture and completed runtime foundation.
- [Phase 66 Media Home and Browse Experience](development/phase-66-media-home-browse-experience.md) — accepted bounded implementation contract and completed sequence.
- [Phase 65.D Playback Semantics Consolidation](development/phase-65d-playback-semantics-consolidation.md) — completed Phase-65 semantic contract/history.
- [Frontend Playback Integration Contract](development/frontend-playback-integration-contract.md) — binding production-owner/lifecycle proof rules for playback frontend work.
- [Phase 64 Final Closeout](development/phase-64-closeout.md) — accepted Timer-engine completion boundary and exact evidence.
- [Project Overview](project-overview.md) — compact product and architecture overview.
- [Project Principles](project-principles.md) — binding product and engineering principles.
- [Strict Roadmap](planning/roadmap.md) — binding phase order and completion gates.
- [Golden User Journeys](planning/golden-user-journeys.md) — vertical product acceptance paths.

## Architecture

- [Architecture Documentation](architecture/index.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Current Architecture State](development/current-architecture-state.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [Domain Dependency Map](planning/domain-dependency-map.md)
- [Implementation Dependency Map](planning/implementation-dependency-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Architecture Decision Records](adr/index.md)

## Development and history

- [Development Documentation](development/index.md)
- [Phase 66 Closeout](development/phase-66-closeout.md)
- [Post-Phase-66 Home Performance Hardening](development/post-phase-66-home-performance-hardening.md)
- [Phase 65.D Playback Semantics Consolidation](development/phase-65d-playback-semantics-consolidation.md)
- [Frontend Playback Integration Contract](development/frontend-playback-integration-contract.md)
- [Completed Phases](development/completed-phases.md)
- [Completed Phase Archive](development/completed-phases/README.md)
- [Phase 64 Final Closeout](development/phase-64-closeout.md)
- [Developer Onboarding](development/developer-onboarding.md)
- [Build System State](development/build-system-state.md)
- [GitHub Actions Status Handoff](development/github-actions-status-handoff.md)

Historical closeouts may contain exact accepted SHAs, CI runs, runtime fingerprints and evidence paths. Those values are historical evidence for the bounded candidate they close and must not be interpreted as the current repository head.

## Planning

- [Planning Documentation](planning/index.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Golden User Journeys](planning/golden-user-journeys.md)
- [VDR Ecosystem Parity and Product Gaps](planning/parity-audit-and-frontend-gap-roadmap.md)

## Status model

- **CURRENT** — volatile operational truth; owned only by `docs/CURRENT.md`.
- **ROADMAP** — binding dependency/order and phase gates; no exact active-head duplication.
- **ARCHITECTURE** — stable ownership, identity and contract boundaries.
- **COMPLETED** — historical implementation evidence for closed work.
- **PLANNED** — genuinely open work with explicit prerequisites.
- **HISTORICAL** — retained traceability that is not a current work prompt.
- **SUPERSEDED** — replaced content with a named current successor.
- **DEFERRED** — intentionally postponed work with named prerequisites.

Accepted ADRs change target contracts. They do not by themselves prove runtime implementation or phase completion.

## Back

- [Back to README](../README.md)
- [Back to Current State](CURRENT.md)
