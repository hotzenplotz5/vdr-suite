# ADR-0036: TVScraper Recording Metadata Integration Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Roadmap](../planning/roadmap.md)
- [TVScraper and Recording Metadata Roadmap](../planning/tvscraper-recording-metadata-roadmap.md)

---

## Status

Accepted

---

## Context

VDR-Suite already has foundations for recording metadata, person metadata, character search, genre and content classification. Real VDR validation has shown that TVScraper-derived actor and character metadata can be surfaced through the RESTfulAPI recording payload and mapped into VDR-Suite domain objects.

However, TVScraper and similar plugins are not just passive data files. They are external VDR plugin-owned metadata systems with their own storage, update behavior, artwork handling and field semantics.

VDR-Suite must avoid two failure modes:

- silently ignoring TVScraper and other recording metadata sources in the roadmap
- reimplementing TVScraper, scraper2vdr or other backend-specific metadata systems inside VDR-Suite

The multi-backend direction also means that not every backend will expose the same metadata features. A backend may provide EPG-only metadata, TVScraper metadata, scraper2vdr metadata, artwork, cast data, genre data, ratings, external IDs or none of these.

---

## Decision

VDR-Suite treats TVScraper and recording metadata as a provider-backed, capability-advertised metadata domain.

VDR-Suite does not reimplement TVScraper. TVScraper remains the authoritative source for TVScraper-owned metadata when the plugin is installed and exposed by the backend.

VDR-Suite normalizes metadata into backend-neutral domain objects for API, search, recommendation and frontend use.

The provider model from ADR-0025 remains the base decision. This ADR narrows the runtime and roadmap rule for recording metadata:

```text
RecordingMetadataProvider
├─ EpgOnlyRecordingMetadataProvider
├─ TvscraperRecordingMetadataProvider
├─ Scraper2VdrRecordingMetadataProvider
└─ SuiteRecordingMetadataDbProvider
```

Provider-backed metadata is exposed only through explicit capabilities.

Example capability categories:

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

If a backend does not advertise a capability, VDR-Suite must not assume the feature exists. The API and frontend may still provide EPG-only fallback behavior.

---

## Rules

- TVScraper is an integration source, not a subsystem to clone.
- VDR-Suite must not duplicate TVScraper's internal update, scraping or artwork ownership logic unless a later ADR explicitly accepts a suite-owned metadata provider.
- VDR-Suite may cache normalized metadata for performance, but cache ownership must be explicit and invalidatable.
- Metadata features must be backend-scoped.
- Metadata features must be capability-advertised.
- Unknown backend metadata features must be treated as unavailable, not guessed.
- EPG-only installations remain supported.
- TVScraper-specific storage and payload quirks stay behind provider boundaries.
- API and frontend layers consume normalized metadata contracts, not TVScraper-specific records.
- Recording metadata must remain read-only until a later dedicated metadata-write ADR or phase explicitly opens mutation.

---

## Consequences

VDR-Suite can offer a rich modern frontend over TVScraper data without becoming TVScraper.

The same frontend can degrade cleanly for backends that only expose EPG data.

The roadmap must include a visible recording metadata milestone so that TVScraper, artwork, cast, character, genre, rating and external-ID support do not disappear behind SearchTimer and Live parity work.

The capability matrix becomes the deciding layer for cross-backend feature availability.

This also supports future recommendation and content graph work because metadata origin, evidence and backend support remain explicit.

---

## Related Documents

- [ADR-0025: Configurable Metadata Provider Architecture](ADR-0025-configurable-metadata-provider-architecture.md)
- [ADR-0028: Content Classification Architecture](ADR-0028-content-classification-architecture.md)
- [ADR-0031: Person Catalog and External Filmography Architecture](ADR-0031-person-catalog-and-external-filmography.md)
- [Roadmap](../planning/roadmap.md)
- [TVScraper and Recording Metadata Roadmap](../planning/tvscraper-recording-metadata-roadmap.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
