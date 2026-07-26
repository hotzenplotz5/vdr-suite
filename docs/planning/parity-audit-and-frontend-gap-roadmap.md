# VDR Ecosystem Parity and Product Gap Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)

---

## Purpose

This document records the current comparison between VDR-Suite and the principal VDR ecosystem reference points:

- VDR Core;
- the Live plugin;
- the epgsearch plugin and service interface;
- the RESTfulAPI plugin.

It answers two different questions:

1. Which capabilities are already present in VDR-Suite?
2. Is VDR-Suite already a complete replacement for each comparison target?

The answer to the second question is still no. VDR-Suite is stronger in several architecture, safety, metadata and multi-backend areas, but streaming, legacy OSD, production RBAC, secure Agents and some exact plugin semantics remain incomplete.

---

## Terminology

There is no VDR-Suite or established VDR ecosystem component named `Wikipedia Search` in this project.

The canonical comparison target is:

```text
epgsearch
```

When `Wikipedia Search` appears in conversation, it should be treated as a likely speech-to-text or naming mistake and verified rather than documented as a new product.

---

## Status Legend

```text
STRONG      implemented and well covered for the current Suite scope
PARTIAL     useful implementation exists, but exact parity or full UX is incomplete
MISSING     not implemented in VDR-Suite
DEFERRED    intentionally assigned to a later roadmap phase
DIFFERENT   not intended to copy the comparison target exactly
BETTER      VDR-Suite provides a stronger boundary or capability
```

A status is not based only on source presence. Strong claims should have domain ownership, tests, API wiring and real-system evidence where native behavior is involved.

---

## Current Project Position

