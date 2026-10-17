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

### Screenshots — Mitmachen erwünscht / capture slots

**The following are text placeholders, not actual screenshots or rendered UI.** They show where real photographs/captures will be included once available.

| Web Home / Now & Next | Web recording browser | Hisense VIDAA television |
| --- | --- | --- |
| **📷 Screenshot fehlt** — Home screen | **📷 Screenshot fehlt** — Recording list / details | **📷 Foto fehlt** — App running on a real TV |
| [Capture instructions](docs/screenshots.md#1-web-home-now--next) | [Capture instructions](docs/screenshots.md#2-web-recordings) | [Capture instructions](docs/screenshots.md#3-hisense-vidaa-tv) |

You can help replace these slots with genuine captures. Please redact private data and confirm any visible programme artwork can be shared. Provide the captures to the project maintainer; they will be reviewed before adding them to the README. **No placeholder represents tested playback, audio or subtitle support.**

## How the pieces fit

```text
Web / TV / external clients
   -> VDR-Suite API and authenticated streaming gateway
   -> Suite daemon: identity, authorization, models, orchestration
   -> Backend Agent and explicitly owned private providers
   -> VDR native runtime
```

VDR remains authoritative for native devices, schedules, timers, recordings and OSD. Plugin access (RESTfulAPI, SVDRP, Streamdev, SuiteBridge, TVScraper) stays behind adapter boundaries; it does not automatically become a public contract.

## Debian packages and the Backend Agent

The experimental Debian/Ubuntu source package currently produces **three separate binary packages**. They serve different roles; building all three does **not** mean that every installation must run three services.

| Package | Role |
| --- | --- |
| `vdr-suite-daemon` | Central Control Plane, API, authentication and Web UI |
| `vdr-plugin-suitebridge` | ABI-specific native plugin loaded by VDR; its package must match the installed VDR ABI |
| `vdr-suite-backend-agent` | Separate outbound connector for a VDR backend/site: enrollment, backend identity, health/observations and capability-gated native commands |

**Single VDR host:** The daemon and native VDR integration can share one machine. A *separately running* Backend Agent is **not a blanket installation requirement** for a single-host setup; whether the selected deployment mode uses an embedded/local backend or a separate agent must be verified for that release. Do not enable the Agent just because its `.deb` was built.

**Multiple VDR machines or sites:** A configured Backend Agent on a backend host initiates an authenticated **outbound HTTPS** connection to the central Control Plane. Clients connect to the Control Plane, not to public SVDRP/RESTfulAPI/SuiteBridge endpoints at each site. The Agent is not a second central database or a Web UI.

The agent package includes `/usr/sbin/vdr-suite-backend-agent`, enrollment and administration tools, `/etc/vdr-suite/backend-agent.conf` and `vdr-suite-backend-agent.service`. The example configuration points to `https://control-plane.example.invalid` and **cannot connect as shipped**; controlled enrollment, a real HTTPS origin and protected local identity storage are required. Package builds do not enroll an Agent, enable its service, migrate the live SQLite database or authorize a production installation.

**Packaging release gate (17 October 2026):** Noble and Resolute three-package builds and isolated lifecycle/upgrade checks succeeded in [CI run 38970428064](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38970428064). A provisional [Debian copyright inventory](debian/copyright) now documents the TVScraper exception and unresolved artwork licensing; it does **not** constitute rights clearance. The three prior Lintian `no-copyright-file` errors require verification on freshly rebuilt packages. **No PPA upload or takeover of a manually installed yaVDR is authorized.** [Full PPA gates](docs/planning/packaging-launchpad-ppa.md).

**Details:** [Agent package contents, configuration, enrollment and deployment boundaries](debian/README.md#backend-agent-package-roles-and-deployment) · [Multi-site trust boundary](docs/adr/ADR-0039-backend-agent-control-plane-boundary.md) · [Packaging/release status](packaging/README.md).

## For developers

Start with the [clean-checkout quickstart](docs/development/quickstart.md), then [Developer Onboarding](docs/development/developer-onboarding.md), the [current architecture](docs/development/current-architecture-state.md), [ADRs](docs/adr/index.md) and [source-tested API](api/README.md). The repository's detailed rules for automated changes are in [AGENTS](AGENTS.md), not prerequisites to *reading* or *trying* the project.

Phases 64–69 are completed within their historically accepted scopes. The next strict numbered phase is Phase 70; [current status](docs/CURRENT.md) and [roadmap](docs/planning/roadmap.md) distinguish later R2 work from original Phase-69.F compatibility acceptance.

## Project status and decision sources

[CURRENT](docs/CURRENT.md) is the **sole repository authority for volatile operational status**; always verify current GitHub refs and the installed runtime separately. [New Chat Handoff](docs/NEW-CHAT-HANDOFF.md) explains how to resume active repository work without mistaking historical checkpoints for current implementation.

**A chat discussion is not a binding VDR-Suite project decision** until represented by the appropriate repository ADR, roadmap, status or accepted implementation contract.

## License and publication status

The maintainer has selected **`GPL-3.0-or-later` for original VDR-Suite code they are authorized to license**, subject to the documented provenance boundaries; see [LICENSE](LICENSE). See the [GPLv3 compatibility and copyright review](docs/gplv3-compatibility-audit.md) and [publication readiness](docs/publication-readiness.md), also covering GitHub About metadata and verified product screenshots. Publicly readable source code is not by itself a redistribution permission.
