# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)

---

## Purpose

This roadmap describes the forward direction of VDR-Suite.

Current project status belongs to:

- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)

Completed implementation history belongs to:

- [Completed Phases](../development/completed-phases.md)

---

## Roadmap Principles

VDR-Suite remains:

- VDR-centered
- backend-neutral
- daemon-driven
- snapshot-oriented
- service-oriented
- prepared for multi-VDR environments
- suitable for multiple future clients and integrations

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

VDR-Suite should prefer selective backend queries over full-domain transfers whenever possible.

The runtime performance target is backend workload comparable to established VDR frontends such as `live` when equivalent user-visible information is requested.

---

## Current Position

```text
Completed implementation state
Phase 46.6 - Genre Resolution Localization JSON

Next implementation step
Phase 46.7 - Genre Architecture Documentation
```

Phase 45 completed the EPG search API block:

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

Phase 46.0 added ADR-0028 as the content classification architecture decision.

ADR-0028 defines that genre must not be modeled as a single plain string. Content classification is source-aware evidence for genre, content rating, keywords, collections, user tags and folder hints.

---

## Implemented Foundation Summary

Implemented foundation areas include:

- multi-backend polling and runtime context foundation
- multi-backend snapshot read visibility
- snapshot change feed validation
- live transport foundation
- selective backend query contracts
- heavy-domain EPG refresh policy
- selective EPG REST API boundary
- recording query API foundation
- backend-aware recording identity
- recording action runtime hardening and upstream diagnostics
- additive EPG search API over selective EPG windows
- content classification ADR for source-aware genre and rating work

Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md).

---

## Strategic Roadmap Pillars

The next roadmap is organized around three pillars:

```text
1. Multi-Backend VDR Federation
2. Content Intelligence
3. Client Platform
```

### Multi-Backend VDR Federation

Goal:

Expose several VDR systems through one coherent API while preserving backend identity, capability state and permissions.

Important future scenarios:

- one frontend for several VDR servers
- read-only and read-write backends
- partner or household installations with multiple TVs
- global views over recordings, EPG, timers and later streams

### Content Intelligence

Goal:

Build source-aware classification, metadata and search foundations above VDR-owned state.

Important future scenarios:

- multi-source genres
- content ratings such as FSK
- user tags and manual overrides
- tvscraper, TMDb, TVDb and IMDb enrichment
- SearchTimer constraints using classification and metadata

### Client Platform

Goal:

Expose stable, frontend-independent APIs for web, desktop, mobile and TV clients.

Important future scenarios:

- Web frontend
- Windows frontend
- Android / iOS frontends
- Hisense / VIDAA TV frontend evaluation
- TV profile behavior
- streaming and playback APIs

---

## Phase 46.x - Content Classification and Genre Foundation

Goal:

Implement the first domain foundation based on ADR-0028.

Planned direction:

- define source-aware genre/domain objects
- implement the first isolated genre classification test
- resolve a primary genre from source-aware evidence while preserving all evidence
- expose genre resolution through a JSON contract with canonical IDs and source evidence
- map multilingual genre labels to stable canonical genre IDs before localization
- provide localized labels for canonical genre IDs without changing the stable IDs
- expose localized genre labels in JSON while preserving canonical IDs and source evidence
- preserve original provider values
- define normalized genre values for search and grouping
- support multiple genre facts per content item
- prepare content rating as a classification type
- keep classification separate from policy enforcement
- avoid persistent full EPG mirroring

Initial sources to keep open:

```text
EPG / DVB
tvscraper
scraper2vdr
TMDb
TVDb
IMDb
User
Folder / Path
Derived
```

Architecture rule:

```text
Content classification is enrichment evidence.
VDR remains source of truth for VDR-owned state.
```

---

## Phase 47.x - SearchTimer Foundation

Goal:

Define SearchTimer support as backend-neutral and capability-based architecture.

Planned direction:

