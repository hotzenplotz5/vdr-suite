# ADR-0038: Suite Metadata Database and External Provider Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Roadmap](../planning/roadmap.md)
- [Recording Metadata Roadmap](../planning/tvscraper-recording-metadata-roadmap.md)

---

## Status

Accepted

Date: 2026-07-15

---

## Context

VDR-Suite needs rich recording, EPG and media metadata for a modern frontend, cross-backend browsing, people and character navigation, search, recommendations and future content graph features.

Metadata may originate from different systems:

- native VDR EPG data
- TVScraper
- scraper2vdr
- external movie and television catalogs
- imported sidecar metadata
- manual user assignments
- a suite-owned metadata database

Not every backend exposes the same provider set. A backend may provide rich plugin metadata, EPG-only metadata or no enriched metadata at all.

VDR-Suite must avoid two failure modes:

1. exposing provider-specific payloads and storage details as the public contract
2. reimplementing scraper acquisition, matching and artwork logic when a mature provider already solves it

The source audit of TVScraper, scraper2vdr and epgd also showed that provider identity, provenance, confidence and artwork ownership must be explicit. Local plugin paths and plugin-global result state are not suitable public identities or client contracts.

---

## Decision

VDR-Suite will maintain a suite-owned normalized metadata database while keeping metadata acquisition provider-based.

The suite metadata database is the stable internal integration, cache, indexing and cross-backend normalization layer. It is not itself required to scrape external services.

Provider categories include:

```text
MetadataProvider
├─ EpgMetadataProvider
├─ PluginBackedMetadataProvider
│  ├─ TvscraperMetadataProvider
│  ├─ Scraper2VdrMetadataProvider
│  └─ GenericVdrPluginMetadataProvider
├─ ExternalCatalogMetadataProvider
├─ SidecarMetadataProvider
├─ ManualMetadataProvider
└─ SuiteMetadataDbProvider
```

Provider names are architectural categories, not commitments to one concrete external service.

Multiple providers may contribute evidence for the same entity. VDR-Suite must not be limited to one globally active metadata provider.

The preferred flow is:

```text
provider source
  -> explicit adapter boundary
  -> immutable normalized provider result
  -> metadata resolver
  -> suite metadata entity and assignment
  -> suite metadata database
  -> stable API and frontend contract
```

---

## Normalized Domain Model

The suite model separates the metadata entity from its assignment to VDR-Suite resources.

```text
MetadataEntity
  metadataEntityId
  mediaType
  titles
  descriptions
  release data
  people
  genres
  ratings
  external IDs

MetadataAssignment
  target type
  target ID
  metadataEntityId
  provider
  evidence
  confidence
  manual flag
  created at
  created by

ArtworkAsset
  assetId
  metadataEntityId
  artwork type
  dimensions
  language
  provider
  checksum
```

Recording, timer and EPG resources reference metadata assignments. Provider-owned database rows and local image paths are not public resource identities.

---

## Provenance and Resolution

Every imported result must retain its origin.

The resolver may consider:

- provider identity
- external IDs
- source backend
- observation time
- confidence
- language
- manual overrides
- provider priority
- current entity revision

Manual assignments may lock selected fields or entity relationships against automatic replacement.

A resolver decision must remain explainable. The suite database stores the selected normalized result and enough evidence to understand why it was selected.

---

## Artwork Boundary

Artwork is exposed through suite-owned asset identities.

Public APIs return an `assetId` or a suite-controlled asset URL. They must not expose arbitrary local filesystem paths from TVScraper, scraper2vdr or another backend plugin.

Artwork ingestion must support:

- checksum validation
- size and media-type validation
- atomic publication
- provider attribution
- invalidation and refresh
- backend-independent client URLs

---

## Rules

- Prefer mature external metadata providers over reinventing scraper logic.
- Keep provider-specific storage, matching heuristics and payload quirks behind adapters.
- Normalize selected results into suite-owned domain objects.
- Preserve provider provenance, evidence and confidence.
- Support multiple contributing providers.
- Support EPG-only fallback behavior.
- Do not require TVScraper, scraper2vdr or any external catalog for basic views.
- Do not expose plugin filesystem paths as artwork identities.
- Cache ownership and invalidation must be explicit.
- Metadata features remain backend- and capability-aware.
- Write-back into provider-owned stores remains closed unless a later ADR explicitly opens it.
- VDR-Rectools may consume resolved metadata but is not the metadata source of truth.

---

## Provider Evaluation Criteria

Before implementing a suite-owned acquisition path, evaluate:

- matching quality for German and international titles
- series, season and episode recognition
- movie versus television disambiguation
- cast, character, director and writer coverage
- genre, rating, keyword and external-ID coverage
- artwork quality and licensing
- refresh and invalidation behavior
- API stability and operational requirements
- offline and cache behavior
- multi-backend normalization suitability
- attribution and usage constraints

---

## Consequences

Positive:

- stable metadata contracts across different backends
- rich metadata for metadata-poor backends
- provider replacement without frontend changes
- offline and indexed access through the suite database
- explicit provenance and confidence
- a foundation for search, recommendations and knowledge graphs

Trade-offs:

- resolver and assignment logic add complexity
- provider refresh and invalidation need explicit policies
- external provider licensing and attribution must be tracked
- duplicate and conflicting metadata require review and reconciliation tools

---

## Non-Goals

This ADR does not select a specific external catalog provider.

It does not define:

- final metadata database tables
- a complete matching algorithm
- provider credentials
- metadata write-back into VDR plugins
- recommendation algorithms
- content graph implementation

---

## Related Decisions

- [ADR-0025: Configurable Metadata Provider Architecture](ADR-0025-configurable-metadata-provider-architecture.md)
- [ADR-0028: Content Classification Architecture](ADR-0028-content-classification-architecture.md)
- [ADR-0031: Person Catalog and External Filmography Architecture](ADR-0031-person-catalog-and-external-filmography.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](ADR-0036-tvscraper-recording-metadata-integration.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
