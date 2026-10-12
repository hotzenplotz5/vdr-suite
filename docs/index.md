# VDR-Suite Documentation

## Welcome — choose your route

You do **not** need to read the implementation history or all ADRs to understand VDR-Suite. Start with the overview, then follow the topic that matters to you.

### Recommended reading path

**First visit or installation question?** [Start with the beginner guide](getting-started.md) for an honest explanation of what you can try and how installation currently works.

1. **Understand the project:** [Project Overview](project-overview.md) — what VDR-Suite is and what it adds to native VDR.
2. **Explore the functionality:** [Project Status Dashboard](project-status-dashboard.md) — Live TV, Recordings, Home, Timers, security and other capabilities, with accepted scope distinguished from future work.
3. **Choose your topic:** [Follow the topic guide below](#choose-your-topic) — jump directly to the area that interests you, without reading architecture or old phase reports first.

[Deutsche Einführung](project-overview.de.md) · [English introduction](project-overview.en.md)

Architecture, ADRs, phase plans and original acceptance evidence are **optional deeper reading** when the chosen topic calls for them. The [Documentation Guide](documentation-guide.md) explains which sources are authoritative.

### Choose your topic

| Your question | Read this first | Go deeper |
| --- | --- | --- |
| How can I try VDR-Suite, and is there a packaged installer? | [Getting started](getting-started.md) | [Packaging status](../packaging/README.md), [staged deployment contract](development/install-runtime-deployment-reliability.md) |
| What is VDR-Suite meant to do? | [Project Overview](project-overview.md) | [Project Principles](project-principles.md) |
| What works, and what is *actually* accepted? | [Project Status Dashboard](project-status-dashboard.md) | [CURRENT — verified project status](CURRENT.md) |
| How do the server, VDR, Agent and clients fit together? | [Current Architecture State](development/current-architecture-state.md) | [Target Architecture](architecture/target-platform-architecture.md), [ADR Index](adr/index.md) |
| Where is the API and how do I write a client? | [API Developer Guide](../api/README.md) | [Partial OpenAPI contract](../api/openapi.json), [JS reference client](../clients/reference-js/README.md) |
| How do Account permissions and TV/device pairing work? | [Multiuser Workstream](development/post-phase69-multiuser-workstream.md) | [Pairing API](../api/device-pairing.md), [Device Lifecycle](../api/device-lifecycle.md) |
| Where do Recording playback and subtitles fit? | [Device Recording playback contract](development/public-v1-device-recording-playback.md) | [Playback integration](development/frontend-playback-integration-contract.md), [Device tracks integration history](development/vidaa-device-tracks-index-srt-integration-20261011.md) |
| How do I contribute, build or test? | [Clean-checkout Developer Quickstart](development/quickstart.md) | [CONTRIBUTING](../CONTRIBUTING.md), [Developer Onboarding](development/developer-onboarding.md) |
| How is installation or packaging organized? | [Packaging and staging guide](../packaging/README.md) | [Install-runtime contract](development/install-runtime-deployment-reliability.md), [Build Requirements](build-requirements.md) |
| What comes next, and what is still missing? | [Phase Map](planning/phase-map.md) | [Strict Roadmap](planning/roadmap.md), [Gap Matrix](planning/architecture-audit-gap-matrix.md) |
| Why was something designed this way? | [ADR Index](adr/index.md) | [Architecture Index](architecture/index.md) |
| Where are old phases, PR evidence and closeouts? | [Historical Documentation Hub](history/index.md) | [Completed Phases](development/completed-phases.md) |
| I am continuing automated repository work or a new chat | [New Chat Handoff](NEW-CHAT-HANDOFF.md) | [Agent Workflow Rules](../AGENTS.md), [CURRENT](CURRENT.md) |

For authentic product pictures and GitHub publication/licensing status, see the [screenshot checklist](screenshots.md) and [publication readiness](publication-readiness.md).

### How the documentation is arranged

| Area | What belongs there |
| --- | --- |
| [Project Overview](project-overview.md), [Project Principles](project-principles.md) | Understand the product without implementation detail |
| [Architecture](architecture/index.md) and [ADRs](adr/index.md) | Implemented boundaries, target design and accepted technical decisions |
| [API](../api/README.md) | Public client/developer contracts; OpenAPI coverage is explicitly partial |
| [Development](development/index.md) | Developer workflow, source-backed implementation contracts, technical closeouts |
| [Planning](planning/index.md) | Phase ordering, prerequisites, Golden User Journeys and open gaps |
| [History](history/index.md) | Original acceptance evidence, retired plans and preserved snapshots |

**Important:** A merged source change is not proof that a running server or TV has been updated. For live progress and verified acceptance use [CURRENT](CURRENT.md), for executable sequence use the [Strict Roadmap](planning/roadmap.md), and for installed-device truth check the device itself. Older closeouts are evidence of what was true **at their own checkpoint**, not instructions to restart a phase.

## Further reading — technical reference shelves

The sections below are deliberately for deeper investigation. They are **not** a required reading list for newcomers.

### Architecture

- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Current Architecture State](development/current-architecture-state.md)
- [ADR-0059 VDR-Native Recording Editing](adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)
- [ADR-0058 Media Home](adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR-0054 Broadcast Companion Services](adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0056 Playback Semantics](adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [ADR-0060 Federated VDR-Suite Sharing and Reciprocal Site Trust](adr/ADR-0060-federated-vdr-suite-sharing-reciprocal-site-trust.md)
- [ADR-0061 Actor Permissions, Federation and Client Access](adr/ADR-0061-actor-permissions-federation-client-access.md)
- [ADR-0062 First-Party Living-Room Output Client](adr/ADR-0062-first-party-living-room-output-client.md)
- [Architecture Decision Records](adr/index.md)

### Repository-area developer entrypoints

- [Core domains and internal runtime owners](../core/README.md)
- [SQLite schema and sample-data boundaries](../database/README.md)
- [Install staging and packaging assets](../packaging/README.md)
- [Historical service sketches and actual Core owners](../services/README.md)
- [Repository checks and operator-tool safety](../tools/README.md)

These pages distinguish implemented code from historical plans and are not instructions to deploy or modify a running installation.

#### Reserved repository namespaces (not runtime implementations)

- [Module/add-on placeholders](../modules/README.md) — `image`, `music`, `rectools` and `tvscraper` are reserved directories, not installable modules in this audited tree.
- [Legacy compatibility namespace](../legacy/README.md) — historical organization placeholder, not an implementation owner.
- [UI namespace](../ui/README.md) — reserved `web`/`osd` paths, not the built web frontend or native OSD.
- [Database migration placeholder](../database/migrations/README.md) and [job-service placeholder](../services/jobs/README.md) — no runner or deployable worker in these paths.


### Development and history

- [Development Documentation](development/index.md)
- [Completed Phases](development/completed-phases.md)
- [Latest Completed Marker](development/completed-phases-latest.md)
- [Completed Phase Archive](development/completed-phases/README.md)
- [Phase 69 Closeout](development/phase-69-closeout.md)
- [Post-Phase-66 Home Rebuild Closeout](development/post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Recording Detail Closeout](development/post-phase66-recording-detail-closeout.md)
- [Phase 67 Closeout](development/phase-67-closeout.md)
- [Phase 67 Teletext Closeout](development/phase-67-teletext-closeout.md)
- [Phase 66 Closeout](development/phase-66-closeout.md)
- [Post-Phase-66 Native Recording Editing Closeout](development/post-phase66-recording-editing-closeout.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](development/phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Developer Onboarding](development/developer-onboarding.md)
- [Build System State](development/build-system-state.md)
- [Documentation Standards and authority rules](development/documentation-standards.md)
- [Build Requirements](build-requirements.md) — source-linked, not a certified OS matrix
- [Dependencies](dependencies.md) — preserved early sketch with current source pointers
- [Development Milestone History](development/milestones.md) and [Planning Milestone History](planning/milestones.md) — archived-status context, not active phase authority

### Planning

- [Planning Documentation](planning/index.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Golden User Journeys](planning/golden-user-journeys.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Platform Productization Roadmap](planning/platform-productization-roadmap.md)
- [Repository Truth Refresh 2026-07 Archive](planning/history/repository-truth-refresh-2026-07/README.md) — historical pre-refresh snapshots retained for traceability only.

## Status model

- **CURRENT** — volatile operational truth.
- **ROADMAP** — binding dependency/order and phase gates.
- **ARCHITECTURE** — stable ownership/identity/contract boundaries.
- **COMPLETED** — historical implementation evidence for closed work.
- **POST-PHASE HARDENING** — bounded non-numbered correctness/performance/product work that does not reopen a completed phase.
- **PLANNED** — genuinely open work with explicit prerequisites.
- **HISTORICAL** — traceability that is not a current work prompt.

Historical closeouts may retain exact SHAs, CI runs and wording that was true at the time they closed. Current operational interpretation comes from `CURRENT.md` and the Strict Roadmap.

## Back

- [Back to README](../README.md)
- [Back to Current State](CURRENT.md)

## Publication and licensing review

- [GPLv3 compatibility, provenance and copyright clearance (open review)](gplv3-compatibility-audit.md)
- [Publication readiness and remaining owner actions](publication-readiness.md)
