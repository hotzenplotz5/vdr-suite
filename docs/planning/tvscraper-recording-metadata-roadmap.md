# Recording Metadata, External Providers and Suite Metadata Database Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)

---

## Purpose

This document keeps Recording metadata, plugin-backed metadata, external catalog providers, artwork and the suite-owned normalized metadata database visible in the roadmap.

The strategic rule is:

```text
Use mature external metadata providers when they solve acquisition and matching well.
Normalize selected results into VDR-Suite-owned metadata entities and assignments.
Keep provenance, evidence and confidence.
Do not make TVScraper or any single provider the whole strategy.
Do not reimplement scraper behavior without a proven gap.
```

---

## Position in the Roadmap

Recording metadata preparation begins with Phase 60.15 after the lazy Recording browser and Recording detail UX foundation.

The full suite metadata database milestone follows as Phase 61.

Recommendation and content graph work moves to Phase 62 because the 59.x and 60.x ranges already contain completed frontend implementation slices.

Recommended order:

```text
Phase 57      - Multi-site access-mode and backend permission foundation
Phase 58      - Frontend and Live-parity umbrella track
Phase 59.x    - Completed Client API and frontend module slices
Phase 60.1-14 - Completed frontend platform and Recording UX slices
Phase 60.15   - Recording metadata and poster preparation
Phase 61      - Suite metadata database and external provider integration
Phase 62      - Recommendation and content knowledge graph foundations
```

Completed implementation history is not renumbered.

---

## Phase 60.15 - Recording Metadata and Poster Preparation

Status: Completed.

Goal:

- Add provider-neutral Recording metadata and artwork hooks without destabilizing the lazy Recording browser.

Completed preparation:

- VDR-owned technical fields remain separate from provider-derived Recording metadata
- RESTfulAPI scraper metadata maps into provider-neutral movie and series/episode value types
- source-scoped artwork references remain internal cache evidence
- Recording metadata persists through the existing SQLite lazy cache and restart path
- deterministic poster placeholders preserve EPG-only and metadata-poor behavior
- Suite-owned opaque artwork IDs replace provider paths at the client boundary
- authenticated local artwork delivery supports JPEG, PNG and WebP below allowlisted roots
- lazy folder and detail loading remain unchanged and regression covered

Phase 60.15 intentionally does not implement the complete Phase 61 normalized metadata entity, assignment, provenance and provider platform.

---

## Phase 61 - Suite Metadata Database and External Provider Integration

Status: Planned next major milestone.

Goal:

- Build a backend-aware metadata layer that can combine plugin metadata, external catalog data, sidecars, manual assignments and a suite-owned normalized database.

Expected outcomes:

- metadata entity identity
- metadata assignment model
- artwork asset identity and delivery boundary
- backend-scoped metadata provider registry
- suite-owned normalized metadata persistence
- provider evidence and confidence model
- TVScraper provider boundary
- scraper2vdr provider boundary
- generic plugin-backed provider boundary
- external catalog provider boundary
- sidecar and manual provider boundaries
- EPG-only fallback
- cast, character, director, writer and guest normalization
- genre, rating, keyword and external-ID normalization
- import, refresh and invalidation pipelines
- read-only enriched metadata APIs
- frontend-ready stable contracts
- diagnostics for provider and refresh failures

---

## Provider Strategy

VDR-Suite uses provider-backed metadata resolution.

```text
MetadataService
  -> MetadataProviderRegistry
       -> EpgMetadataProvider
       -> PluginBackedMetadataProvider
            -> TvscraperMetadataProvider
            -> Scraper2VdrMetadataProvider
            -> GenericVdrPluginMetadataProvider
       -> ExternalCatalogMetadataProvider
       -> SidecarMetadataProvider
       -> ManualMetadataProvider
       -> SuiteMetadataDbProvider
  -> MetadataResolver
  -> MetadataEntity / MetadataAssignment / ArtworkAsset
```

The provider registry is backend-aware, but the suite metadata entity may be shared across backends when evidence shows that the resources refer to the same movie, series or episode.

The suite database is not a replacement for every provider. It is the normalized persistence, cache, index, audit and cross-backend consistency layer.

