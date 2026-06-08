# ADR-0005: External VDR Integration Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Accepted

## Date

2026-05-31

---

## Context

VDR-Suite is intended to become a modern backend and frontend platform
for VDR recordings, metadata management, job processing, dashboard
services and future Web, OSD and mobile clients.

During Phase 8 an analysis of existing VDR ecosystem components was
performed.

Existing solutions already provide substantial functionality:

- vdr-plugin-restfulapi
- epgsearch
- scraper2vdr
- tvscraper
- vdr-rectools

The analysis showed that these components already implement:

- channel management
- EPG access
- timer management
- search timers
- remote control
- OSD access
- scraper integration
- metadata retrieval
- recording management

Reimplementing these capabilities inside VDR-Suite would duplicate
many years of existing development effort.

---

## Decision

VDR-Suite will not attempt to replace existing VDR ecosystem
components where mature implementations already exist.

Instead VDR-Suite will act as an orchestration and integration layer.

Existing components should be reused whenever practical.

---

## Architecture

### Existing Components

VDR Core

↓

External Plugins

- vdr-plugin-restfulapi
- epgsearch
- scraper2vdr
- tvscraper
- vdr-rectools

↓

VDR-Suite

- SQLite
- Metadata Layer
- Repository Layer
- Service Layer
- Workflow Layer
- Worker Layer
- Dashboard Layer
- REST Layer
- Web UI
- OSD UI
- Mobile Clients

---

## Responsibilities

### vdr-plugin-restfulapi

Provides:

- channels
- events
- timers
- recordings
- remote control
- OSD access

### epgsearch

Provides:

- search timers
- intelligent recording rules

### scraper2vdr / tvscraper

Provides:

- artwork
- posters
- fanart
- metadata

### vdr-rectools

Provides:

- recording processing
- repair
- cut
- remux
- conversion
- maintenance operations

### VDR-Suite

Provides:

- orchestration
- workflow management
- job management
- queue processing
- metadata aggregation
- dashboards
- user interfaces
- unified user experience

---

## Consequences

Benefits:

- reduced duplication
- reduced maintenance effort
- compatibility with existing VDR installations
- faster development
- easier adoption

Trade-offs:

- dependency on existing ecosystem components
- adapter layers required for integration

---

## Future Work

Planned integration points:

- RestfulApiAdapter
- EpgSearchAdapter
- ScraperAdapter
- RectoolsAdapter

Future modules should prefer integration over replacement whenever
possible.
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
