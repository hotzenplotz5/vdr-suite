# TVScraper and Recording Metadata Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)

---

## Purpose

This document keeps TVScraper, recording metadata and metadata-provider work visible in the roadmap.

The strategic rule is:

```text
Integrate plugin-owned metadata through provider and capability boundaries.
Do not reimplement TVScraper inside VDR-Suite.
```

---

## Position in the Roadmap

TVScraper and recording metadata work belongs after the current SearchTimer and Live parity stabilization work, but before recommendation and content-graph features become primary implementation goals.

Recommended order:

```text
55.4c/55.4e - Daemon build/runtime/shutdown stabilization
55.4d       - Shared JSON decoder cleanup
55.5        - Native EPGSearch preview mode
55.6        - Live recording operations audit
56.x        - Backend capability matrix and read-only/write policy visibility
57.x        - Multi-backend permissions and backend administration
58.x        - Live parity for everyday frontend usage
59.x        - TVScraper and recording metadata provider integration
60.x        - Recommendation and content knowledge graph foundations
```

---

## Phase 59 - TVScraper and Recording Metadata Provider Integration

Status: Planned.

Goal:
- Turn TVScraper-derived and other provider-derived recording metadata into a complete backend-aware API surface without cloning TVScraper.

Expected outcomes:
- Recording metadata capability model.
- Backend-scoped metadata provider registry.
- TVScraper provider boundary.
- EPG-only fallback provider.
- Normalized recording metadata aggregate.
- Cast, character, director, writer and guest mapping audit.
- Genre, rating, keywords and external-id mapping audit.
- Artwork and poster reference model.
- Recording metadata refresh diagnostics.
- Read-only API for enriched recording metadata.
- Frontend-ready response contracts.

---

## Provider Strategy

VDR-Suite uses provider-backed metadata resolution.

```text
RecordingMetadataService
  -> RecordingMetadataProviderRegistry
       -> EpgOnlyRecordingMetadataProvider
       -> TvscraperRecordingMetadataProvider
       -> Scraper2VdrRecordingMetadataProvider
       -> SuiteRecordingMetadataDbProvider
```

The provider registry is backend-scoped.

A backend can expose:

```text
metadata.recording.basic
metadata.recording.people
metadata.recording.characters
metadata.recording.genre
metadata.recording.rating
metadata.recording.artwork
metadata.recording.externalIds
metadata.recording.provider.tvscraper
metadata.recording.provider.scraper2vdr
```

If a capability is not advertised, VDR-Suite does not guess.

---

## Boundaries

VDR-Suite must not:

- scrape external metadata directly as part of TVScraper integration
- write into TVScraper-owned storage
- assume TVScraper is installed on every backend
- require TVScraper for basic recording views
- expose TVScraper-specific records directly to the frontend

VDR-Suite may:

- read TVScraper-derived data exposed by RESTfulAPI or another backend adapter
- normalize provider-specific metadata into stable suite domain objects
- cache normalized metadata with explicit invalidation
- provide EPG-only fallback data
- expose capability-specific frontend fields

---

## Relation to Existing Foundations

Existing completed foundations already cover part of this direction:

- Person metadata domain.
- Recording-person search.
- Recording-character search.
- Content classification foundation.
- Genre and rating groundwork.
- TVScraper actor and character metadata validation through real VDR payloads.

The planned Phase 59 does not restart this work. It consolidates it into a full recording metadata provider milestone with explicit TVScraper visibility.

---

## Back

- [Back to Planning Index](index.md)
- [Back to Roadmap](roadmap.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
