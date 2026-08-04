# ADR-0051: Manual Recording Metadata Assignment

## Navigation

- [README](../../README.md)
- [ADR Index](index.md)
- [TVScraper Recording Metadata Integration](ADR-0036-tvscraper-recording-metadata-integration.md)
- [Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [Manual Recording Metadata Runtime](../development/manual-recording-metadata-assignment.md)

---

## Status

Accepted

Date: 2026-08-04

---

## Context

TVScraper remains the preferred recording metadata provider when it identifies a recording. Some recordings nevertheless remain unmatched or are matched to the wrong movie, series or episode.

The public TVScraper service accepts a VDR `cRecording*` and returns the metadata already selected by TVScraper. It does not provide a candidate-search or exact manual-assignment API. Writing directly into TVScraper's database or private cache would create an unsupported ownership boundary and would require knowledge of provider-private storage.

VDR-Suite already owns:

- backend-scoped Recording target identities;
- normalized metadata entities, evidence and assignments;
- manual and relationship-locked assignment fields;
- actor identity, backend permissions, CSRF and accountability;
- server-side external HTTP transport and TMDB credentials;
- Recordings 2 detail ownership.

The missing capability is therefore a Suite-owned review and correction workflow rather than a TVScraper modification.

---

## Decision

VDR-Suite owns manual recording metadata search, selection and withdrawal.

The workflow is:

```text
Recordings 2 detail
  -> authenticated Suite API
  -> backend-only external catalog search
  -> bounded normalized candidates
  -> explicit user selection
  -> Suite-owned immutable manual evidence
  -> selected relationship-locked assignment
  -> existing recording metadata read model
```

TVScraper remains read-only from VDR-Suite's perspective. The Suite does not write into TVScraper storage and does not fork or extend TVScraper for this feature.

The initial external candidate provider is TMDB. The public API and frontend remain provider-neutral enough to add another candidate provider later, but no second provider is selected by this ADR.

---

## Assignment Semantics

A selected candidate creates:

- one immutable `manual` evidence observation;
- one Suite metadata entity;
- one selected metadata assignment with `manual_assignment = true`;
- `relationship_locked = true`;
- one `manual-override` assignment-evidence link;
- provider-scoped external-ID evidence;
- presentation values required by the recording detail read model.

Only one assignment may remain selected for a recording target. A replacement supersedes the previous selected assignment. Withdrawal changes the selected manual assignment to `withdrawn`; it does not delete history.

An expected revision is required when replacing or withdrawing an existing selection. A stale browser tab fails with a conflict instead of overwriting a newer decision.

---

## Resolver Priority

The recording metadata read model applies this priority:

1. active relationship-locked manual assignment;
2. current persisted native provider metadata, normally TVScraper;
3. existing native Recording and EPG fallback fields;
4. deterministic empty or placeholder presentation.

Automatic provider refresh may continue while a manual assignment is active. New automatic evidence remains available for later resolution but must not silently replace the locked manual relationship.

Withdrawing the manual assignment immediately restores normal automatic resolution.

---

## Recording Identity

Browser requests use the existing backend-native Recording identity. The server resolves it against `vdr_recording_cache` to the canonical backend-scoped `cache_key` used by Suite metadata target bindings.

The browser must not construct or persist internal metadata target IDs or cache keys.

---

## Candidate Search

Candidate search is explicit and never runs during ordinary Recording GET rendering.

Supported initial operations:

- movie search;
- series search;
- season enumeration;
- episode enumeration.

Search requirements:

- backend-only provider communication;
- bounded request text and result count;
- configured timeouts and response-size limits;
- retry only for bounded transient and rate-limit failures;
- no provider token in browser state, responses, logs or accountability evidence;
- no automatic acceptance of the first fuzzy result.

Search, season and episode requests are protected operations because they consume managed provider credentials and external provider quota.

---

## Artwork

TMDB candidate paths are provider references, not browser URLs and not Suite artwork identities.

Only the poster belonging to the explicitly selected candidate is downloaded. It is:

- fetched server-side through the allow-listed HTTP transport;
- bounded by response size and accepted image media types;
- written atomically beneath `/var/cache/vdr-suite/recording-metadata/posters`;
- served through the existing authenticated Suite Recording metadata image route.

The external provider URL and the local filesystem path are not returned to the browser.

---

## Security and Accountability

All candidate search and assignment POST routes use the Phase 62 security gate.

Required permission:

```text
metadata.recording.assign
```

Rules:

- backend scope is derived from the route, never from the JSON body;
- browser-session requests require valid CSRF evidence;
- `role.admin` grants the permission only for its backend scope;
- `role.read-only` blocks the operation;
- the authenticated actor ID is supplied by the HTTP security context;
- clients cannot choose `created_by_ref`;
- authorization decisions and success/failure outcomes are recorded through the existing accountability model;
- provider tokens, candidate payloads, descriptions and filesystem paths are not copied into accountability records.

---

## Capability Contract

The backend capability report advertises:

```text
metadata.recording.manualSearch
metadata.recording.manualAssignment
metadata.recording.manualAssignment.movie
metadata.recording.manualAssignment.series
metadata.recording.manualAssignment.episode
```

The runtime still validates provider configuration. A capability does not turn missing credentials into a successful provider request.

---

## Public Routes

Initial route family:

```text
GET  /api/backends/{backendId}/recordings/metadata/manual
POST /api/backends/{backendId}/recordings/metadata/search
POST /api/backends/{backendId}/recordings/metadata/seasons
POST /api/backends/{backendId}/recordings/metadata/episodes
POST /api/backends/{backendId}/recordings/metadata/assign
POST /api/backends/{backendId}/recordings/metadata/withdraw
```

The existing Recording detail and image routes remain the public read-model owner after assignment.

---

## Consequences

Positive:

- unmatched recordings can be corrected without modifying TVScraper;
- the selected relationship survives daemon restart;
- correction history and actor provenance remain explainable;
- automatic evidence is preserved rather than destroyed;
- film, series and exact episode selection share one consistent workflow;
- the frontend remains free of provider credentials and direct provider calls.

Trade-offs:

- candidate search consumes external provider quota;
- exact episode selection adds an additional series/season navigation step;
- manual assignments require explicit lifecycle and conflict handling;
- selected poster storage becomes a Suite-owned cache responsibility.

---

## Non-Goals

This ADR does not introduce:

- a TVScraper fork or upstream change;
- writes into TVScraper-owned databases or caches;
- automatic bulk acceptance of fuzzy matches;
- field-by-field metadata editing;
- multi-recording batch correction;
- a universal metadata administration center;
- direct browser communication with TMDB or another catalog;
- provider search during normal Recording GET requests.

---

## Validation

Required deterministic coverage includes:

- assignment, replacement, revision conflict and withdrawal persistence;
- immutable evidence and manual-override relation;
- movie, series, season and episode candidate parsing;
- timeout, rate-limit and provider-error behavior;
- selected-poster validation and atomic cache publication;
- route-authoritative backend authorization;
- Admin, Read-only, wrong-backend and CSRF behavior;
- dispatch and outcome accountability;
- manual read-model priority and automatic fallback after withdrawal;
- Recordings 2 search, candidate, series, season, episode and withdrawal UI contracts;
- install staging for frontend assets and poster cache.

Real-system acceptance is defined in the linked development document.

---

## Related Decisions

- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0014: Recording Identity Strategy](ADR-0014-recording-identity-strategy.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](ADR-0036-tvscraper-recording-metadata-integration.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0049: Audit and Security Event Model](ADR-0049-audit-security-event-model.md)
- [ADR-0050: Domain Repository SQLite Boundary](ADR-0050-domain-repository-sqlite-boundary.md)
