# VDR Ecosystem Parity and Product Gap Roadmap

## Purpose

This document compares current VDR-Suite behaviour with the relevant ecosystem reference points:

- VDR Core;
- Live;
- epgsearch;
- RESTfulAPI;
- VDR-Suite.

TVScraper and SuiteBridge appear only where they are real private metadata/plugin/Agent sources or adapters. They are not substitute public application architectures.

There is no VDR-Suite component named `Wikipedia Search`. The canonical comparison for SearchTimer and EPG-search semantics is `epgsearch`.

## Status legend

- **STRONG** — implemented and well covered for the stated Suite scope.
- **PARTIAL** — useful implementation exists, but exact parity or full semantics remain incomplete.
- **MISSING** — no complete Suite runtime surface exists.
- **DEFERRED** — assigned intentionally to a later phase.
- **DIFFERENT** — Suite intentionally uses a different architecture.
- **BETTER** — Suite provides a stronger boundary or safety model for the stated scope.

## Current position

Baseline reconciled on 2026-07-27 against `44ae3102ab202ee0dfc974ee0bc9624b9219ad2d`.

```text
Latest completed numbered runtime phase: Phase 61
Completed hardening: Post-Phase 61 Performance Hardening (B1-B4)
Completed platform features: Remote/Live Overlay (#110), Global Search (#111)
Next strict runtime phase: Phase 62 - Identity, RBAC and Accountability Foundation
```

## Executive comparison

| Dimension | VDR Core | Live | epgsearch | RESTfulAPI | VDR-Suite |
| --- | --- | --- | --- | --- | --- |
| Native VDR authority | STRONG | Uses VDR | Uses VDR | Uses VDR | DIFFERENT: delegates native authority to VDR |
| Web UI | None in core | STRONG mature single-VDR UI | Mostly OSD/plugin | API only | STRONG in current browser domains, incomplete full replacement |
| Channels and EPG | Native | STRONG | Search focused | STRONG API | STRONG: timeline, day views, details, cache and Genres |
| Recordings | Native | STRONG UI | Limited relevance | STRONG API | STRONG: Recordings 2, metadata, people, Genres and guarded actions |
| SearchTimer | None | Mature integration | STRONG authority | Service exposure | STRONG foundation; edge parity PARTIAL |
| Metadata / people / artwork | Plugin dependent | Plugin integrated | Not primary | Provider-shaped | BETTER provider-neutral persisted read models |
| Genre browsing | Skin/plugin dependent | Limited/general | Search categories | Raw/provider dependent | STRONG persistent Recording/EPG Genre platform |
| Global search | Native/plugin-specific pieces | Multiple page searches | EPG/SearchTimer search | Endpoint-specific | STRONG first backend-scoped cross-domain search |
| Remote control | Native input | STRONG UI | Not primary | API support | STRONG backend-neutral foundation; #110 mobile behaviour complete |
| Live streaming | Native providers/devices | STRONG | Not primary | Provider endpoints | MISSING public Gateway; Phase 65 |
| Legacy OSD in browser | Native OSD | STRONG compatibility surface | Native menus | OSD surfaces | MISSING isolated bridge; Phase 66 |
| Multi-backend scope | Single native instance | Single-instance oriented | Single-instance oriented | Per plugin instance | BETTER architecture/read-only foundation |
| User RBAC/accountability | Native config/limited | Limited | Limited | Limited | DEFERRED to Phase 62 |
| Secure remote sites | External setup | Not central | Not central | Not central | DEFERRED to Phase 63 |
| Stable public versioned API | Plugin/ABI boundaries | UI routes | Service commands | Plugin API | PARTIAL; `/api/v1` Phase 67 |

# VDR Core comparison

VDR Core remains authoritative for tuners, schedules, native timers, recordings, replay, OSD and plugin execution. VDR-Suite is not a VDR fork and must not duplicate those internals in its control plane.

| VDR Core area | VDR-Suite status | Current assessment |
| --- | --- | --- |
| Runtime status | STRONG | Backend-aware status and health foundations exist. |
| Channels | STRONG | Read, current programme, day navigation and movement/sorting foundations exist. |
| EPG | STRONG | Persistent cache, windows, search, details and Genre paths exist. |
| Recordings | STRONG | Recordings 2 owns lazy folders, cards, details, metadata, people, artwork and actions. |
| Timers read | STRONG foundation | Backend-aware read paths and Client API wrappers exist. |
| Timer mutation | PARTIAL | Controlled native paths/readback exist; universal revision/idempotency and field parity do not. |
| Recording rename/move/trash | STRONG | Validation, preview, policy, execution and readback boundaries. |
| Remote input | STRONG foundation | Backend-neutral allowlisted actions and live acceptance. |
| Native OSD | MISSING as Suite bridge | Phase 66; not implied by live overlay. |
| Live/replay media | MISSING as Suite Gateway | Phase 65. |
| Multi-instance federation | BETTER target | Backend scope/read-only exist; secure Agents remain Phase 63. |