A plugin-backed provider is never the frontend contract. The suite domain model remains the contract for UI, search, recommendations and cross-backend behavior.

---

## Capability Direction

A backend or provider may advertise capabilities such as:

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
metadata.recording.provider.sidecar
metadata.recording.provider.suiteDb
```

If a capability is not advertised, VDR-Suite does not guess.

Capability support does not imply user permission to trigger metadata mutation or refresh.

---

## Provenance and Resolution

Provider results retain:

- provider identity
- backend identity when relevant
- external IDs
- observation time
- language
- evidence
- confidence
- provider revision
- source payload revision or fingerprint when available

The resolver may combine several providers instead of selecting one globally active provider.

Manual assignments may override or lock selected relationships.

Resolver decisions must remain explainable and auditable.

---

## Artwork Boundary

Artwork is represented through suite-owned asset identities.

```text
provider image
  -> validated import or proxy boundary
  -> ArtworkAsset
  -> assetId
  -> suite-controlled client URL
```

The frontend must not receive arbitrary local paths from TVScraper, scraper2vdr or another backend plugin.

Artwork handling must prepare for:

- media-type validation
- dimension and size limits
- checksum validation
- atomic publication
- invalidation
- attribution
- provider licensing constraints
- backend-independent URLs

---

## Provider Evaluation Before Reinvention

Before VDR-Suite implements direct acquisition or scraper behavior, evaluate:

- matching quality for German and international titles
- alias and translated-title support
- movie, series, season and episode disambiguation
- cast, characters, directors, writers and guests
- genre, rating, keywords and external IDs
- artwork and backdrop quality
- update and invalidation behavior
- API stability
- operational requirements
- licensing and attribution
- offline and cache behavior
- multi-backend normalization suitability

Only proven gaps should become suite-owned acquisition logic.

---

## Plugin-Backed Provider Boundary

The preferred flow is:

```text
VDR plugin metadata source
  -> explicit local adapter
  -> immutable provider result
  -> VDR-Suite provider
  -> metadata resolver
  -> normalized suite metadata model
  -> suite metadata database
  -> stable API and frontend contract
```

Plugin-global state, borrowed pointers, provider database rows and local file paths must not escape the adapter boundary.

A plugin may remain authoritative for its own acquisition and matching behavior. VDR-Suite remains authoritative for the normalized entities and assignments it persists.

---

## Boundaries

VDR-Suite must not:

- treat TVScraper as the only metadata strategy
- treat any plugin-backed provider as the public contract
- expose provider-specific rows directly to the frontend
- expose arbitrary provider filesystem paths as artwork IDs
- write into plugin-owned storage without a later explicit ADR
- assume every backend exposes the same metadata capabilities
- reimplement an external scraper before provider evaluation
- make VDR-Rectools the metadata source of truth

VDR-Suite may:

- read plugin-derived metadata through backend adapters
- use external catalogs through provider adapters
- import sidecar metadata
- allow manual assignments
- normalize and persist selected metadata
- cache normalized results with explicit invalidation
- provide EPG-only fallback
- expose capability-specific fields
- let VDR-Rectools consume resolved metadata for processing and export

---

## Why the Suite Metadata Database Matters

The suite database provides:

- stable cross-backend metadata identity
- provider-independent frontend contracts
- cached and offline provider results
- search and recommendation input
- artwork reference stability
- provenance and confidence tracking
- manual correction support
- enrichment for metadata-poor backends
- auditability of resolver decisions

---

## Relation to Existing Foundations

Existing foundations already cover part of this direction:

- Person metadata domain
- Recording-person search
- Recording-character search
- content classification
- genre and rating groundwork
- TVScraper-derived actor and character mapping validation
- ADR-0025 configurable metadata provider architecture
- ADR-0031 person catalog and external filmography architecture
- ADR-0036 TVScraper Recording metadata integration strategy
- ADR-0038 suite metadata database and external provider strategy

Phase 61 does not restart this work. It consolidates the existing foundations into a complete normalized metadata platform.

---

## Back

- [Back to Planning Index](index.md)
- [Back to Roadmap](roadmap.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
