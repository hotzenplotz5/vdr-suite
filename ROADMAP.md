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
Phase 65 - Streaming Gateway and Media Sessions

Current active numbered runtime phase:
none - Phase 66 has not started

Next strict numbered runtime phase:
Phase 66 - Media Home and Browse Experience
```

Phase 65 is completed; see [Phase 65 Closeout](docs/development/phase-65-closeout.md). Accepted [ADR-0058](docs/adr/ADR-0058-media-home-responsive-browse-preview.md) and [Phase 66 Media Home and Browse Experience](docs/development/phase-66-media-home-browse-experience.md) define the next planning boundary. Runtime remains not started until a separate kickoff.

## Revised strict forward sequence

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience [NEXT; NOT STARTED]
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

Completed history is unchanged; ADR-0058 changes only not-yet-started future numbering.

## Cross-cutting product milestones

The following are deliberately not inserted as numbered phases:

- Account and Backend Access Administration;
- Broad Timer Product UI;
- Audit/Security/Operations product surfaces;
- Legacy Basic retirement;
- first-party browser/TV/native/Kodi client rollout.

The Broad Timer Product UI depends on the completed Phase-62 identity/RBAC foundation, the completed Phase-64 Timer engine and the still-required account/backend access administration product boundary. It may proceed alongside Phase 65 without blocking Streaming.

## Roadmap rule

Completed phases and accepted Phase-65 verticals/slices are not renumbered or reopened merely because optional product surfaces or still-deferred capabilities remain.

Future not-yet-started phases may be reordered only through explicit repository planning/architecture reconciliation. Provider additions, diagnostics, administration products and frontend work must be classified explicitly rather than smuggled into a completed phase or vertical.

For all details, use the [Strict Roadmap](docs/planning/roadmap.md).
