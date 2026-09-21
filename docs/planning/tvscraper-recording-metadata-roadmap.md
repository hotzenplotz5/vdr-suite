# Post-Phase-61 Metadata Provider Strategy

## Status

This document is **post-Phase-61 strategy and backlog**. It is not an active Phase 61 implementation instruction.

Phase 61 is completed through PR #100, with performance hardening through PRs #102-#108. Current main also includes backend-scoped Global Search from PR #111 over the persisted metadata/person read models.

Manual Recording metadata search and assignment is implemented in Draft PR #135 under [ADR-0051](../adr/ADR-0051-manual-recording-metadata-assignment.md). It is not complete until the final real-system acceptance in [Manual Recording Metadata Assignment](../development/manual-recording-metadata-assignment.md) succeeds.

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
- Recordings 2 and existing EPG detail-owner integration;
- actor identity, backend-scoped permissions, browser CSRF and accountability from Phase 62.

These capabilities are documented in:

- [Phase 61 Metadata, Genre and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Metadata-Backed Genre Browser](../architecture/metadata-genre-browser.md)
- [Backend-Scoped Global Search](../architecture/global-search.md)
- [Manual Recording Metadata Assignment](../development/manual-recording-metadata-assignment.md)

## Current provider rules

- VDR-Suite persistence and domain repositories own public metadata behaviour.
- TVScraper, SuiteBridge, sidecars or future services provide evidence only.
- Browser code never calls provider databases, plugin services or provider APIs directly.
- Normal GET rendering/search does not trigger provider resolution.
- Explicit authorized candidate search may call a managed external catalog through a bounded backend adapter.
- Evidence carries backend scope, provider identity, source kind, state and original value.
- Provider failure retains stale usable evidence rather than deleting it.
- A new provider must not create a competing public identity/taxonomy/read model.
- Shared provider databases must not become a client or Agent protocol.
- Manual Suite assignments must not write back into provider-owned storage.

## Current source status

| Source / strategy | Status | Current role / condition |
| --- | --- | --- |
| VDR native Recording metadata | Implemented | Persistent native technical/title/subtitle/person evidence and cache integration. |
| DVB EPG descriptors | Implemented bounded source | Native fallback evidence for accepted classification rules. |
| TVScraper via SuiteBridge | Implemented accepted paths | Recording/EPG metadata, people, artwork and Genre/media-type evidence. |
| Persisted public TVScraper EPG cache | Implemented | Suite-owned cache/read model; normal GETs do not call provider. |
| Suite manual Recording assignment | Draft implementation in PR #135 | Immutable manual evidence, locked assignment, revision conflict, withdrawal and automatic fallback; pending final real-system acceptance. |
| TMDB Recording candidate adapter | Draft bounded implementation in PR #135 | Explicit movie/series search plus season/episode enumeration and selected-poster materialization; never used by normal GET rendering. |
| Sidecar metadata import | Backlog | Must map into existing Suite target/evidence/assignment model. |
| General TMDB/TVDB/IMDb enrichment adapter | Deferred/backlog | The bounded manual TMDB candidate adapter does not create a general background metadata provider. Any broader adapter still requires a separate contract. |
| epgd/epg2vdr adapter or migration | Deferred | Phase 61 prerequisite is complete, but no shared-database integration is approved. Use an adapter/import boundary, not direct public DB authority. |
| User tags and field-level corrections | Planned later | Manual relationship selection is bounded by ADR-0051; arbitrary tags and field editing remain separate work. |
| Derivative artwork processing | Backlog | Existing provider-neutral references/delivery are complete; resizing/transcoding farm is not required for manual assignment. |
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
3. acquisition is bounded and asynchronous or explicitly user-triggered where remote/native calls are involved;
4. normal GETs remain provider-free and query-only where documented;
5. failure retains prior usable evidence with explicit stale/error state;
6. aliases/normalization do not create a parallel taxonomy;
7. write transactions are bounded and no-op work is avoided;
8. migrations, restart persistence and provider-unavailable operation are tested;
9. frontend modules use `VdrSuiteClientApi` and existing detail owners;
10. real-system acceptance is recorded when VDR/plugin or public metadata behaviour changes;
11. browser sessions use the Phase 62 CSRF, authorization and accountability path;
12. provider credentials and private paths remain outside public responses and audit evidence.

## Priorities after Phase 61

### Active bounded work

- complete PR #135 deterministic CI and final real-system acceptance;
- prove one unmatched movie and one exact series episode through assignment, restart, stale revision, denial and withdrawal;
- preserve TVScraper as automatic evidence and verify immediate fallback after manual withdrawal.

### Near-term maintenance/backlog

- monitor query plans and refresh cadence on production-shaped data;
- add only evidence-backed provider/alias corrections;
- preserve no-op and query-only guardrails;
- decide the remote asset separately from metadata/provider work;
- keep Global Search on persisted data rather than introducing live provider lookup;
- evaluate user tags and field-level correction separately from relationship assignment.

### Deferred until later phases

- actor-scoped provider credentials beyond the current managed daemon credential;
- remote-site provider dispatch through secure Agents;
- cross-site provider reconciliation;
- general background external-catalog enrichment;
- bulk metadata correction and administration center.

### Later vision

- explainable recommendations and knowledge graph in Phase 68.

## Superseded planning statement

Any older text that says “Phase 61 must now implement the metadata database/provider integration” is superseded by the completed Phase 61 closeout. This file now owns only post-phase provider strategy, the bounded manual-assignment acceptance state and explicit backlog.
