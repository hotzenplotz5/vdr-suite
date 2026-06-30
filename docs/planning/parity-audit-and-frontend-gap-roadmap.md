# Parity Audit and Frontend Gap Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Roadmap](roadmap.md)
- [Phase Map](phase-map.md)

---

## Purpose

This document records the current parity view between VDR-Suite and the main VDR ecosystem reference points.

It is not a claim that all parity is complete.

It is a planning matrix for deciding what should be proven, implemented, deferred or intentionally left out before a 1.0 product boundary.

Reference targets:

- VDR RESTfulAPI plugin
- VDR Live plugin
- epgsearch plugin service interface
- VDR core timer and recording semantics

---

## Legend

```text
OK      present or likely close to parity
PARTIAL partly present or parity not fully proven
MISSING not seen or likely absent
CHECK   unclear and should be audited
BETTER  VDR-Suite is stronger than the comparison target
```

---

## Current High-Level Assessment

VDR-Suite is no longer only a backend prototype.

The project already has strong foundations for:

- RESTfulAPI integration
- recording actions
- recording action safety
- SearchTimer CRUD and preview
- SearchTimer dry-run and validation flows
- real VDR acceptance checks
- backend-neutral service boundaries
- multi-backend architecture
- packaging and install staging

The main remaining work is now less about basic feasibility and more about proof and product completeness:

- prove exact parity where it matters
- identify gaps with tests and real VDR acceptance evidence
- decide what belongs to version 1.0
- expose existing backend capability through a usable frontend

---

## RESTfulAPI Parity View

| Area | VDR-Suite status | Notes |
| --- | --- | --- |
| VDR status and info | OK | Core read parity likely. |
| Channels read | OK | Core read parity likely. |
| Events and EPG read | OK | Core read parity likely. |
| EPG search | OK | Own EPG search block exists from Phase 45. |
| Recordings read | OK | Core read parity likely. |
| Recording query | OK | VDR-Suite likely stronger because of query/domain work. |
| Recording move, rename and write operations | OK | Strong but policy guarded. |
| Recording action preview | BETTER | Preview and safety model are VDR-Suite strengths. |
| Recording mutation safety | BETTER | Safety gates and dry-run policy are central. |
| Timers read | OK | Present, but detail parity still needs audit. |
| Timer write operations | PARTIAL | Present or planned, exact completeness should be checked. |
| SearchTimer create/update/delete | OK | Strong SearchTimer backend work exists. |
| SearchTimer search and preview | PARTIAL | Present, but exact epgsearch semantics should be proven. |
| SearchTimer discovery catalogs | PARTIAL | Provider and wiring are present, catalog completeness needs audit. |
| Blacklists | PARTIAL | Support likely exists, exact list parity should be proven. |
| Channel groups | PARTIAL | Discovery likely exists, exact list parity should be proven. |
| Extended EPG info | PARTIAL | Support likely exists, exact list parity should be proven. |
| Recording directories | CHECK | Targeted audit required. |
| Timer conflicts | CHECK | Targeted audit required. |
| OSD | CHECK | Not confirmed as current VDR-Suite scope. |
| Remote control | CHECK | Not confirmed as current VDR-Suite scope. |
| Femon / signal values | CHECK | Not confirmed as current VDR-Suite scope. |
| Scraper images | PARTIAL | Metadata and TVScraper context exists, image parity unclear. |
| Wirbelscan | MISSING | Probably not a core VDR-Suite goal. |
| Multi-backend | BETTER | VDR-Suite is stronger here. |
| Backend-specific policy | BETTER | VDR-Suite has a stronger target model. |

---

## Live Plugin Parity View

Live is strong because it has a complete web user interface.

VDR-Suite is stronger architecturally, but not yet a Live replacement for end users.

| Live area | VDR-Suite status | Notes |
| --- | --- | --- |
| Recording browser backend | OK | Backend foundation present. |
| Recording actions | OK | Strong backend and safety model. |
| Timer backend | PARTIAL | Present, detail audit required. |
| EPG backend | OK | Present. |
| SearchTimer backend | OK | Strong backend work exists. |
| SearchTimer preview | OK | Present. |
| Live parity discovery | OK | Phase 51 exists for discovery. |
| Web UI | MISSING | Major product gap. |
| Recording web UI | MISSING | Needed for user parity. |
| Timer web UI | MISSING | Needed for user parity. |
| SearchTimer web UI | MISSING | Needed for user parity. |
| EPG day view / What's On | PARTIAL | API may support it, UI does not. |
| Multischedule | CHECK | Needs audit. |
| Remote page | CHECK | Needs scope decision. |
| OSD page | CHECK | Needs scope decision. |
| Setup page | CHECK | Needs scope decision. |
| Timer conflicts page | CHECK | Needs backend and UI audit. |
| recstream / streaming | CHECK | Needs concept and scope decision. |
| SSE / live transport | OK | Present, but not identical to Live streaming. |
| Multi-backend | BETTER | VDR-Suite target is stronger. |
| Read-only backend policy | BETTER | VDR-Suite target is stronger. |

---

## epgsearch Parity View

SearchTimer is no longer a missing foundation.

The remaining question is exact semantic compatibility with epgsearch behavior.

