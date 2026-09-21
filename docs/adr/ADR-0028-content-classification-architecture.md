# ADR-0028: Content Classification Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Development Index](../development/index.md)

---

## Status

Proposed

## Date

2026-06-19

## Context

VDR-Suite now has an implemented EPG search API foundation.

The next architecture area is classification of media and EPG content. This includes genres, content ratings, keywords, collections, user tags, folder-derived classifications and metadata-provider classifications.

The project must not model genre as a single plain string. That would create a dead end once multiple sources become available.

Future classification sources include:

- DVB / EPG content descriptors
- tvscraper
- scraper2vdr
- TMDb
- TVDb
- IMDb
- user-defined genres and tags
- folder or path based hints
- derived rules from metadata, names or recording groups

Future client and household scenarios also require classification to support policy decisions, not only filtering. Examples include:

- children should only see content below an allowed content rating
- a TV frontend may have a default profile
- a user may see only selected backends
- a backend may be read-only for some users
- search results may need to hide content before rendering
- streaming may need a policy decision before playback starts

VDR remains the source of truth for VDR-owned domains. VDR-Suite complements VDR and may enrich VDR data with metadata and user-owned classifications.

## Decision

VDR-Suite shall introduce a content classification architecture instead of a genre-only architecture.

Genre is a classification type, not the entire model.

The architecture shall preserve classification evidence from multiple sources and resolve user-facing classification views from that evidence.

The conceptual model is:

```text
ContentClassification
├─ content identity
├─ classification type
├─ source
├─ original value
├─ normalized value
├─ optional confidence
├─ optional provider reference
└─ optional timestamp
```

Initial classification types:

```text
ClassificationType
├─ genre
├─ contentRating
├─ keyword
├─ collection
├─ userTag
└─ folderHint
```

Initial classification sources:

```text
ClassificationSource
├─ epg
├─ dvb
├─ tvscraper
├─ scraper2vdr
├─ tmdb
├─ tvdb
├─ imdb
├─ user
├─ folder
└─ derived
```

The classification layer must preserve original provider values and provide normalized values for search, filtering, grouping and policy decisions.

## Genre Decision

Genres shall be modeled as source-aware classification entries.

The system may eventually resolve a user-facing genre set from several sources:

```text
GenreSet
├─ primaryGenre
├─ secondaryGenres
├─ sourceGenres[]
└─ userOverrides[]
```

A single EPG event or recording may therefore have several genre facts at once:

```text
EPG:        Spielfilm
tvscraper: Krimi
TMDb:      Crime, Drama
User:      Lieblingsserie
Folder:    Nordic Noir
```

The architecture must not discard source data when resolving the final view.

User classifications and overrides may influence display and search ranking, but they must not erase provider evidence.

## Content Rating Decision

Content rating shall be modeled as a classification type.

The model must be able to support more than one rating system:

```text
ContentRating
├─ FSK
├─ MPAA
├─ TV parental rating
├─ provider-specific rating
└─ user-defined household rating
```

A future policy layer may use content ratings to filter visibility and playback permission for profiles.

Example:

```text
Profile: child-15
Allowed rating: FSK 12 and below
Hidden: FSK 16 and FSK 18
```

Phase 46.0 does not implement policy enforcement. It defines the classification shape required for future policy enforcement.

## Policy Boundary

Classification and policy are related but separate.

Classification answers:

```text
What is this content?
```

Policy answers:

```text
May this user or device see, stream or modify this content?
```

Future policy decisions may consider:

- profile identity
- backend permissions
- content rating
- user tags
- backend read-only state
- action type
- device type
- streaming permission
- timer permission
- delete or move permission

The classification architecture must provide the facts that policy can evaluate later, but policy enforcement is out of scope for this ADR.

## Relationship To Existing ADRs

ADR-0025 defines a configurable metadata provider architecture. Content classification uses that provider boundary to ingest and normalize metadata-derived facts.

ADR-0020 and ADR-0027 keep VDR-Suite open for future media federation. Content classification must therefore not assume that every classified item is only a VDR recording or VDR EPG event.

ADR-0021 defines selective backend query strategy. Classification resolution must not require full EPG mirroring as a default search path.

## Rules

- Do not model genre as a single plain string in stable APIs.
- Preserve source identity for classifications.
- Preserve original values from providers.
- Provide normalized values for search and grouping.
- Allow several classifications of the same type for the same content.
- Allow user classifications and overrides without deleting provider evidence.
- Keep classification separate from policy enforcement.
- Keep VDR as the source of truth for VDR-owned state.
- Do not introduce persistent full EPG mirroring as a classification prerequisite.
- Keep metadata-provider details behind provider boundaries.

## Consequences

### Positive

- Supports DVB, tvscraper, TMDb, TVDb, IMDb, user and folder classifications without redesign.
- Enables richer EPG search and recording search.
- Prepares genre filtering, genre grouping and genre sorting.
- Prepares content rating and FSK-style filtering.
- Prepares profile-aware frontends such as TV apps.
- Preserves provider evidence for transparency.
- Keeps future SearchTimer matching open for genre and rating constraints.

### Negative

- More complex than a single genre string.
- Requires resolver logic before a final user-facing view can be shown.
- Requires future API care to avoid exposing unstable provider-specific internals.
- Policy enforcement still needs a separate ADR and implementation plan.

## Implementation Guidance

Near-term implementation should proceed in small steps:

1. Define genre classification domain objects.
2. Define classification source enums or value objects.
3. Define normalized genre values without discarding source values.
4. Add tests for multi-source genre evidence.
5. Add JSON only after the domain shape is stable.
6. Add search filters only after resolver behavior is clear.

Do not start with tvscraper, TMDb, TVDb or IMDb integration directly.

The first implementation should prove the domain model with simple in-memory or EPG-derived classifications.

## Out Of Scope

The following are out of scope for this ADR:

- C++ implementation
- database schema changes
- tvscraper integration
- TMDb integration
- TVDb integration
- IMDb integration
- user account management
- parental control enforcement
- streaming authorization
- UI profile switching
- SearchTimer execution

## Future ADR Candidates

This ADR intentionally prepares but does not decide:

- user profile architecture
- access policy architecture
- content rating enforcement
- streaming authorization
- TV frontend profile behavior
- metadata provider priority rules

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
