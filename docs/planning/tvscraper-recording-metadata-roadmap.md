# Post-Phase-61 Metadata Provider Strategy

## Status

This document is **post-Phase-61 strategy and backlog**. It is not an active Phase 61 implementation instruction.

Phase 61 is completed through PR #100, with performance hardening through PRs #102-#108. Current main also includes backend-scoped Global Search from PR #111 over the persisted metadata/person read models.

## Completed foundation

The following former roadmap goals are implemented for the accepted Recording/EPG scope:

- Suite-owned backend-scoped metadata target bindings;
- persistent native/provider/derived evidence;
- normalized people relations;
- canonical Genre identities and assignments;
- explicit active, missing, unknown, stale and conflict states;
- provider-neutral artwork references and authenticated delivery;
- Recording and EPG Genre read models;
- TVScraper media-type/Genre evidence acquisition through SuiteBridge;
- asynchronous bounded enrichment;
- query-only provider-free Genre and global-search GET paths;
- provider-failure isolation and restart persistence;
- Recordings 2 and existing EPG detail-owner integration.

These capabilities are documented in:

- [Phase 61 Metadata, Genre and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Metadata-Backed Genre Browser](../architecture/metadata-genre-browser.md)
- [Backend-Scoped Global Search](../architecture/global-search.md)

## Current provider rules

- VDR-Suite persistence and domain repositories own public metadata behaviour.
- TVScraper, SuiteBridge, sidecars or future services provide evidence only.
- Browser code never calls provider databases, plugin services or provider APIs directly.
- Normal GET rendering/search does not trigger provider resolution.
- Evidence carries backend scope, provider identity, source kind, state and original value.
- Provider failure retains stale usable evidence rather than deleting it.
- A new provider must not create a competing public identity/taxonomy/read model.
- Shared provider databases must not become a client or Agent protocol.

## Current source status

| Source / strategy | Status | Current role / condition |
| --- | --- | --- |
| VDR native Recording metadata | Implemented | Persistent native technical/title/subtitle/person evidence and cache integration. |
| DVB EPG descriptors | Implemented bounded source | Native fallback evidence for accepted classification rules. |
| TVScraper via SuiteBridge | Implemented accepted paths | Recording/EPG metadata, people, artwork and Genre/media-type evidence. |
| Persisted public TVScraper EPG cache | Implemented | Suite-owned cache/read model; normal GETs do not call provider. |
| Sidecar metadata import | Backlog | Must map into existing Suite target/evidence/assignment model. |
| TMDB/TVDB/IMDb direct adapter | Deferred/backlog | Requires explicit provider contract, credentials, rate limits, provenance and no browser coupling. |
| epgd/epg2vdr adapter or migration | Deferred | Phase 61 prerequisite is complete, but no shared-database integration is approved. Use an adapter/import boundary, not direct public DB authority. |
| User corrections/tags | Planned later | Requires actor identity, authorization, provenance and accountability from Phase 62. |
| Derivative artwork processing | Backlog | Existing provider-neutral references/delivery are complete; resizing/transcoding farm is not required to keep Phase 61 open. |
| Provider job history/percentiles | Backlog | Current operational diagnostics exist; durable provider-job model requires an explicit later owner. |
| Recommendation/knowledge graph enrichment | Deferred | Phase 68 after identity, provenance, stable public API and accountability. |

## Recording-person payload contract

Current end-to-end main contract:

```text
maximum people: 128
maximum RMETA payload: 65,535 bytes
```

The regression model preserves all 52 modelled `Pulp Fiction` people. Draft PR #101 changes only plugin-side bounds and is not a valid provider-roadmap completion. Any larger contract must be versioned and changed across plugin, SVDRP transport, backend parser and tests together.

## Provider adapter acceptance checklist

A new provider/import path must prove:

1. existing Suite identities and backend scope are reused;
2. provider/source/provenance and evidence state are persisted;
3. acquisition is bounded and asynchronous where remote/native calls are involved;
4. normal GETs remain provider-free and query-only where documented;
5. failure retains prior usable evidence with explicit stale/error state;
6. aliases/normalization do not create a parallel taxonomy;
7. write transactions are bounded and no-op work is avoided;
8. migrations, restart persistence and provider-unavailable operation are tested;
9. frontend modules use `VdrSuiteClientApi` and existing detail owners;
10. real-system acceptance is recorded when VDR/plugin behaviour changes.

## Priorities after Phase 61

### Near-term maintenance/backlog

- monitor query plans and refresh cadence on production-shaped data;
- add only evidence-backed provider/alias corrections;
- preserve no-op and query-only guardrails;
- decide the remote asset separately from metadata/provider work;
- keep Global Search on persisted data rather than introducing live provider lookup.

### Deferred until Phase 62+

- user corrections, preferences and ownership;
- actor-scoped provider credentials;
- accountability for manual metadata changes;
- remote-site provider dispatch through secure Agents;
- cross-site provider reconciliation.

### Later vision

- explainable recommendations and knowledge graph in Phase 68.

## Superseded planning statement

Any older text that says “Phase 61 must now implement the metadata database/provider integration” is superseded by the completed Phase 61 closeout. This file now owns only post-phase provider strategy and explicit backlog.