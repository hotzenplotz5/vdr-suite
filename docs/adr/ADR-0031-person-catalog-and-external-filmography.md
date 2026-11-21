# ADR-0031: Person Catalog and External Filmography Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Architecture Documentation](../architecture/index.md)

---

## Status

Accepted

## Context

VDR-Suite already models people and roles as part of content classification and recording metadata.

The existing foundations include person identities, roles, provider references, person search services and recording person search. EPGSearch and SearchTimer work also make EPG events a first-class searchable domain.

LIVE remains the functional reference for VDR-centric workflows, but VDR-Suite is intended to go beyond a simple LIVE clone. A modern media interface should allow users to start from a person and then discover local and external media context.

A typical user flow is:

1. open an EPG event or recording detail view;
2. see a person such as an actor, director or producer;
3. open the person's detail view;
4. see local EPG events involving that person;
5. see local recordings involving that person;
6. see the role context for each result;
7. optionally compare against external filmography sources.

This is especially useful for actors, directors, producers, writers and other recurring contributors.

## Decision

VDR-Suite will introduce a future Person Catalog architecture as a domain-level concept above local EPG, recording and metadata search.

The Person Catalog will aggregate person-related information from local and external sources while keeping those sources clearly separated.

The local catalog side may include:

- EPG events;
- recordings;
- timers;
- SearchTimers;
- person-role classifications;
- provider references from local metadata sources.

The external catalog side may include later integrations such as:

- IMDb;
- TMDb;
- Wikidata;
- other configurable metadata providers.

External data is reference data only. It must not be treated as local VDR availability.

The primary person detail view should distinguish at least:

- local EPG matches;
- local recording matches;
- optional timer/SearchTimer matches;
- external known filmography.

Results must be grouped by role where possible:

- actor;
- director;
- producer;
- writer;
- composer;
- other/unknown;
- multiple roles.

A person may appear in more than one role for the same event or recording. The catalog must preserve that information instead of flattening it into a single role.

## Non-Goals

The Person Catalog is not intended to make external filmographies look like local inventory.

The Person Catalog is not intended to require a single hard-coded external provider.

The Person Catalog is not intended to replace local VDR, EPG or recording search.

The Person Catalog is not intended to implement provider-specific scraping in the domain layer.

## Consequences

### Positive

- Users can navigate from EPG or recording detail views to person-centered discovery.
- VDR-Suite can exceed LIVE behavior by combining local availability with optional external reference data.
- Existing person, role and provider-reference models can be reused.
- Local and external data can be presented together without confusing availability semantics.
- SearchTimer and EPGSearch can later use person identity as a richer search input.
- Future UI views can support person pages, role-grouped tabs and availability badges.

### Negative

- A new aggregation layer is required above EPG, recordings and metadata providers.
- Identity resolution becomes more important because people may have aliases, localized names or provider-specific identifiers.
- External provider integrations introduce caching, rate-limit and privacy considerations.
- Role grouping must remain explicit to avoid misleading search results.

## Architecture Guidance

The recommended future domain split is:

- `PersonIdentity` for stable local identity and provider references;
- `PersonRoleGroup` for grouped role presentation;
- `PersonLocalCatalogResult` for local EPG, recording, timer and SearchTimer matches;
- `PersonExternalCatalogResult` for external reference filmography;
- `PersonCatalogService` for local aggregation;
- `ExternalPersonCatalogProvider` for optional external lookup;
- `PersonCatalogController` for REST exposure.

Local catalog results should always remain authoritative for local availability.

External catalog results should always be labeled as external reference data.

The controller/API layer should expose enough metadata for a frontend to render clear sections such as:

- locally in EPG;
- locally in recordings;
- locally scheduled or SearchTimer-related;
- externally known filmography.

## API Direction

A future REST API may expose endpoints similar to:

- `GET /api/persons/search`;
- `GET /api/persons/{personId}`;
- `GET /api/persons/{personId}/catalog`;
- `GET /api/persons/{personId}/catalog/local`;
- `GET /api/persons/{personId}/catalog/external`.

The API must make source boundaries visible in JSON.

Example high-level response structure:

```json
{
  "person": {
    "displayName": "Johnny Depp",
    "normalizedName": "johnny depp",
    "providerReferences": []
  },
  "local": {
    "epg": [],
    "recordings": [],
    "timers": [],
    "searchTimers": []
  },
  "external": {
    "provider": "imdb-or-tmdb-or-wikidata",
    "filmography": []
  }
}
```

## Implementation Order

The Person Catalog should not interrupt the current EPGSearch/SearchTimer hardening path.

A sensible later implementation order is:

1. document person catalog architecture;
2. add local person catalog query/result models;
3. aggregate local EPG matches;
4. aggregate local recording matches;
5. group results by role;
6. expose local person catalog API;
7. add UI contract documentation;
8. add external provider boundary;
9. add external filmography providers;
10. add caching, rate-limit and privacy policy.

## Relationship to LIVE Parity

LIVE remains the local functional reference for VDR workflows.

The Person Catalog intentionally goes beyond LIVE parity by treating people as first-class navigation and discovery entities.

This supports the VDR-Suite goal of being a LIVE superset rather than a screen-for-screen clone.

## Back

- [Back to ADR Index](index.md)
- [Back to Architecture Documentation](../architecture/index.md)
- [Back to Documentation Index](../index.md)
