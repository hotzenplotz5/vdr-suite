# VDR-Suite

![VDR-Suite logo](web/frontend/logo-vdr-suite.svg)

**A modern Web interface and control plane for [Video Disk Recorder (VDR)](https://www.tvdr.de/).** Browse TV channels and EPG, discover and manage recordings, use supported Live TV/recording playback, and connect first-party or external clients through bounded APIs. **VDR stays in charge of native recording, tuner, timer and plugin execution.**

## Start here

| I want to… | Open |
| --- | --- |
| See what VDR-Suite can do | [Feature overview](docs/project-status-dashboard.md) |
| Explore an existing server or find the installation status | [Getting started](docs/getting-started.md) |
| Compile and test from a clean checkout | [Developer quickstart](docs/development/quickstart.md) |
| Contribute a change | [Contributing](CONTRIBUTING.md) |
| Build a client | [Public API guide](api/README.md) and [partial OpenAPI](api/openapi.json) |
| Understand architecture and roadmap | [Documentation home](docs/index.md) |

**Deutsch:** [Projektüberblick](docs/project-overview.de.md) · [Dokumentationsübersicht](docs/index.md)

**Installation status:** source builds, test targets and a controlled staging/deployment procedure exist. There is **not yet a generally published one-command Ubuntu/Debian installer or a verified `.deb` release** in this repository. Do not run development deployment commands on an existing VDR without a reviewed installation and rollback plan. See [packaging status](packaging/README.md) and [staged deployment contract](docs/development/install-runtime-deployment-reliability.md).

## What you get

- **Live TV and programme guide:** Channels, EPG, metadata and playback on supported client/source combinations.
- **Recordings:** browse/search, metadata and artwork, supported playback profiles, and VDR-native editing boundaries.
- **Home and discovery:** Now/Next, recently recorded, film/genre and series browsing.
- **Recording plans:** backend-aware TimerIntent/TimerAssignment orchestration and protected VDR-native actions.
- **Secure clients:** persistent identities, server-enforced permissions, Device pairing and versioned Public-v1 contracts.
- **Multi-backend architecture:** Suite control plane and bounded Backend Agent instead of exposing plugin-provider URLs to every client.

For accepted scope versus ongoing/hardware-gated work, see the [feature dashboard](docs/project-status-dashboard.md) and [Current State](docs/CURRENT.md). **Merged source does not prove a particular machine or television is running that build.**

## Web and TV clients

The browser frontend lives in this repository under `web/frontend/`. The separate [VDR-Suite VIDAA client for Hisense televisions](https://github.com/hotzenplotz5/vdr-suite-vidaa) has its own installation, testing and real-TV acceptance status. It is not implied that an arbitrary TV already supports picture, sound, seeking or subtitles.

This README uses a **real repository logo**, not a fabricated TV screenshot. Authentic Web/Hisense interface screenshots will be added only after real capture and permission/privacy review; see the [screenshot checklist](docs/screenshots.md).

## How the pieces fit

```text
Web / TV / external clients
   -> VDR-Suite API and authenticated streaming gateway
   -> Suite daemon: identity, authorization, models, orchestration
   -> Backend Agent and explicitly owned private providers
   -> VDR native runtime
```

VDR remains authoritative for native devices, schedules, timers, recordings and OSD. Plugin access (RESTfulAPI, SVDRP, Streamdev, SuiteBridge, TVScraper) stays behind adapter boundaries; it does not automatically become a public contract.

## For developers

Start with the [clean-checkout quickstart](docs/development/quickstart.md), then [Developer Onboarding](docs/development/developer-onboarding.md), the [current architecture](docs/development/current-architecture-state.md), [ADRs](docs/adr/index.md) and [source-tested API](api/README.md). The repository's detailed rules for automated changes are in [AGENTS](AGENTS.md), not prerequisites to *reading* or *trying* the project.

Phases 64–69 are completed within their historically accepted scopes. The next strict numbered phase is Phase 70; [current status](docs/CURRENT.md) and [roadmap](docs/planning/roadmap.md) distinguish later R2 work from original Phase-69.F compatibility acceptance.

## Project status and decision sources

[CURRENT](docs/CURRENT.md) is the **sole repository authority for volatile operational status**; always verify current GitHub refs and the installed runtime separately. [New Chat Handoff](docs/NEW-CHAT-HANDOFF.md) explains how to resume active repository work without mistaking historical checkpoints for current implementation.

**A chat discussion is not a binding VDR-Suite project decision** until represented by the appropriate repository ADR, roadmap, status or accepted implementation contract.

## License and publication status

The repository's `LICENSE` file is currently empty; **no project-wide open-source license should be inferred** until the maintainer has approved and published one. This is tracked together with GitHub About metadata and verified product screenshots in [publication readiness](docs/publication-readiness.md). Publicly readable source code is not by itself a redistribution permission.