| epgsearch area | VDR-Suite status | Notes |
| --- | --- | --- |
| SearchTimer domain | OK | Present. |
| SearchTimer create/update/delete | OK | Present. |
| RESTfulAPI command executor | OK | Present. |
| Payload body enrichment | OK | Strong work exists. |
| Channel, time, duration, repeat, series, validity and action body | OK | Present or strongly indicated by phase work. |
| Blacklist support | PARTIAL | Completeness should be proven. |
| Extended EPG support | PARTIAL | Completeness should be proven. |
| SearchTimer preview | OK | Present. |
| Preview cache and invalidation | OK | Present. |
| Typo/fallback preview | OK | Present. |
| Native preview capability | OK | Present. |
| Real VDR validation | OK | Present. |
| Readback verification | OK | Present. |
| SearchTimer discovery catalogs | PARTIAL | Catalog completeness needs audit. |
| SearchTimerList | CHECK | Needs exact mapping proof. |
| QuerySearchTimer | CHECK | Preview exists, exact semantics need proof. |
| QuerySearch | CHECK | Needs proof against epgsearch behavior. |
| ExtEPGInfoList | CHECK | Needs proof. |
| ChanGrpList | CHECK | Needs proof. |
| BlackList | CHECK | Needs proof. |
| DirectoryList / ShortDirectoryList | CHECK | Important target gap. |
| TimerConflictList | CHECK | Important target gap. |
| IsConflictCheckAdvised | CHECK | Important target gap. |
| Evaluate expression against event | CHECK | Not confirmed. |
| Automatic timer creation from automation | PARTIAL | Dry-run and proposal flows exist; execute path must be proven. |
| Duplicate detection | PARTIAL | Model exists, real behavior should be proven. |
| AvoidRepeats semantics | CHECK | Needs exact semantic proof. |

---

## VDR Core Timer Parity View

VDR-Suite abstracts VDR already.

The open question is whether all relevant core timer details are represented losslessly.

| VDR core area | VDR-Suite status | Notes |
| --- | --- | --- |
| Status, channels, events, recordings | OK | Present. |
| Timers read | OK | Present. |
| Timer parser / request builder | OK | Timer parser source split exists in Phase 56 history. |
| Timer write operations | PARTIAL | Needs exact detail audit. |
| Timer flags | CHECK | Needs field-level audit. |
| Priority and lifetime | PARTIAL | Present in related contexts, exact parity should be proven. |
| VPS | CHECK | Needs field-level audit. |
| Aux | CHECK | Needs field-level audit. |
| Remote/local timer semantics | CHECK | Needs field-level audit. |
| Pattern/spawned timers | CHECK | Needs field-level audit. |
| Event matching | PARTIAL | EPG and preview exist, exact VDR-core behavior should be proven. |
| Conflict/overlap behavior | CHECK | Needs audit. |
| Multi-backend beyond VDR core | BETTER | VDR-Suite is stronger here. |

---

## Most Important Real Gaps

| Priority | Gap | Why it matters |
| ---: | --- | --- |
| 1 | Timer conflict list and conflict checking | Central for Live and epgsearch parity. |
| 2 | QuerySearchTimer semantic proof | Preview exists, but parity needs evidence. |
| 3 | DirectoryList and ShortDirectoryList | Important for SearchTimer and recording targets. |
| 4 | ExtEPGInfo, channel groups and blacklists | Likely partly present, but matrix proof is missing. |
| 5 | Real timer creation from SearchTimer automation | Dry-run is not full parity. |
| 6 | VDR core timer details | Needed for lossless timer handling. |
| 7 | Live-like web UI | Largest end-user parity gap. |
| 8 | Streaming / recstream | Important for Live parity if in scope. |
| 9 | OSD, remote control and signal values | RESTfulAPI and Live parity areas, but may be optional. |
| 10 | Hard parity matrix with tests and real VDR evidence | Prevents guessing and stale claims. |

---

## Roadmap Consequence

Do not skip Phase 57.

Phase 57 is the backend and policy foundation that the frontend needs:

- backend identity
- backend selection
- backend-specific permissions
- read-only secondary-site mode
- visible capability and policy state

After Phase 57, Phase 58 should become the frontend and Live-parity validation block.

The frontend should be a thin usability and parity layer first, not a large UI rewrite.

Recommended Phase 58 MVP:

- dashboard
- backend selector
- backend status
- channels
- EPG/events
- recordings
- recording action preview
- recording action execution with safety gate
- SearchTimer list and discovery
- SearchTimer preview
- SearchTimer dry-run and automation preview

---

## Audit Method

For each external feature, record:

```text
External feature
  -> VDR-Suite domain class or service
  -> REST route or command surface
  -> unit or integration test
  -> real VDR acceptance evidence
  -> UI status
  -> status: done / partial / missing / deferred / intentionally out of scope
```

This is the basis for deciding the 1.0 feature boundary.

---

## Status

This is a planning document.

It should be converted into a stricter parity matrix during the Phase 57 to Phase 58 transition.

---

## Back

- [Back to Roadmap](roadmap.md)
- [Back to Phase Map](phase-map.md)
- [Back to Current State](../CURRENT.md)
- [Back to Documentation Index](../index.md)
