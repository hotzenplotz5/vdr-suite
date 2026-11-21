# Genre Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [ADR-0028 Content Classification Architecture](../adr/ADR-0028-content-classification-architecture.md)

---

## Purpose

This document describes the current VDR-Suite genre architecture.

It documents the Phase 46 genre foundation and explains how multilingual provider labels are mapped to stable internal genre IDs.

---

## Core Rule

Genres are not stored or compared as display strings.

The stable internal value is a canonical genre ID.

Example:

- canonical ID: `crime`
- German label: `Krimi`
- English label: `Crime`

This prevents the same genre from appearing as separate groups such as `Krimi`, `Crime` and `Kriminalfilm`.

All of them can resolve to the same canonical ID: `crime`.

---

## Implemented Components

### GenreClassification

Represents one source-specific genre evidence item.

Fields:

- `source`
- `originalValue`
- `normalizedValue`
- `confidence`
- `providerReference`

Meaning:

- `source` identifies the origin, for example DVB, TMDb, user or folder.
- `originalValue` preserves the provider value.
- `normalizedValue` is the current canonical ID candidate.
- `confidence` is optional evidence strength.
- `providerReference` may point to provider-specific metadata.

### GenreCollection

Collects multiple genre evidence entries for one content item.

A content item can have several genre facts at once:

- DVB: `Krimi`
- TMDb: `Crime`
- User: `Lieblingsserie`
- Folder: `Tatort`

### GenreResolver

Selects a primary genre from a collection while preserving all evidence.

Rules currently implemented:

1. Prefer evidence with confidence.
2. Prefer higher confidence.
3. Use deterministic source priority as tie-breaker.
4. Never delete lower-priority evidence.

### GenreResolutionResult

Contains:

- `resolved`
- `primaryGenre`
- `evidence[]`

The primary genre is a view decision.

The evidence list remains available for transparency and future debugging.

### GenreResolutionJsonSerializer

Serializes genre resolution results.

The non-localized contract preserves stable values and provider evidence.

The localized contract adds:

- `label`
- `locale`

while keeping:

- `canonicalId`
- `source`
- `originalValue`
- `normalizedValue`
- `confidence`
- `providerReference`

### CanonicalGenreRegistry

Maps multilingual aliases to stable canonical genre IDs.

Examples:

- `Krimi` -> `crime`
- `Crime` -> `crime`
- `Kriminalfilm` -> `crime`
- `Komödie` -> `comedy`
- `Comedy` -> `comedy`
- `Spielfilm` -> `movie`
- `Movie` -> `movie`

Unknown genres are normalized into stable fallback IDs.

Example:

- `Science Fiction` -> `science-fiction`

### GenreLocalization

Maps canonical genre IDs to localized labels.

Current in-code foundation:

- `crime`, `de` -> `Krimi`
- `crime`, `en` -> `Crime`
- `comedy`, `de` -> `Komödie`
- `comedy`, `en` -> `Comedy`
- `movie`, `de` -> `Spielfilm`
- `movie`, `en` -> `Movie`

This is a domain foundation only.

External language files are not implemented yet.

---

## Future Localization Files

The intended future file layout is:

- `resources/i18n/de_DE/genres.json`
- `resources/i18n/en_US/genres.json`
- `resources/i18n/fr_FR/genres.json`
- `resources/i18n/it_IT/genres.json`

Example German genre translation file:

- `crime`: `Krimi`
- `comedy`: `Komödie`
- `movie`: `Spielfilm`

The domain model should continue to use canonical IDs internally.

Language files should only provide display labels.

---

## API Direction

Future APIs should expose both stable IDs and localized display labels when a locale is requested.

Example fields:

- `canonicalId`: stable internal ID, for example `crime`
- `label`: localized display label, for example `Krimi`
- `locale`: requested locale, for example `de_DE`
- `source`: source of the evidence, for example `tmdb`
- `originalValue`: original provider value, for example `Crime`
- `normalizedValue`: normalized value, for example `crime`

Clients should use `canonicalId` for filtering, grouping and stable behavior.

Clients should use `label` for display only.

---

## Boundaries

Implemented:

- genre evidence
- source-aware genre collection
- deterministic genre resolution
- genre JSON serialization
- canonical genre registry
- in-code localization foundation
- localized genre JSON

Not implemented yet:

- REST endpoint
- database persistence
- external language files
- metadata provider integration
- tvscraper integration
- TMDb integration
- TVDb integration
- IMDb integration
- SearchTimer integration
- user profile policy
- content rating / FSK enforcement

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
