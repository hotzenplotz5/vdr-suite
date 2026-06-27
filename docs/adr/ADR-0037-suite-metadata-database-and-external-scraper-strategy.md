# ADR-0037: Suite Metadata Database and External Scraper Strategy

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

---

## Context

VDR-Suite needs rich recording and media metadata for a modern frontend, cross-backend browsing, people and character navigation, recommendations and future content graph features.

Some VDR installations can expose metadata through plugins such as TVScraper or scraper2vdr. Other backends may expose only EPG data or no rich recording metadata at all.

A backend without a metadata source would make VDR-Suite visually and functionally weaker: no reliable artwork, cast, characters, external IDs, genres, ratings, keywords, collections or recommendation evidence.

At the same time, VDR-Suite should not build a custom scraper just because scraping is possible. If an existing external scraper, service or provider already solves metadata acquisition, matching, artwork handling and refresh behavior well, VDR-Suite should evaluate and reuse it through a provider boundary.

---

## Decision

VDR-Suite will build a suite-owned normalized metadata database, but it will not automatically reimplement external scraper functionality.

The suite metadata database is the stable internal integration, cache, indexing and cross-backend normalization layer.

External metadata acquisition remains provider-based.

Provider categories include:

```text
RecordingMetadataProvider
├─ EpgOnlyRecordingMetadataProvider
├─ PluginBackedRecordingMetadataProvider
│  ├─ TvscraperRecordingMetadataProvider
│  └─ Scraper2VdrRecordingMetadataProvider
├─ ExternalCatalogRecordingMetadataProvider
│  ├─ MovieDatabaseProvider
│  ├─ TvDatabaseProvider
│  └─ ProviderSpecificIdResolver
└─ SuiteRecordingMetadataDbProvider
```

The provider names above are architecture categories, not a commitment to one concrete external service or API.

VDR-Suite may use external scrapers and catalog providers when they provide better matching, richer metadata or safer artwork handling than a suite-owned implementation.

VDR-Suite must still normalize the results into suite-owned domain objects and persist selected results in the suite metadata database when this improves cross-backend availability, performance, offline access, auditability or frontend consistency.

---

## Rules

- Prefer mature external metadata providers over reinventing scraper logic.
- Evaluate provider quality before implementing a suite-owned scraper path.
- Keep provider-specific API behavior, storage quirks and matching heuristics behind provider boundaries.
- Store normalized metadata in the suite database when it is needed for cross-backend consistency, caching, search, recommendations or frontend response stability.
- Treat plugin-backed metadata and external catalog metadata as inputs, not as frontend contracts.
- Keep metadata origin, provider evidence and confidence visible in the domain model.
- Support EPG-only fallback behavior.
- Do not require TVScraper, scraper2vdr or any external catalog for basic recording views.
- Do not assume that every backend exposes the same metadata capabilities.
- Keep write-back into plugin-owned metadata stores closed unless a later ADR explicitly opens it.

---

## Provider Evaluation Criteria

Before adding a direct suite-owned scraper implementation, evaluate whether an existing provider already solves the problem.

Evaluation dimensions:

- Matching quality for German and international titles.
- Series, episode and season recognition.
- Movie versus TV event disambiguation.
- Cast, character, director, writer and guest coverage.
- Genre, rating, keyword and external ID coverage.
- Artwork, poster and backdrop handling.
- Refresh and invalidation behavior.
- API stability and operational requirements.
- Licensing, attribution and usage constraints.
- Offline/cache behavior.
- Suitability for multi-backend normalization.

---

## Consequences

VDR-Suite gets an own metadata database without becoming a scraper clone.

The frontend can depend on stable VDR-Suite metadata contracts even when the source differs by backend.

Backends with rich plugin metadata can contribute that data through provider adapters.

Backends without plugin metadata can still benefit from suite-owned normalized metadata once external catalog providers or local metadata enrichment are configured.

Future recommendation and content graph work can rely on stable metadata identity, provenance and evidence instead of one backend-specific payload shape.

---

## Related Documents

- [ADR-0025: Configurable Metadata Provider Architecture](ADR-0025-configurable-metadata-provider-architecture.md)
- [ADR-0028: Content Classification Architecture](ADR-0028-content-classification-architecture.md)
- [ADR-0031: Person Catalog and External Filmography Architecture](ADR-0031-person-catalog-and-external-filmography.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](ADR-0036-tvscraper-recording-metadata-integration.md)
- [Roadmap](../planning/roadmap.md)
- [Recording Metadata Roadmap](../planning/tvscraper-recording-metadata-roadmap.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