Largest VDR-Core parity risks are lossless Timer representation, uncertain native mutation outcomes, cross-site lifecycle semantics, streaming and OSD compatibility.

# Live comparison

## Where Live remains stronger

- mature integrated Live TV/recording streaming;
- legacy OSD/plugin compatibility pages;
- polished long-established Timer/SearchTimer editing;
- broad single-server operational familiarity;
- specialist plugin-specific pages.

## Where VDR-Suite is competitive or stronger

| Live area | VDR-Suite status | Assessment |
| --- | --- | --- |
| Dashboard/backend status | STRONG | Backend-aware and designed for federation. |
| Channel browser/day guide | STRONG | Modern modular browser flows. |
| EPG timeline/details | STRONG | Persistent cache, rich details and Genre navigation. |
| Recording browser | STRONG | Recordings 2, lazy loading, metadata/people/artwork and guarded actions. |
| Recording action safety | BETTER | Explicit validation, preview, policy and readback. |
| Metadata details | BETTER boundary | Provider-neutral Suite routes; no browser/provider coupling. |
| Genre browsing | BETTER | Persistent canonical backend-scoped Recording/EPG read model. |
| Global search | STRONG first slice | One selected backend across persisted Recording/EPG titles, subtitles and people. |
| SearchTimer | STRONG foundation | Exact edge semantics and full polish remain partial. |
| Remote control | STRONG foundation | Backend-neutral; #110 isolates pressed state and dispatch guard. |
| Live streaming | MISSING | Phase 65. |
| Legacy OSD | MISSING | Phase 66. |
| Multi-backend policy | BETTER | Explicit backend identity and server-enforced read-only mode. |
| Per-user RBAC/audit | DEFERRED | Phase 62. |

VDR-Suite is already a credible modern browser for channels, EPG, Recordings, metadata, Genres, global search and several control workflows. It is not yet a complete Live replacement because streaming, legacy OSD and some mature Timer/SearchTimer UX remain missing.

# epgsearch comparison

Implemented foundations:

- SearchTimer domain values and backend-neutral list;
- create/update/delete command paths;
- RESTfulAPI service adapter;
- validation, dry-run and controlled execution;
- preview cache/invalidation and native preview capability;
- channel-group, extended-info, blacklist, directory and conflict discovery foundations;
- real VDR validation/readback;
- Client API wrappers;
- backend-scoped EPG title search and the separate global-search read model.

| epgsearch area | VDR-Suite status | Remaining work/proof |
| --- | --- | --- |
| SearchTimer list | STRONG | Keep exact edge-field mapping regression covered. |
| Create/update/delete | STRONG foundation | Universal revision/idempotency and failure semantics remain. |
| QuerySearchTimer preview | STRONG foundation | Permanent exact-result parity matrix still needed. |
| QuerySearch | PARTIAL | Suite EPG/global search exists; service-command equivalence is not universal. |
| Channel groups | PARTIAL | Discovery exists; completeness/UI coverage needs proof. |
| Extended EPG info | PARTIAL | Contracts exist; exact semantics need proof. |
| Blacklists | PARTIAL | Discovery/mapping exists; full behaviour needs proof. |
| Directory lists | PARTIAL | Discovery exists; exact short/full semantics need proof. |
| Timer conflicts | PARTIAL | Report paths exist; advice semantics/UI remain incomplete. |
| IsConflictCheckAdvised | MISSING or unproven | Needs explicit contract/acceptance. |
| AvoidRepeats | PARTIAL | Model support exists; exact native semantics need proof. |
| Duplicate detection | PARTIAL | Central orchestration-grade semantics belong to Phase 64. |
| Automatic native Timer creation | PARTIAL | Controlled paths exist; TimerIntent/assignment remains Phase 64. |
| Native OSD configuration | DIFFERENT/MISSING | Compatibility bridge is Phase 66, not primary Suite UI. |

SearchTimer is not a missing foundation. Full epgsearch replacement must not be claimed until repeat/duplicate/conflict/discovery edge semantics and real automation outcomes have strict test/live evidence.

# RESTfulAPI comparison