- detect backend SearchTimer capabilities
- support plugin-specific behavior through adapters
- keep SearchTimer contracts separate from generic EPG reads
- avoid making EPG persistence a prerequisite
- preserve compatibility with backends that do not support SearchTimers
- separate SearchTimer definition, validation, execution and result views
- prepare genre and content-rating constraints once classification exists

Architecture rule:

```text
SearchTimer API -> capability-aware service -> backend/plugin adapter
```

SearchTimer support must be optional. It must not make EPG persistence mandatory and must not weaken the backend-neutral EPG query boundary.

---

## Phase 48.x - Multi-Backend Unified Search

Goal:

Search across recordings, selective EPG windows, metadata and classification while preserving backend identity.

Planned direction:

- search across multiple VDR backends
- show backend origin for every result
- combine recording and EPG search views where useful
- avoid unsafe implicit merging of backend namespaces
- keep policy filtering possible before rendering results
- prepare unified search for TV frontends

Example target:

```text
Search: Tatort

Backend A: live EPG event today
Backend B: existing recording
Backend C: timer candidate tomorrow
```

Architecture rule:

Unified search may aggregate results, but it must not hide backend origin or replace VDR-owned state.

---

## Phase 49.x - User Profiles, Policy and Content Rating

Goal:

Introduce profile-aware visibility and action policy above the multi-backend and classification foundations.

Planned direction:

- define user/profile model
- define backend permissions per profile
- define action permissions for delete, move, timer creation and streaming
- define content rating visibility rules
- prepare FSK-style filtering
- keep classification separate from policy decisions
- keep policy enforcement above backend capability checks

Example target:

```text
Profile: child-15
Allowed: FSK 12 and below
Hidden: FSK 16 and FSK 18
```

Architecture rule:

Classification answers what content is. Policy answers whether a profile may see, stream or modify it.

---

## Phase 50.x - Streaming API Foundation

Goal:

Prepare live TV and recording playback through stable VDR-Suite APIs.

Planned direction:

- expose stream capabilities per backend
- keep stream provider details behind adapters
- support live channel stream references
- support recording stream references
- evaluate HLS and MPEG-TS delivery strategies
- keep playback authorization policy-ready
- keep streaming optional per backend capability

Potential providers:

```text
streamdev
live
RESTfulAPI extension
future native adapter
```

Architecture rule:

VDR-Suite should expose stream intent and capability-aware stream references. Backend-specific stream mechanics stay behind provider boundaries.

---

## Phase 51.x - TV Frontend Strategy

Goal:

Define how TV clients consume VDR-Suite without coupling backend architecture to a single frontend technology.

Planned direction:

- define TV-friendly API expectations
- evaluate browser-based TV frontend first
- evaluate native Hisense / VIDAA feasibility separately
- keep remote-control navigation constraints visible
- prepare profile selection and household-device behavior
- keep all client behavior API-first

Architecture rule:

A TV frontend is a client of VDR-Suite APIs. It must not depend on local VDR filesystem paths, RESTfulAPI URLs or backend-specific plugin details.

---

## Phase 52.x - Rectools Integration Layer

Goal:

Integrate VDR-Rectools as a media-processing and workflow companion without making it the metadata source of truth.

Planned direction:

- keep Rectools focused on processing workflows
- allow Rectools to consume resolved metadata and classification
- avoid making Rectools a VDR-Suite metadata importer
- integrate jobs, actions and workflow state only through explicit boundaries
- preserve package cleanliness for Debian/Ubuntu systems

Architecture rule:

```text
Rectools processes media.
VDR-Suite manages orchestration, metadata views and client APIs.
```

---

## Future Product Layers

Possible later layers:

- Web frontend
- Windows frontend
- Android frontend
- iOS frontend
- TV frontend
- OSD integration
- stream provider integration
- media library expansion
- authentication and authorization
- permission-aware backend federation
- content rating and household profile policy

---

## Roadmap Rule

This file describes direction, not detailed implementation history.

Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md).

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
