# ADR-0025: Configurable Metadata Provider Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Accepted

## Context

VDR-Suite needs metadata for EPG events, timers, recordings, user-facing recording views, artwork references, search, and later workflow decisions.

The metadata architecture must support different installations:

- plain VDR installations with only EPG information
- installations using tvscraper
- installations using scraper2vdr
- installations that want VDR-Suite to keep its own metadata cache or database
- installations that want to disable external metadata enrichment

VDR-Suite must not hard-code a single metadata source.

VDR-Suite already uses SQLite as a long-term metadata, artwork, job, search index, and service-data store. This does not mean that every metadata source must be imported by VDR-Suite. External metadata providers may remain the authoritative source for their own data.

Rectools is not a metadata import component for VDR-Suite. Rectools may be used for recording import and shrink workflows, and may consume metadata for naming or export decisions, but it must not become the metadata source of truth.

## Decision

VDR-Suite uses a configurable metadata provider architecture.

The EPG layer is the base layer for metadata resolution. Metadata enrichment must build on top of EPG events, timers, recordings, and recording identity instead of bypassing them.

Metadata access is modeled as provider-based integration.

Initial provider categories:

```text
MetadataProvider
├─ EpgOnlyProvider
├─ TvscraperProvider
├─ Scraper2VdrProvider
└─ SuiteMetadataDbProvider
```

The active provider is selected through configuration.

Example configuration model:

```text
metadata.provider = epg-only
metadata.provider = tvscraper
metadata.provider = scraper2vdr
metadata.provider = suite-db
```

The provider boundary is responsible for reading and normalizing metadata. The UI and API must consume normalized metadata objects and must not depend on provider-specific storage details.

## Rules

- EPG is the base domain for metadata resolution.
- Metadata enrichment is configurable.
- VDR-Suite must not hard-code tvscraper or scraper2vdr as the only source.
- VDR-Suite may use SQLite for its own metadata cache or suite-owned metadata.
- Existing plugin metadata stores may remain external and authoritative.
- Rectools must not import metadata for VDR-Suite.
- Rectools may consume already resolved metadata for import, shrink, naming, or export workflows.
- Provider-specific details must stay behind the metadata provider boundary.
- API and UI layers consume normalized metadata, not plugin-specific records.

## Consequences

VDR-Suite can support simple and advanced installations without forcing a single metadata strategy.

Installations without tvscraper or scraper2vdr can still use EPG-only metadata.

Installations with tvscraper or scraper2vdr can integrate those plugins without duplicating all metadata into VDR-Suite.

Installations that need suite-owned metadata can use the SuiteMetadataDb provider backed by SQLite.

The architecture keeps metadata separate from recording processing. Rectools remains a processing tool for import and shrink workflows and does not become a metadata import subsystem.

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
