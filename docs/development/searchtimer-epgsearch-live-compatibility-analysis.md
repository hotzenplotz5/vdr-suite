# SearchTimer epgsearch / Live Compatibility Analysis

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)
- [SearchTimer Real Payload Validation](searchtimer-real-payload-validation.md)
- [SearchTimer Completeness Audit](searchtimer-completeness-audit.md)

---

## Purpose

This document captures the SearchTimer compatibility concern between VDR, epgsearch, the Live-derived SearchTimer model, RESTfulAPI and VDR-Suite.

The goal is to prevent VDR-Suite from treating SearchTimer create and update as a simple REST CRUD problem.

SearchTimer behavior is defined by epgsearch semantics. RESTfulAPI is the transport boundary. VDR-Suite must therefore model and write SearchTimer rules carefully, field group by field group.

---

## Verified External Baseline

The current RESTfulAPI SearchTimer implementation is based on the Live-derived vdrlive::SearchTimer model.

The RESTfulAPI SearchTimer responder delegates create and update request bodies into vdrlive::SearchTimer::LoadFromQuery.

The external Pull Request for RESTfulAPI adds PUT /searchtimers/<id> and keeps POST /searchtimers/update as a trigger for epgsearch update execution.

Real VDR validation confirmed:

- POST /searchtimers creates SearchTimers.
- PUT /searchtimers/<id> updates SearchTimers.
- GET /searchtimers.json exposes the saved result.
- DELETE /searchtimers/<id> removes SearchTimers.

This proves the transport path, but it does not prove that VDR-Suite already writes a complete Live/epgsearch-compatible rule body.

---

## Core Risk

VDR-Suite currently has a broad read-side SearchTimer domain and mapper.

However, write-side create and update requests still use a minimal body.

This creates a compatibility gap:

- Existing SearchTimers can be read with many fields.
- New or updated SearchTimers may lose or fail to preserve advanced rule semantics.
- Wrong field combinations can trigger epgsearch validation errors.
- Missing field groups can silently fall back to defaults.
- Backend-neutral API design must not hide epgsearch-specific rule constraints.

---

## Important epgsearch / Live Rules

SearchTimer creation and update must respect these rule categories:

### Required Search Scope

A SearchTimer must define a search string and at least one search area.

Relevant fields:

- search
- use_title
- use_subtitle
- use_description

### Match Semantics

These fields define how the search expression is interpreted:

- mode
- match_case
- tolerance
- summary_match
- content_descriptors

### Time and Schedule Scope

These fields constrain event matching or recording timing:

- use_time
- start_time
- stop_time
- margin_start
- margin_stop
- use_vps

### Channel Scope

Channel handling is conditional.

Relevant fields:

- use_channel
- channel_min
- channel_max
- channels

Risk:

- use_channel interval mode requires valid channel_min and channel_max.
- channel group or selected channel modes require a channels value.
- Therefore channel enrichment must be handled separately from simple scalar fields.

### Duration and Day-of-Week Scope

Relevant fields:

- use_duration
- duration_min
- duration_max
- use_dayofweek
- dayofweek

Risk:

- Enabled duration filtering requires valid min/max values.
- Enabled day-of-week filtering requires valid bitmask semantics.

### Recording Options

Safe first enrichment group:

- directory
- priority
- lifetime

### SearchTimer Activation and Validity

Relevant fields:

- use_as_searchtimer
- use_as_searchtimer_from
- use_as_searchtimer_til
- use_in_favorites

### Repeat and Series Handling

Relevant fields:

- avoid_repeats
- allowed_repeats
- repeats_within_days
- compare_title
- compare_subtitle
- compare_summary
- compare_categories
- compare_time
- use_series_recording
- keep_recs

### Actions and Cleanup

Relevant fields:

- search_timer_action
- pause_on_recs
- switch_min_before
- unmute_sound_on_switch
- del_recs_after_days
- del_mode
- del_after_count_recs
- del_after_days_of_first_rec

### Blacklists and Extended EPG

Relevant fields:

- blacklist_mode
- blacklist_ids
- use_ext_epg_info
- ext_epg_info
- ignore_missing_epg_cats

Risk:

- These fields contain array and category semantics.
- They must not be added as casual scalar fields.

---

## Architecture Decision

VDR-Suite will not treat SearchTimer create/update enrichment as a single large JSON-body patch.

Instead, write-side enrichment must proceed in safe field groups.

Recommended order:

1. Safe scalar recording and schedule options.
2. Match semantics.
3. Time and duration constraints.
4. Channel constraints.
5. Repeat and series handling.
6. Validity and action behavior.
7. Blacklist and extended EPG metadata.

This keeps each phase testable against RESTfulAPI and real VDR behavior.

---

## Immediate Next Implementation Phase

The next implementation phase should be:

Phase 47.56 - SearchTimer series recording enrichment

Scope:

- Extend create/update request models with safe scalar fields.
- Extend create/update request parsers.
- Extend RESTfulAPI command executor body generation.
- Cover POST and PUT body output with tests.

Initial safe fields:

- directory
- priority
- lifetime
- margin_start
- margin_stop
- use_vps

Explicitly deferred:

- channel constraints
- time constraints
- duration constraints
- repeat handling
- blacklists
- extended EPG info
- action and cleanup behavior

---

## Consequence

This phase turns the earlier SearchTimer completeness audit into a stricter compatibility rule:

Read-side completeness is not enough.

Write-side compatibility must preserve epgsearch and Live semantics, otherwise VDR-Suite risks creating simplified SearchTimers that work technically but do not match user expectations from Live and epgsearch.

---

## Back

- [Back to Development Index](index.md)
- [Back to SearchTimer Completeness Audit](searchtimer-completeness-audit.md)
- [Back to SearchTimer Backend Contract](searchtimer-backend-contract.md)