RESTfulAPI is a private backend adapter, not VDR-Suite's final public API. Browsers consume Suite routes and `VdrSuiteClientApi`; provider URLs, credentials and response shapes remain private.

| RESTfulAPI area | VDR-Suite status | Assessment |
| --- | --- | --- |
| Status/info | STRONG | Normalized through Suite services. |
| Channels | STRONG | Read and movement/sorting paths exist. |
| Events/EPG | STRONG | Suite cache/query/detail models are richer than passthrough. |
| Recordings read | STRONG | Lazy cache and Recordings 2. |
| Recording operations | STRONG | Adapter-backed with Suite safety/policy gates. |
| Timers | PARTIAL to STRONG | Core paths exist; universal parity/revision semantics incomplete. |
| SearchTimer commands | STRONG foundation | Adapter/workflow boundaries exist. |
| Remote control | STRONG foundation | Backend-neutral Suite action contract. |
| OSD endpoints | MISSING as public Suite feature | Phase 66 isolated bridge. |
| Streaming/provider URLs | DIFFERENT | Must remain private; Phase 65 adds Suite media sessions. |
| Signal/Femon/specialist data | MISSING or unproven | Needs explicit product decision. |
| Scraper/provider data | BETTER boundary | Normalized/persisted/proxied without provider path exposure. |
| Multi-backend identity/policy | BETTER | Explicit backend scope/read-only enforcement. |
| Public versioned API | PARTIAL/DEFERRED | Current transition routes; Phase 67 `/api/v1`. |

Exact parity with every RESTfulAPI endpoint is neither proven nor always desirable.

# TVScraper and SuiteBridge role

TVScraper provides private metadata evidence. SuiteBridge provides bounded plugin-local capability, metadata and transport functions. Neither is a browser API or Suite authority.

Current rules:

- provider calls run in bounded asynchronous worker/adapter paths;
- persisted Suite identities/evidence/read models own public behaviour;
- provider outages do not erase cached browse/search capability;
- normal Genre/global-search GETs perform no provider resolution;
- SuiteBridge/SVDRP limits are end-to-end contracts, not plugin-only constants;
- current RMETA bounds are 128 people and 65,535 bytes.

# VDR-Suite product assessment

## Strong today

- backend-aware status, channels and EPG navigation;
- EPG timeline and channel-day guide;
- Recordings 2 and guarded Recording actions;
- SearchTimer list/preview/controlled workflows;
- persistent metadata, people, artwork and Genres;
- backend-scoped global search;
- backend-neutral Remote and LiveOverlay;
- provider-neutral API/service boundaries;
- backend isolation and server-enforced read-only policy;
- query-only read paths for Genre/search;
- modular frontend ownership and packaging/real-system testing.

## Architecturally stronger separation

- Suite-owned identities and backend scope;
- provider evidence/provenance instead of provider authority;
- private adapter protocols;
- server-side read-only/safety gates;
- repository/service/controller boundaries;
- dedicated query-only SQLite reads;
- frontend reuse of single detail owners.

## Not a complete replacement yet

- Streaming Gateway;
- legacy OSD compatibility;
- production identity/RBAC/accountability;
- secure Backend Agents;
- universal revision/idempotency and durable jobs;
- TimerIntent orchestration;
- exact full epgsearch edge parity;
- stable `/api/v1` and complete audit/accountability path;
- every specialist RESTfulAPI or Live page.

## Practical readiness

| Use case | Readiness |
| --- | --- |
| Modern Recording browser for one VDR | STRONG |
| EPG timeline/day guide/rich details | STRONG |
| Metadata/people/artwork/Genre browsing | STRONG |
| Backend-scoped Recording/EPG search | STRONG first slice |
| Safe Recording maintenance | STRONG |
| SearchTimer management | STRONG foundation; edge parity PARTIAL |
| Browser remote control | STRONG foundation |
| Complete Live replacement with streaming/OSD | NOT YET |
| Secure multi-user household | NOT YET; Phase 62 |
| Secure multi-site federation | NOT YET; Phase 63 |
| Central Timer failover | NOT YET; Phase 64 |
| Stable third-party API platform | NOT YET; Phase 67 |

## Roadmap consequence

```text
Phase 62 identity/RBAC/accountability
  -> Phase 63 secure Agents
  -> Phase 64 Timer orchestration
  -> Phase 65 streaming
  -> Phase 66 legacy OSD
  -> Phase 67 public API hardening
  -> Phase 68 recommendations
```

This document replaces older Phase 57/58 transition assumptions and includes the completed Phase 61, B1-B4, Remote and Global Search state.