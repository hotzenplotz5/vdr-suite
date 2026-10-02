# VDR-Suite Roadmap

## Navigation

- [README](README.md)
- [Current State](docs/CURRENT.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Golden User Journeys](docs/planning/golden-user-journeys.md)
- [Completed History](docs/development/completed-phases.md)
- [Post-Phase-66 Home Rebuild Closeout](docs/development/post-phase66-home-rebuild-closeout.md)
- [P2 Legacy Basic Retirement Closeout](docs/development/post-phase69-p2-legacy-basic-retirement-closeout.md)

## Purpose

This root file is the compact roadmap entry point. The authoritative execution order, phase prerequisites and completion gates live in [docs/planning/roadmap.md](docs/planning/roadmap.md). Volatile completed/active/next phase status lives in [docs/CURRENT.md](docs/CURRENT.md). Exact historical implementation evidence belongs in closeouts.

## Current position

```text
Latest completed numbered runtime phase:
Phase 69 - Public API and Client Compatibility Hardening

Current active numbered runtime phase:
none - Phase 70 - Recommendation and Content Knowledge Graph not started

Next strict numbered runtime phase:
Phase 70 - Recommendation and Content Knowledge Graph
```

Phase 66 is completed. The later non-numbered Home performance, Recording Discovery, metadata/artwork, native Recording editing and Home-rebuild work is also completed for the merged accepted scopes and does not reopen Phase 66. The consolidated Home-rebuild evidence is recorded in [Post-Phase-66 Home Rebuild Closeout](docs/development/post-phase66-home-rebuild-closeout.md).

Phase 67, Phase 68 and Phase 69 are completed for their accepted bounded scopes. Durable Phase-69 evidence is in [Phase 69 Closeout](docs/development/phase-69-closeout.md). The cross-cutting P2 Legacy Basic retirement milestone is also completed and accepted on the real yaVDR target; see [P2 Legacy Basic Retirement Closeout](docs/development/post-phase69-p2-legacy-basic-retirement-closeout.md). Phase 70 is the next strict numbered phase but is not started and has no runtime authorization until its required ADR is accepted.

## Strict forward sequence

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience [COMPLETED]
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV [COMPLETED]
  -> Phase 68 - Legacy OSD Compatibility Bridge [COMPLETED]
  -> Phase 69 - Public API and Client Compatibility Hardening [COMPLETED]
  -> Phase 70 - Recommendation and Content Knowledge Graph [NOT STARTED]
```

## Cross-cutting product milestones

The following are deliberately not inserted as numbered phases:

- Multiuser / Account and Backend Access Administration **[ACTIVE — MU.6 DONE / MU.7 IN PROGRESS / MU.7A CANDIDATE — ACCEPTANCE PENDING]**;
- Broad Timer Product UI **[PLANNED — gated on Multiuser administration]**;
- Audit/Security/Operations product surfaces;
- Legacy Basic retirement **[COMPLETED]**;
- first-party browser/TV/native/Kodi client rollout;
- bounded post-phase correctness/performance hardening.

Active Multiuser authority: [Post-Phase-69 Multiuser Productization Workstream](docs/development/post-phase69-multiuser-workstream.md).

Completed Multiuser foundation through MU.6: explicit Human Accounts, read-only Public-v1 Account discovery, First Admin/bootstrap, normal Human Account browser login, local recovery, Legacy Basic retirement, lifecycle administration and Atomic Account CREATE/idempotency. MU.6D passed real yaVDR acceptance and merged as PR #411. [ADR-0067](docs/adr/ADR-0067-human-account-backend-access-administration.md) remains the administration architecture authority. MU.7 grant administration is in progress through the bounded MU.7A channels.view@<concrete-backend> candidate; local and real yaVDR acceptance are pending.

Phase 70 remains a separate not-started numbered phase.

## Roadmap rule

Completed phases are not renumbered or reopened merely because later hardening consumes their contracts. Historical closeouts may retain wording or future numbering that was true at the time of acceptance; current execution authority is always [docs/CURRENT.md](docs/CURRENT.md) plus the [Strict Roadmap](docs/planning/roadmap.md).