```text
Latest completed runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening block:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next runtime implementation phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 61 and B1-B4 materially changed the comparison: VDR-Suite now has a persistent metadata-backed Genre runtime, provider-neutral evidence paths, indexed browse queries and production-hardened EPG/metadata refresh behavior.

---

## Executive Comparison

| Dimension | VDR Core | Live | epgsearch | RESTfulAPI | VDR-Suite |
| --- | --- | --- | --- | --- | --- |
| Native VDR authority | STRONG | Uses VDR | Uses VDR | Uses VDR | DIFFERENT: delegates native authority to VDR |
| Web user interface | None in core | STRONG | OSD/plugin focused | API only | PARTIAL to STRONG depending on workflow |
| Channels, EPG, Recordings, Timers read | Native | STRONG UI | Search-focused | STRONG API | STRONG backend and useful UI |
| Recording browser and actions | Native semantics | STRONG | Limited relevance | STRONG API | STRONG with safety gates and metadata |
| SearchTimer workflows | None in core | Integrates epgsearch | STRONG authority | Exposes service | STRONG foundation, exact semantics PARTIAL |
| Metadata, people and artwork | Plugin dependent | Plugin integration | Not primary scope | Provider-dependent exposure | BETTER provider-neutral Suite read models |
| Genre browser | Plugin/skin dependent | Limited/general | Search categories | Raw/API dependent | STRONG persistent Recording/EPG Genre runtime |
| Remote control | Native input | STRONG UI | Not primary scope | API support | STRONG foundation, live accepted |
| Live TV streaming | Native devices/providers | STRONG | Not primary scope | Provider endpoints | MISSING public Streaming Gateway; Phase 65 |
| Legacy OSD in browser | Native OSD | STRONG compatibility UI | Native OSD menus | OSD API surfaces | MISSING; Phase 66 |
| Multi-backend model | Single native instance | Single-instance oriented | Single-instance oriented | Per-plugin instance | BETTER architecture and backend policy foundation |
| Secure remote sites | External setup | Not central | Not central | Not central | DEFERRED to Phase 63 |
| User-level RBAC and audit | Limited/native config | Limited | Limited | Limited | DEFERRED to Phase 62 |
| Stable public versioned API | Native/plugin ABI | UI routes | Service commands | Plugin API | DEFERRED `/api/v1` hardening to Phase 67 |

---

# VDR Core Comparison

## Role Difference

VDR Core is the native runtime authority for devices, channels, schedules, timers, recordings, replay, OSD and plugin execution.

VDR-Suite is not intended to replace those internals. It provides an external domain, policy, metadata, orchestration and client layer while preserving VDR ownership.

## Capability Matrix

| VDR Core area | VDR-Suite status | Current assessment |
| --- | --- | --- |
| Runtime status | STRONG | Backend-aware status and health foundations exist. |
| Channels | STRONG | Read, navigation, current programme and channel movement foundations exist. |
| EPG events | STRONG | Persistent cache, time/channel windows, search, detail and Genre reads exist. |
| Recordings | STRONG | Lazy cache, folders, detail, metadata, people, artwork and guarded actions exist. |
| Timers read | STRONG | Read paths and client wrappers exist. |
| Timer create/update/delete | PARTIAL | Controlled native action paths exist; complete field-level parity still needs proof. |
| Timer flags, VPS, aux, priority, lifetime | PARTIAL | Some fields exist, but lossless universal parity is not fully documented. |
| Recording rename/move/trash | STRONG | Suite adds validation, preview, confirmation and readback. |
| Remote input | STRONG foundation | Backend-neutral actions were live accepted. |
| Native OSD | MISSING as Suite surface | VDR owns OSD; compatibility bridge is Phase 66. |
| Live/replay media path | MISSING as public Suite gateway | Phase 65 owns authenticated media sessions. |
| Plugin ecosystem | DIFFERENT | Suite uses internal adapters and SuiteBridge rather than exposing plugins publicly. |
| Multi-instance federation | BETTER target | Backend identity and read-only policy exist; secure Agents remain Phase 63. |

## Current Conclusion Versus VDR Core

VDR-Suite already covers a broad external management and browsing surface around VDR. It is not a VDR fork and should not reproduce device, recorder, scheduler or OSD internals in the control plane.

The largest core-parity risks are lossless Timer representation, uncertain native mutation outcomes and later media/OSD integration. These are explicitly assigned to Phases 62 through 67 rather than hidden behind current adapters.

---

# Live Plugin Comparison

## Where Live Is Still Stronger

Live remains the most direct comparison for an established end-user VDR web interface. It provides mature, tightly integrated workflows around a single VDR installation.

Areas where Live is still stronger or more complete:

- integrated Live TV streaming/recstream experience;
- legacy OSD access and plugin compatibility pages;
- mature Timer and SearchTimer editing workflows;
- long-established single-server setup and operational familiarity;
- some specialised pages and plugin-specific integrations.

## Where VDR-Suite Is Already Competitive or Stronger

| Live area | VDR-Suite status | Assessment |
| --- | --- | --- |
| Dashboard and backend status | STRONG | Suite is backend aware and designed for federation. |
| Channel browser | STRONG | Current programme, day navigation and modular UI exist. |
| EPG timeline | STRONG | Existing timeline, details, TVScraper tabs and Genre entry points exist. |
| Recording browser | STRONG | Lazy folders, pagination, metadata, people, artwork and actions are integrated. |
| Recording action safety | BETTER | Explicit validation, preview, confirmation, policy and readback boundaries. |
| EPG metadata detail | BETTER in current scope | Provider-neutral Suite routes, people and galleries without browser/provider coupling. |
| Genre browsing | BETTER | Persistent Recording and EPG Genre read model with canonical hierarchy. |
| SearchTimer backend/preview | STRONG foundation | CRUD, preview, validation and discovery foundations exist. |
| SearchTimer polished UI | PARTIAL | Useful workflows exist, but exact Live-style completeness is not proven. |
| Timer polished UI | PARTIAL | Read/action paths exist; complete Live workflow parity is unfinished. |
| Remote control | STRONG foundation | Backend-neutral remote actions and live overlay exist. |
| Live streaming | MISSING | Phase 65. |
| Legacy OSD page | MISSING | Phase 66. |
| Multi-backend policy | BETTER | Backend identity and server-enforced read-only mode exist. |
| Secure multi-site federation | DEFERRED | Phase 63. |
| Per-user RBAC/audit | DEFERRED | Phase 62. |

## Current Conclusion Versus Live

VDR-Suite is already a credible modern browser for channels, EPG, recordings, metadata, Genres and several control workflows. It is not yet a complete Live replacement because media streaming, legacy OSD compatibility and some mature Timer/SearchTimer workflows are missing.

The architecture is intentionally broader than Live: VDR-Suite targets several backends, explicit policy, provider-neutral metadata and future multi-client contracts. That broader target requires Phases 62 through 67 before claiming full product replacement.

---

# epgsearch Comparison

## Implemented Foundations

VDR-Suite already has substantial epgsearch integration and SearchTimer domain work:

- SearchTimer list and backend-neutral domain objects;
- create, update and delete workflows;
- RESTfulAPI command execution adapter;
- validation and dry-run boundaries;
- preview cache and invalidation;
- native preview capability handling;
- discovery catalog foundations;
- real VDR validation and readback verification;
- frontend Client API wrappers;
- automation planning and guarded execution foundations.

## Exact-Parity Matrix

| epgsearch area | VDR-Suite status | Remaining proof or work |
| --- | --- | --- |
| SearchTimer list | STRONG | Exact edge-field mapping should remain regression covered. |
| Create/update/delete | STRONG foundation | Continue native readback and failure-path proof. |
| QuerySearchTimer preview | STRONG foundation | Exact result semantics need a permanent parity matrix. |
| QuerySearch | PARTIAL | Search paths exist; full service-command equivalence is not proven. |
| Channel groups | PARTIAL | Discovery exists; completeness and UI use need proof. |
| Extended EPG info | PARTIAL | Provider contracts exist; exact semantics need proof. |
| Blacklists | PARTIAL | Discovery/mapping exists; full behavior needs proof. |
| DirectoryList / ShortDirectoryList | PARTIAL | Recording directory discovery exists; exact parity needs proof. |
| Timer conflicts | PARTIAL | Report paths exist; advice semantics and complete UI remain gaps. |
| IsConflictCheckAdvised | MISSING or unproven | Needs explicit contract and acceptance. |
| AvoidRepeats | PARTIAL | Model support exists; exact native semantics need proof. |
| Duplicate detection | PARTIAL | Foundations exist; orchestration-grade behavior is Phase 64. |
| Automatic native Timer creation | PARTIAL | Controlled paths exist; central intent/assignment orchestration is Phase 64. |
| Native OSD configuration pages | DIFFERENT/MISSING | Legacy OSD compatibility is Phase 66, not the primary Suite architecture. |

## Current Conclusion Versus epgsearch

SearchTimer is not a missing foundation. VDR-Suite can list, preview, validate and mutate SearchTimers through bounded backend-neutral workflows.

What remains is exact semantic proof and central orchestration. Phase 64 will deliberately move from provider-owned native Timer creation toward Suite-owned `TimerIntent` and `TimerAssignment` while still allowing epgsearch to produce proposals or intents.

VDR-Suite should not claim full epgsearch replacement until blacklists, extended fields, conflict advice, repeat/duplicate semantics and real automation outcomes have a strict test-and-live parity matrix.

---

# RESTfulAPI Comparison

## Current Role

RESTfulAPI is an important private backend adapter for VDR-Suite. It is not the final public platform API.

The browser communicates with Suite routes and Web Client API wrappers. RESTfulAPI URLs, credentials and provider-specific response shapes remain behind backend adapters.

## Capability Matrix

| RESTfulAPI area | VDR-Suite status | Assessment |
| --- | --- | --- |
| Status/info | STRONG | Normalized through Suite backend/runtime boundaries. |
| Channels | STRONG | Read and movement paths exist. |
| Events/EPG | STRONG | Suite cache and query models are stronger than raw passthrough. |
| Recordings read | STRONG | Lazy cache and richer Suite read models. |
| Recording operations | STRONG | Adapter-backed with Suite safety and policy gates. |
| Timers read/actions | PARTIAL to STRONG | Core paths exist; universal field parity remains incomplete. |
| SearchTimer commands | STRONG foundation | Adapter and workflow boundaries exist. |
| Remote control | STRONG foundation | Backend-neutral Suite action contract exists. |
| OSD endpoints | MISSING as public Suite feature | Deferred to isolated Phase 66 bridge. |
| Streaming/provider URLs | DIFFERENT | Must remain private; Phase 65 adds Suite media sessions. |
| Signal/Femon and specialist plugin data | MISSING or unproven | Not currently a central Suite product scope. |
| Scraper/provider data | BETTER boundary | Suite normalizes and proxies without exposing provider paths. |
| Multi-backend identity/policy | BETTER | Suite has explicit backend scope and read-only enforcement. |
| Public versioned API | DEFERRED | Phase 67 replaces current unversioned transition surface. |

## Current Conclusion Versus RESTfulAPI

VDR-Suite already covers the most important RESTfulAPI-backed reads and mutations used by the product. In several areas it is deliberately more than a proxy: caching, metadata, Genres, safety, backend scope and frontend contracts are Suite-owned.

Exact parity with every RESTfulAPI endpoint is neither proven nor always desirable. OSD, streaming, signal values and specialist plugin surfaces need explicit product decisions. Private adapter coverage must not be confused with the future stable public `/api/v1` contract.

---

# VDR-Suite Product Assessment

## Strong Today

VDR-Suite is currently strong for:

- backend-aware VDR browsing and status;
- channels and EPG navigation;
- persistent EPG cache and search;
- lazy Recording browsing at large catalogue size;
- Recording actions with safety and readback;
- metadata, people, artwork and TVScraper detail integration;
- persistent Recording and EPG Genre browsing;
- SearchTimer list, preview and controlled workflows;
- backend-neutral remote actions and live overlay;
- modular frontend and Client API boundaries;
- server-enforced read-only backend policy;
- packaging, daemon installation and real-system testing.

## Not Yet Complete

VDR-Suite is not yet complete for:

- multi-user identity, RBAC and security accountability;
- secure remote Backend Agents and multi-site command execution;
- central Timer intent, assignment, reconciliation and failover;
- authenticated Live TV and Recording streaming gateway;
- legacy OSD viewing/control compatibility;
- stable public `/api/v1`, ETags and common errors;
- full Live workflow polish;
- full epgsearch semantic parity;
- every RESTfulAPI specialist endpoint;
- recommendation and knowledge graph features.

---

## Practical Replacement View

| Use case | Current readiness |
| --- | --- |
| Modern Recording browser for one VDR | STRONG |
| EPG timeline, channel guide and rich details | STRONG |
| Metadata/people/artwork/Genre browsing | STRONG |
| Safe Recording maintenance | STRONG |
| SearchTimer preview and management | STRONG foundation, semantic edge parity PARTIAL |
| Browser remote control | STRONG foundation |
| Complete Live replacement including streaming and OSD | NOT YET |
| Secure multi-user household deployment | NOT YET; Phase 62 |
| Secure several-site federation | NOT YET; Phase 63 |
| Central multi-backend Timer failover | NOT YET; Phase 64 |
| Stable third-party public API platform | NOT YET; Phase 67 |

---

## Most Important Remaining Gaps

| Priority | Gap | Roadmap owner |
| ---: | --- | --- |
| 1 | Actor identity, RBAC and append-only accountability | Phase 62 |
| 2 | Secure Backend Agent, generation, lease and reconnect | Phase 63 |
| 3 | TimerIntent, assignment, scheduler and reconciliation | Phase 64 |
| 4 | Live TV and Recording Streaming Gateway | Phase 65 |
| 5 | Legacy OSD view/control bridge | Phase 66 |
| 6 | Versioned `/api/v1`, ETags and structured compatibility | Phase 67 |
| 7 | Exact epgsearch conflict/repeat/discovery parity tests | Phase 64 and parity backlog |
| 8 | Complete Live-style Timer/SearchTimer workflow polish | Client work after domain proof |
| 9 | Specialist RESTfulAPI surfaces such as signal/OSD decisions | Explicit scope decision |
| 10 | Recommendation and knowledge graph | Phase 68 |

---

## Audit Method

For every external feature, record:

```text
external VDR/Live/epgsearch/RESTfulAPI capability
  -> Suite domain owner
  -> private adapter boundary
  -> Suite REST and Client API contract
  -> unit/integration tests
  -> real VDR acceptance evidence where native behavior changes
  -> frontend status
  -> status: strong / partial / missing / deferred / intentionally different
```

A browser screen or adapter call alone is not parity proof.

---

## Roadmap Consequence

The next runtime phase is not another metadata or frontend catch-all.

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 62 is required before secure multi-user policy and later Agent-backed privileged operations. The following order remains authoritative:

```text
Phase 62 identity/RBAC/accountability
  -> Phase 63 secure Agents
  -> Phase 64 Timer orchestration
  -> Phase 65 streaming
  -> Phase 66 legacy OSD
  -> Phase 67 public API hardening
  -> Phase 68 recommendations
```

---

## Status

This is the maintained parity and product-gap document after the Phase 61 and B1-B4 closeout. It replaces the older assumption that Phase 57/58 transition work was still the immediate roadmap boundary.

---

## Back

- [Back to Roadmap](roadmap.md)
- [Back to Phase Map](phase-map.md)
- [Back to Current State](../CURRENT.md)
- [Back to Documentation Index](../index.md)