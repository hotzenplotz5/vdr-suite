# VDR-Suite Roadmap

## Navigation

- [README](README.md)
- [Documentation Index](docs/index.md)
- [Current State](docs/CURRENT.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Implementation Dependency Map](docs/planning/implementation-dependency-map.md)
- [Phase 61 and Performance Closeout](docs/development/phase-61-metadata-genre-performance-closeout.md)
- [VDR Ecosystem Parity](docs/planning/parity-audit-and-frontend-gap-roadmap.md)

---

## Purpose

This root roadmap is the compact repository entry point. The authoritative execution order and exit criteria live in [docs/planning/roadmap.md](docs/planning/roadmap.md).

Completed implementation history is maintained in:

- [Completed Phases](docs/development/completed-phases.md)
- [Completed Phases Latest Marker](docs/development/completed-phases-latest.md)
- [Phase 61 and Performance Closeout](docs/development/phase-61-metadata-genre-performance-closeout.md)

---

## Current Roadmap Position

```text
Latest completed runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening block:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next runtime implementation phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 61 is no longer a feature branch or pending acceptance item. PR #100 is merged and the metadata-backed Genre runtime was verified on the real VDR-Suite installation. PRs #102 through #108 completed the associated EPG and metadata performance hardening.

---

## Strict Forward Sequence

```text
Phase 62 - Identity, RBAC and Accountability Foundation
Phase 63 - Backend Agent and Secure Multi-Site Runtime
Phase 64 - Timer Intent and Multi-Backend Orchestration
Phase 65 - Streaming Gateway and Media Sessions
Phase 66 - Legacy OSD Compatibility Bridge
Phase 67 - Public API and Client Compatibility Hardening
Phase 68 - Recommendation and Content Knowledge Graph
```

The accepted target architecture, domain dependency map and implementation dependency map remain mandatory prerequisites. Later phases may not bypass authorization, accountability, generation fencing or stable identity by moving policy into the frontend, a plugin or an external provider.

---

## Strategic Pillars

### Multi-Backend VDR Federation

One Suite API layer should expose several VDR systems while preserving backend identity, capabilities, health, permissions and native ownership.

### Content Intelligence

Metadata, Genres, people, artwork and later recommendations must remain provider neutral and provenance aware. Phase 61 established the first accepted persistent vertical slice.

### Client Platform

Web, desktop, mobile and TV clients should consume stable Suite-owned contracts without learning VDR filesystem paths or private RESTfulAPI, SVDRP, Streamdev, TVScraper or SuiteBridge details.

---

## Roadmap Rule

Work is read from top to bottom in the strict roadmap. Completed phases are not reopened merely because optional extensions remain possible; deferred extensions must be assigned explicitly to a later phase or backlog.