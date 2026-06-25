# Live Plugin Parity Source Audit

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [Live / EPGSearch Feature Inventory](live-feature-inventory.md)
- [EPGSearch Capability Matrix](epgsearch-capability-matrix.md)

---

## Purpose

Phase 51.0 starts the Live Plugin Parity Foundation.

The goal is not to copy the Live plugin UI and not to fork VDR.

The goal is to map the practical information quality of Live onto the existing VDR-Suite architecture:

- VDR remains the source of truth for VDR-owned state.
- epgsearch remains the source of truth for SearchTimer semantics.
- RESTfulAPI remains the preferred transport boundary for yaVDR and classic VDR environments.
- VDR-Suite remains backend-neutral, multi-backend-capable and API-first.

---

## Verified Phase 50 Baseline

Phase 50 completed the SearchTimer User Workflow Foundation.

Completed foundation capabilities:

- backend-neutral SearchTimer workflow request, validation and planning models
- guarded create, update and delete workflow semantics
- explicit operator confirmation for write workflows
- executor opt-in and controlled executor injection gates
- backend write allowlist and per-backend permission gate foundations
- closed production policy gate
- executor invocation kill-switch behavior
- create and update backend readback verification
- delete absence verification
- dispatch integration for readback services
- REST-visible verified execution result contract
- dedicated end-to-end verified execution test

Important safety result:

- Production mutation remains closed.
- Real backend mutation is still not enabled by Phase 51.0.

---

## Source Audit Result

### VDR Core

VDR owns the core runtime state:

- channels
- schedules and events
- timers
- recordings
- recording metadata files
- VPS, running status, timer flags, weekdays, remote timer metadata and recording usage state

VDR does not own epgsearch SearchTimers as a core object.

Consequence:

- Live parity should not start with a VDR fork.
- VDR-Suite should enrich VDR-owned event, timer and recording detail models where useful.
- VDR-Suite should keep VDR state read and write paths separated from epgsearch-specific SearchTimer state.

### epgsearch

epgsearch owns the SearchTimer semantics.

Important epgsearch service areas:

- SearchTimerList
- AddSearchTimer
- ModSearchTimer
- DelSearchTimer
- QuerySearchTimer
- QuerySearch
- ExtEPGInfoList
- ChanGrpList
- BlackList
- DirectoryList
- TimerConflictList
- IsConflictCheckAdvised
- ShortDirectoryList
- Evaluate

The epgsearch SearchTimer model is field-rich and rule-based.

Important SearchTimer field groups:

- search expression and search mode
- title, subtitle and description field selection
- fuzzy tolerance and case handling
- channel interval, channel group and free-to-air handling
- time window, duration window and day-of-week handling
- recording directory, priority, lifetime, margins and VPS
- SearchTimer action mode
- series recording and repeat avoidance
- comparison flags
- blacklist mode and selected blacklist ids
- extended EPG categories and matching mode
- content descriptors and parental rating
- validity windows and favorites
- retention and deletion policy

Consequence:

- SearchTimer create and update must be preserve-first.
- Existing unknown or not-yet-modeled epgsearch fields must not be silently dropped.
- Write-side enrichment must proceed by safe field groups.

### RESTfulAPI

RESTfulAPI already exposes or can expose much of the Live/epgsearch surface.

Relevant RESTfulAPI SearchTimer surfaces:

- SearchTimer list
- SearchTimer create
- SearchTimer update
- SearchTimer delete
- SearchTimer result query
- channel group list
- recording directory list
- blacklist list
- timer conflict list
- extended EPG info list
- SearchTimer update trigger

Consequence:

- The preferred direction remains RESTfulAPI integration and extension, not a VDR fork.
- VDR-Suite should turn these RESTfulAPI surfaces into backend-neutral domain, service and JSON contracts.
- Real yaVDR read-only validation should precede write-side expansion.

### Live Plugin

Live is the practical parity reference because it combines:

