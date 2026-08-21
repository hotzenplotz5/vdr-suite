# VDR-Suite Project Status Dashboard

## Navigation

- [README](../README.md)
- [Current State](CURRENT.md)
- [Project Overview](project-overview.md)
- [Strict Roadmap](planning/roadmap.md)
- [Golden User Journeys](planning/golden-user-journeys.md)

---

## Purpose

This page is a **capability dashboard**, not a second operational-status authority.

Exact active PRs, branch heads, CI runs, current checkpoint SHAs and the current completed/active/next phase position belong only in [Current State](CURRENT.md). When this page and `CURRENT.md` ever appear to disagree about volatile state, `CURRENT.md` wins.

## Platform capability view

| Area | Durable assessment | Ownership / boundary |
| --- | --- | --- |
| Core daemon and SQLite | Established platform foundation | Domain repositories own persistence and migrations. |
| Backend registry and access policy | Established | Backend identity/scope and server-owned access decisions. |
| Channels and EPG | Established product foundation | Suite read models and client API; VDR remains native schedule authority. |
| Recordings | Established product foundation | Recordings 2 owns delivered browsing/detail flows. |
| Recording actions | Strong bounded mutation foundation | Validation, policy, execution and readback remain Suite-controlled. |
| Metadata, people, artwork and Genres | Established | Provider evidence is normalized/persisted behind Suite contracts. |
| Search and SearchTimer | Strong foundation | SearchTimer/epgsearch remain automation sources, not global scheduler authority. |
| Identity, RBAC and accountability | Established platform foundation | Exact actor/backend authorization and append-only evidence. |
| Backend Agent and multi-site trust | Established platform foundation | Enrolled identity, generation fencing, observations, durable commands/results and explicit local provider ownership. |
| Protected native writes | Strong safety foundation | Idempotency, resource leases/fences, expected revisions, unknown outcome and authoritative readback. |
| Timer orchestration | Numbered roadmap domain | `TimerIntent -> TimerAssignment -> NativeTimerBinding`; exact current implementation checkpoint is in `CURRENT.md`. |
| Streaming Gateway / MediaSession | Active numbered roadmap domain with accepted runtime foundation | Recording/Live MediaSessions, progressive delivery and backend-scoped output/transcode policy are implemented; current next scope is in `CURRENT.md`. |
| Legacy OSD compatibility | Numbered roadmap domain | Isolated viewer/controller bridge; not conflated with LiveOverlay. |
| Stable public API/SDK | Numbered roadmap domain | Independent version/error/compatibility contract. |
| Recommendation/content graph | Later roadmap domain | Built only on mature Suite identity/provenance. |

## Product acceptance view

The project is evaluated through vertical outcomes, not only component completion:

- Live TV selection must reach picture and sound through an authorized MediaSession and clean up the previous route/provider resources on channel change.
- Recording playback must advertise seek/range/growing-recording capability truthfully: seek is used where supported, growing sources are not presented as immutable, and unsupported advanced seek remains explicitly unsupported until implemented. Durable Suite-owned resume/progress remains later capability work.
- Recording creation must preserve the chain from EPG programme to TimerIntent, TimerAssignment, managed NativeTimerBinding and authoritative native VDR readback.
- Multi-backend scheduling must reject stale, read-only, generation-mismatched or otherwise ineligible backends without exposing private providers to the user.
- Failure must remain classified and visible without blind duplicate mutation or silent provider switching.

See [Golden User Journeys](planning/golden-user-journeys.md).

## Status rules

- **Operational truth:** [Current State](CURRENT.md) only.
- **Execution order:** [Strict Roadmap](planning/roadmap.md).
- **Stable architecture:** [Target Platform Architecture](architecture/target-platform-architecture.md) and accepted ADRs.
- **Historical proof:** completed-phase closeouts and runtime evidence.
- **Product proof:** Golden User Journeys plus required real-system/client acceptance.

A green component CI run is not by itself proof that an installed native/media journey is complete.

## Links

- [Current State](CURRENT.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Strict Roadmap](planning/roadmap.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity and Product Gaps](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](development/completed-phases.md)

## Back

- [Back to Documentation Index](index.md)
- [Back to README](../README.md)
- [Back to Current State](CURRENT.md)
