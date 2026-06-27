# Recording Metadata, External Scrapers and Suite Metadata Database Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [ADR-0037: Suite Metadata Database and External Scraper Strategy](../adr/ADR-0037-suite-metadata-database-and-external-scraper-strategy.md)

---

## Purpose

This document keeps recording metadata, plugin-backed metadata, external scraper providers and the suite-owned metadata database visible in the roadmap.

The strategic rule is:

```text
Use mature external metadata providers when they solve the job well.
Normalize useful results into a VDR-Suite-owned metadata database.
Do not make TVScraper or any single provider the whole strategy.
Do not reimplement scraper behavior unless the provider evaluation shows a real gap.
```

---

## Position in the Roadmap

Recording metadata work belongs after the current SearchTimer and Live parity stabilization work, but before recommendation and content-graph features become primary implementation goals.

Recommended order:

```text
55.4c/55.4e - Daemon build/runtime/shutdown stabilization
55.4d       - Shared JSON decoder cleanup
55.5        - Native EPGSearch preview mode
55.6        - Live recording operations audit
56.x        - Backend capability matrix and read-only/write policy visibility
57.x        - Multi-backend permissions and backend administration
58.x        - Live parity for everyday frontend usage
59.x        - Suite metadata database and external scraper provider integration
60.x        - Recommendation and content knowledge graph foundations
```

---

## Phase 59 - Suite Metadata Database and External Scraper Provider Integration

Status: Planned.

Goal:
- Build a backend-aware recording metadata layer that can use existing plugin metadata, external catalog providers and a suite-owned normalized metadata database.

Expected outcomes:
- Recording metadata capability model.
- Backend-scoped metadata provider registry.
- Suite-owned normalized metadata database.
- Provider evaluation matrix for plugin-backed and external catalog-backed metadata.
- TVScraper provider boundary.
- scraper2vdr provider boundary.
- Generic plugin-backed metadata provider boundary for VDR plugins that expose useful metadata.
- External catalog provider boundary for movie and TV metadata sources.
- EPG-only fallback provider.
- Normalized recording metadata aggregate.
- Cast, character, director, writer and guest mapping audit.
- Genre, rating, keywords and external-id mapping audit.
- Artwork, poster and backdrop reference model.
- Metadata origin, evidence and confidence model.
- Import and refresh pipeline from plugin-backed providers into the suite metadata database.
- Fallback enrichment for backends without their own metadata database.
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
       -> PluginBackedRecordingMetadataProvider
            -> TvscraperRecordingMetadataProvider
            -> Scraper2VdrRecordingMetadataProvider
            -> GenericVdrPluginMetadataProvider
       -> ExternalCatalogRecordingMetadataProvider
            -> MovieDatabaseProvider
            -> TvDatabaseProvider
            -> ProviderSpecificIdResolver
       -> SuiteRecordingMetadataDbProvider
```

The provider registry is backend-scoped.

The suite metadata database is not a replacement for every external provider. It is the normalized persistence, cache, index and cross-backend consistency layer.

A VDR plugin can be used as a metadata helper when it already performs useful matching, artwork lookup or metadata acquisition. The plugin-backed provider reads through an explicit adapter boundary and then imports normalized results into the suite metadata database.

A plugin-backed provider is not the frontend contract. The suite domain model and the suite metadata database remain the contract for UI, search, recommendation and cross-backend behavior.

A backend or provider can expose:

```text
metadata.recording.basic
metadata.recording.people
metadata.recording.characters
metadata.recording.genre
metadata.recording.rating
metadata.recording.artwork
metadata.recording.externalIds
metadata.recording.provider.plugin
metadata.recording.provider.tvscraper
metadata.recording.provider.scraper2vdr
metadata.recording.provider.externalCatalog
metadata.recording.provider.suiteDb
```

If a capability is not advertised, VDR-Suite does not guess.

---

## Provider Evaluation Before Reinvention

Before VDR-Suite implements direct scraper behavior, it must compare existing options.

Evaluation questions:

- Does an existing plugin or external provider already solve matching better?
- Does it handle German titles, international titles and aliases reliably?
- Does it distinguish movie, series, episode and season data well?
- Does it provide cast, characters, directors, writers and guests?
- Does it provide artwork, posters, backdrops and external IDs?
- Does it have acceptable operational, licensing and attribution requirements?
- Can VDR-Suite cache or normalize the results safely?
- Does it help backends that have no own metadata database?
- Can the provider be used without tying VDR-Suite to provider-internal storage details?

Only proven gaps should become suite-owned acquisition logic.

---

## Plugin-Backed Metadata Providers

A VDR plugin may act as a metadata acquisition helper for VDR-originated content when it already solves metadata matching well.

The preferred flow is:

```text
VDR plugin metadata source
  -> explicit adapter or export boundary
  -> VDR-Suite provider
  -> normalized suite metadata model
  -> suite metadata database
  -> stable API and frontend contract
```

This keeps plugin-specific behavior behind the provider and allows VDR-Suite to reuse good existing work without making one plugin mandatory.

The provider may be useful for:

- initial enrichment of VDR recordings
- validating metadata matching quality
- importing artwork and external identifiers
- comparing plugin-derived metadata against external catalog-derived metadata
- bootstrapping suite metadata for installations that already run a mature metadata plugin

It must not become the only solution for metadata-poor non-VDR backends. Those backends still need suite database enrichment through external catalog providers or other configured metadata providers.

---

## Boundaries

VDR-Suite must not:

- treat TVScraper as the only recording metadata strategy
- treat any plugin-backed provider as the only metadata strategy
- scrape external metadata directly without a provider evaluation
- write into plugin-owned storage without a later explicit ADR
- assume any plugin or external catalog is installed or configured for every backend
- expose provider-specific records directly to the frontend

VDR-Suite may:

- read plugin-derived metadata exposed by RESTfulAPI or another backend adapter
- use mature external metadata providers when they are better than building a new scraper
- normalize provider-specific metadata into stable suite domain objects
- persist normalized metadata in the suite database
- cache normalized metadata with explicit invalidation
- provide EPG-only fallback data
- expose capability-specific frontend fields

---

## Why the Suite Metadata Database Still Matters

A backend without metadata support should not make VDR-Suite lose modern browsing and recommendation features.

The suite database provides:

- stable cross-backend metadata identity
- cached provider results
- frontend-ready normalized contracts
- search and recommendation input
- artwork reference stability
- provenance and confidence tracking
- fallback enrichment for metadata-poor backends

---

## Relation to Existing Foundations

Existing completed foundations already cover part of this direction:

- Person metadata domain.
- Recording-person search.
- Recording-character search.
- Content classification foundation.
- Genre and rating groundwork.
- TVScraper actor and character metadata validation through real VDR payloads.
- ADR-0025 configurable metadata provider architecture.
- ADR-0031 person catalog and external filmography architecture.

The planned Phase 59 does not restart this work. It consolidates it into a full recording metadata provider and suite metadata database milestone.

---

## Back

- [Back to Planning Index](index.md)
- [Back to Roadmap](roadmap.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