- EPG search
- SearchTimer list, edit, toggle, delete and update actions
- SearchTimer result browsing
- Timer conflict visibility
- event detail views
- timer and recording context
- extended EPG and blacklist helpers
- recording directory helpers
- user-facing workflow grouping

Consequence:

- VDR-Suite should aim for Live-level information quality.
- VDR-Suite should not copy Live's server-side HTML UI.
- VDR-Suite should expose frontend-ready JSON contracts that can support web, TV, desktop and mobile clients.

---

## VDR-Suite Gap Matrix

| Area | Current VDR-Suite state | Live parity gap | Phase 51 direction |
| --- | --- | --- | --- |
| SearchTimer workflow | Completed Phase 50 foundation | Full Live-style field semantics still incomplete | Preserve-first enrichment and source-audited field groups |
| SearchTimer preview | Basic preview exists | epgsearch QuerySearchTimer / QuerySearch parity missing | Compare VDR-Suite preview with epgsearch result semantics |
| Extended EPG info | Not first-class | ExtEPGInfoList not exposed as VDR-Suite domain | Add read-only discovery domain and JSON contract |
| Channel groups | Not first-class | epgsearch channel groups not exposed | Add read-only discovery domain and JSON contract |
| Blacklists | Fields exist, list capability not first-class | BlackList discovery missing | Add read-only discovery domain and JSON contract |
| Recording directories | Partially present elsewhere | DirectoryList and ShortDirectoryList not first-class SearchTimer helpers | Add discovery contract before write usage |
| Timer conflicts | Missing as domain | TimerConflictList and IsConflictCheckAdvised missing | Add conflict domain and read-only endpoint before any conflict resolution |
| EPG event detail | Compact event model | VDR event details like VPS, running status, components, aux and seen are not fully represented | Prioritize fields needed by Live-style client workflows |
| Timer detail | Compact timer model | flags, weekdays, remote, event binding and file semantics are incomplete | Prioritize conflict and frontend display needs |
| Recording detail | Compact recording model | resume, marks, usage, info file details and edit state are incomplete | Prioritize visible metadata and safety state |
| Authorization | Planned future concern | Live has user rights around SearchTimer editing and deletion | Keep policy out until dedicated profile and permission architecture |
| Automation | Planned Phase 52 | Automatic SearchTimer evaluation is not part of Live parity kickoff | Defer to Phase 52 |

---

## Implementation Order Decision

Phase 51 should start with read-only discovery and information quality.

Recommended order:

1. Discovery helper domains for Extended EPG info, channel groups, blacklists and recording directories.
2. Timer conflict domain and JSON contract.
3. Real yaVDR read-only validation for discovery endpoints.
4. SearchTimer preview parity audit against epgsearch QuerySearchTimer and QuerySearch.
5. Preserve-first SearchTimer update strategy.
6. EPG, timer and recording detail model enrichment based on concrete frontend and conflict-display needs.

---

## Deferred Scope

The following remain intentionally deferred:

- production SearchTimer mutation enablement
- automatic SearchTimer evaluation
- automatic timer creation
- automatic conflict resolution
- duplicate detection and match history
- profile-specific write permissions
- full frontend implementation
- VDR fork work

---

## Phase 51.0 Result

Phase 51.0 completes the Live plugin parity foundation kickoff.

Completed in this phase:

- Verified the source ownership split between VDR, epgsearch, RESTfulAPI, Live and VDR-Suite.
- Confirmed that RESTfulAPI integration and extension is the preferred path over a VDR fork.
- Defined the first Live parity gap matrix for VDR-Suite.
- Confirmed that production mutation remains closed.
- Selected Phase 51.1 as the first implementation step: read-only Live parity discovery API domain foundation.

---

## Next Phase

Phase 51.1 - Live parity discovery API domain foundation

Initial read-only candidates:

- Extended EPG info
- Channel groups
- Blacklists
- Recording directories

The first implementation must stay read-only and backend-neutral.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
