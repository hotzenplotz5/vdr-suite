# VDR-Suite Roadmap

## Navigation

* [README](README.md)
* [Documentation Index](docs/index.md)
* [Project Overview](docs/project-overview.md)
* [Planning Index](docs/planning/index.md)
* [Active Roadmap](docs/planning/roadmap.md)

---

## Purpose

This root roadmap is a repository entry point.

The canonical and detailed roadmap is maintained in:

* [docs/planning/roadmap.md](docs/planning/roadmap.md)

Completed implementation history is maintained in:

* [docs/development/completed-phases.md](docs/development/completed-phases.md)

Current status is maintained in:

* [docs/development/current-status.md](docs/development/current-status.md)
* [docs/project-status-dashboard.md](docs/project-status-dashboard.md)

---

## Current Roadmap Position

```text
Latest completed phase:
Phase 46.0 - Content Classification Architecture ADR

Next implementation focus:
Phase 46.1 - Genre Domain Foundation
```

The Phase 45 EPG search block is complete:

```text
45.1 EPG Search Request
45.2 EPG Search Matcher
45.3 EPG Search Result
45.4 EPG Search JSON Contract
45.5 EPG Search Service
45.6 EPG Search Controller
45.7 EPG Search REST Validation
45.8 EPG Search API Documentation
```

Phase 46.0 added [ADR-0028: Content Classification Architecture](docs/adr/ADR-0028-content-classification-architecture.md).

---

## Strategic Roadmap Pillars

VDR-Suite now tracks its forward direction around three pillars:

```text
1. Multi-Backend VDR Federation
2. Content Intelligence
3. Client Platform
```

### Multi-Backend VDR Federation

One API layer should eventually expose several VDR systems while preserving backend identity, backend capabilities and backend permissions.

Target scenarios include:

* multiple VDR servers
* read-only and read-write backends
* backend-aware recordings, timers, EPG and streams
* household or partner installations with several TVs

### Content Intelligence

Content classification, metadata and search should build on source-aware evidence instead of single flat strings.

Target scenarios include:

* DVB / EPG genres
* tvscraper genres
* TMDb / TVDb / IMDb enrichment
* user-defined genres and tags
* folder/path-derived classifications
* content ratings such as FSK
* future profile and policy decisions

### Client Platform

Future clients should consume stable VDR-Suite APIs and must not depend on VDR filesystem paths, RESTfulAPI-specific URLs or backend plugin details.

Target clients include:

* Web frontend
* Windows frontend
* Android / iOS frontends
* TV frontend
* possible Hisense / VIDAA evaluation

---

## Planned Major Direction

The active detailed roadmap currently points toward:

```text
Phase 46.x - Content Classification and Genre Foundation
Phase 47.x - SearchTimer Foundation
Phase 48.x - Multi-Backend Unified Search
Phase 49.x - User Profiles, Policy and Content Rating
Phase 50.x - Streaming API Foundation
Phase 51.x - TV Frontend Strategy
Phase 52.x - Rectools Integration Layer
```

---

## Roadmap Rule

This file is intentionally short.

The detailed roadmap belongs in [docs/planning/roadmap.md](docs/planning/roadmap.md).

This root file should only summarize the current direction and link to the canonical roadmap documents.

---

## Back

* [Back to README](README.md)
