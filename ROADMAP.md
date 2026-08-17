# VDR-Suite Roadmap

## Navigation

- [README](README.md)
- [Current State](docs/CURRENT.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Implementation Dependency Map](docs/planning/implementation-dependency-map.md)
- [Golden User Journeys](docs/planning/golden-user-journeys.md)
- [Architecture Gap Matrix](docs/planning/architecture-audit-gap-matrix.md)
- [ADR Index](docs/adr/index.md)
- [Completed History](docs/development/completed-phases.md)

## Purpose

This root file is only the compact roadmap entry point.

The authoritative execution order, phase prerequisites, coherent verticals and exit criteria live in [docs/planning/roadmap.md](docs/planning/roadmap.md). Volatile completed/active/next phase status lives in [docs/CURRENT.md](docs/CURRENT.md). Historical exact implementation evidence belongs in closeouts and [Completed History](docs/development/completed-phases.md).

## Current position

```text
Latest completed numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
none - Phase 65 has not started

Next strict numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions
```

Phase 64 is complete, including managed native Timer fulfillment and controlled reassignment/failover. Phase 65 remains next but requires an explicit kickoff before runtime implementation.

## Revised strict forward sequence

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [NEXT, NOT STARTED]
  -> Phase 66 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 67 - Legacy OSD Compatibility Bridge
  -> Phase 68 - Public API and Client Compatibility Hardening
  -> Phase 69 - Recommendation and Content Knowledge Graph
```

Phase 66 is backed by accepted ADR-0054. Runtime remains blocked until Phase 66 is explicitly started after Phase 65.

## Cross-cutting product milestones

The following are deliberately not inserted as numbered phases:

- Account and Backend Access Administration;
- Broad Timer Product UI;
- Audit/Security/Operations product surfaces;
- Legacy Basic retirement;
- first-party browser/TV/native/Kodi client rollout.

The Broad Timer Product UI depends on the completed Phase-62 identity/RBAC foundation, the completed Phase-64 Timer engine and the still-required account/backend access administration product boundary. It may proceed alongside Phase 65 without blocking Streaming.

## Roadmap rule

Completed phases are never renumbered or reopened merely because optional product surfaces remain.

Future not-yet-started phases may be reordered only through explicit repository planning/architecture reconciliation. Provider additions, diagnostics, administration products and frontend work must be classified explicitly rather than smuggled into a completed phase.

For all details, use the [Strict Roadmap](docs/planning/roadmap.md).
