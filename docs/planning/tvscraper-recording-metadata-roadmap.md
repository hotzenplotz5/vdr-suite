# Metadata Provider Strategy and Post-Phase 61 Backlog

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Planning Index](index.md)
- [Strict Roadmap](roadmap.md)
- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)

---

## Status

```text
Phase 60.15 - Recording Metadata and Poster Preparation
Status: Completed

Phase 61 - Suite Metadata and Genre Platform
Status: Completed

Next runtime implementation phase
Phase 62 - Identity, RBAC and Accountability Foundation
```

This document is no longer the main phase-order roadmap. It records the provider strategy and optional post-Phase 61 metadata backlog. The strict numbered order remains in [Roadmap](roadmap.md).

---

## Accepted Provider Rule

```text
Use mature external metadata providers for acquisition and matching.
Normalize accepted evidence into Suite-owned assignments and read models.
Retain provider identity, state and provenance.
Do not make TVScraper or another provider the public contract.
Do not reimplement scraper behavior without a proven gap.
```

Phase 61 established this rule in an accepted persistent vertical slice for Recording and EPG Genre browsing.

---

## Completed Foundation

Phase 60.15 completed:

- separation of technical/native and provider-derived Recording fields;
- provider-neutral metadata and artwork references;
- additive SQLite cache persistence and restart behavior;
- deterministic no-provider placeholders;
- Suite-owned opaque artwork IDs and authenticated delivery;
- preserved lazy Recording folder and detail loading.

Phase 61 completed:

- persistent backend-scoped Recording and EPG target bindings;
- canonical Genre assignments with multiple assignments per target;
- provider and derived evidence for accepted TVScraper and DVB paths;
- explicit active, missing, unknown, stale and conflict states;
- derived EPG browse classes and canonical Film subgenres;
- indexed counts and paged backend-scoped queries;
- provider-neutral Suite REST and Web Client API routes;
- bounded asynchronous enrichment and restart persistence;
- real-system acceptance and subsequent B1-B4 performance hardening.

See [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md).

---

## Current Provider Boundary

The accepted flow is:

```text
VDR or plugin metadata source
  -> bounded local adapter
  -> immutable provider evidence
  -> Suite normalization and assignment
  -> Suite-owned persisted read model
  -> Suite REST and Client API
  -> frontend
```

Provider-global state, borrowed VDR pointers, provider database rows, credentials and local filesystem paths must not cross into public client contracts.

---

## Current TVScraper Use

TVScraper is used behind Suite-owned boundaries for:

- Recording native metadata acquisition;
- EPG movie/series/episode detail acquisition;
- people, roles, portraits and bounded artwork;
- EPG media-type and Genre evidence;
- derived Film, Serie, Dokumentation and Sport browse classification.

TVScraper is not the only possible provider and is not the Suite database authority.

---

## Artwork Boundary

Artwork remains represented through Suite-owned identities and allowlisted delivery paths:

```text
provider image evidence
  -> validation/proxy/import boundary
  -> Suite artwork reference or asset identity
  -> authenticated Suite-controlled URL
```

The frontend must never receive arbitrary provider filesystem paths.

---

## Optional Post-Phase 61 Backlog

The following extensions remain useful, but they do not keep Phase 61 open:

### Additional provider adapters

- scraper2vdr adapter where it provides unique evidence;
- external catalogue adapters;
- sidecar metadata import;
- manual correction/assignment provider;
- offline/local catalogue provider.

### Broader metadata identity

- universal movie, series, season and episode entity resolution;
- cross-backend external-ID reconciliation;
- richer keyword, rating and language normalization;
- manual lock/override workflows.

### Artwork lifecycle

- persistent derivative policy;
- checksum and duplicate handling;
- dimension variants;
- attribution/licensing metadata;
- garbage collection and recovery.

### Operations and observability

- provider refresh job history;
- retry/backoff and invalidation diagnostics;
- backup and restore tooling;
- rolling performance summaries;
- operational export and metrics integration.

### Later intelligence

- recommendation inputs;
- knowledge-graph facts and provenance;
- explainable ranking;
- user corrections and feedback.

Recommendation and knowledge-graph work belongs to Phase 68. RBAC-protected manual changes require Phase 62 foundations.

---

## Provider Evaluation Before Reinvention

Before implementing a new acquisition path, evaluate:

- matching quality for German and international titles;
- alias and translated-title support;
- movie, series, season and episode disambiguation;
- people, roles, characters and guest normalization;
- Genre, rating, keyword and external-ID coverage;
- artwork quality and licensing;
- update and invalidation behavior;
- API stability and operational requirements;
- offline/cache behavior;
- multi-backend normalization suitability.

Only proven gaps should become Suite-owned acquisition logic.

---

## Capability Direction

A future backend/provider may advertise explicit capabilities such as:

```text
metadata.recording.basic
metadata.recording.people
metadata.recording.characters
metadata.recording.genre
metadata.recording.rating
metadata.recording.artwork
metadata.recording.externalIds
metadata.provider.tvscraper
metadata.provider.scraper2vdr
metadata.provider.externalCatalog
metadata.provider.sidecar
metadata.provider.manual
```

Capability support does not imply permission to trigger a refresh or mutation. Phase 62 must provide the actor and authorization foundation for protected operations.

---

## Boundaries

VDR-Suite must not:

- treat TVScraper as the only metadata strategy;
- expose plugin/provider rows directly to clients;
- expose arbitrary provider paths as artwork identity;
- write plugin-owned storage without an explicit contract;
- assume every backend has identical provider capabilities;
- reimplement a scraper before provider evaluation;
- make VDR-Rectools the metadata source of truth;
- reopen Phase 61 merely because optional adapters remain.

VDR-Suite may:

- acquire provider evidence through bounded adapters;
- normalize and persist selected assignments;
- cache results with explicit state and invalidation;
- offer no-provider fallback;
- allow later protected manual corrections;
- let VDR-Rectools consume resolved metadata for processing/export.

---

## Relation to Future Phases

```text
Phase 62
  -> authorize provider refresh and manual correction

Phase 63
  -> carry provider evidence safely across Backend Agents

Phase 67
  -> stabilize public metadata contracts under /api/v1

Phase 68
  -> consume mature metadata/provenance for recommendations and graph work
```

---

## Back

- [Back to Planning Index](index.md)
- [Back to Roadmap](roadmap.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)