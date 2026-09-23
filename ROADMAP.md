# VDR-Suite Roadmap

## Navigation

- [README](README.md)
- [Current State](docs/CURRENT.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Golden User Journeys](docs/planning/golden-user-journeys.md)
- [Completed History](docs/development/completed-phases.md)
- [Post-Phase-66 Home Rebuild Closeout](docs/development/post-phase66-home-rebuild-closeout.md)

## Purpose

This root file is the compact roadmap entry point. The authoritative execution order, phase prerequisites and completion gates live in [docs/planning/roadmap.md](docs/planning/roadmap.md). Volatile completed/active/next phase status lives in [docs/CURRENT.md](docs/CURRENT.md). Exact historical implementation evidence belongs in closeouts.

## Current position

```text
Latest completed numbered runtime phase:
Phase 68 - Legacy OSD Compatibility Bridge

Current active numbered runtime phase:
Phase 69 - Public API and Client Compatibility Hardening

Next strict numbered runtime phase:
Phase 69 - Public API and Client Compatibility Hardening
```

Phase 66 is completed. The later non-numbered Home performance, Recording Discovery, metadata/artwork, native Recording editing and Home-rebuild work is also completed for the merged accepted scopes and does not reopen Phase 66. The consolidated Home-rebuild evidence is recorded in [Post-Phase-66 Home Rebuild Closeout](docs/development/post-phase66-home-rebuild-closeout.md).

Phase 67 is completed through the accepted Teletext and HbbTV verticals. Phase 68 Legacy OSD Compatibility Bridge is also completed through 68.G fenced allowlisted native input; durable evidence is in [Phase 68 Closeout](docs/development/phase-68-closeout.md) and [Phase 68 Kickoff](docs/development/phase-68-legacy-osd-kickoff.md). Phase 69 is active at 69.C Revision/precondition/idempotency exposure; 69.A and 69.B are accepted. See [Phase 69 Kickoff](docs/development/phase-69-public-api-kickoff.md) and [Phase 69.B Closeout](docs/development/phase-69b-closeout.md).

## Strict forward sequence

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience [COMPLETED]
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV [COMPLETED]
  -> Phase 68 - Legacy OSD Compatibility Bridge [COMPLETED]
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

## Cross-cutting product milestones

The following are deliberately not inserted as numbered phases:

- Account and Backend Access Administration;
- Broad Timer Product UI;
- Audit/Security/Operations product surfaces;
- Legacy Basic retirement;
- first-party browser/TV/native/Kodi client rollout;
- bounded post-phase correctness/performance hardening.

## Roadmap rule

Completed phases are not renumbered or reopened merely because later hardening consumes their contracts. Historical closeouts may retain wording or future numbering that was true at the time of acceptance; current execution authority is always [docs/CURRENT.md](docs/CURRENT.md) plus the [Strict Roadmap](docs/planning/roadmap.md).
