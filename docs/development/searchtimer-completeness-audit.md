# SearchTimer Completeness Audit

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)
- [SearchTimer Real Payload Validation](searchtimer-real-payload-validation.md)

---

## Purpose

This document captures the SearchTimer completeness audit after the initial backend-neutral SearchTimer domain expansion.

The goal is to decide whether VDR-Suite should continue adding individual SearchTimer fields or move toward SearchTimer snapshot, change detection and SSE integration.

---

## Current Implemented SearchTimer Areas

Implemented and covered by mapper, serializer and unit tests:

- identity
- backend identity
- search text
- active/inactive state
- recording options
- schedule options
- filter options
- comparison options
- repeat options
- channel options
- series options
- blacklist options

---

## Real Payload Fields Already Covered

The current VDR-Suite model covers these real RESTfulAPI or epgsearch-derived fields:

- id
- search
- use_as_searchtimer
- directory
- priority
- lifetime
- margin_start
- margin_stop
- use_vps
- use_channel
- channels
- channel_min
- channel_max
- use_dayofweek
- dayofweek
- use_duration
- duration_min
- duration_max
- use_title
- use_subtitle
- use_description
- use_time
- start_time
- stop_time
- compare_title
- compare_subtitle
- compare_summary
- compare_categories
- compare_time
- avoid_repeats
- allowed_repeats
- repeats_within_days
- use_series_recording
- keep_recs
- del_mode
- search_timer_action
- blacklist_mode
- blacklist_ids

---

## Remaining Payload Fields

Still not fully modeled as dedicated domain groups:

- mode
- match_case
- tolerance
- summary_match
- use_ext_epg_info
- ext_epg_info
- ignore_missing_epg_cats
- content_descriptors
- use_in_favorites
- use_as_searchtimer_from
- use_as_searchtimer_til
- pause_on_recs
- switch_min_before
- unmute_sound_on_switch
- del_recs_after_days
- del_after_count_recs
- del_after_days_of_first_rec

---

## Recommended Remaining Domain Groups

### Search Match Options

Suggested fields:

- mode
- match_case
- tolerance
- summary_match

Reason:

These describe how the search text is interpreted. They belong together and should not be mixed with recording or schedule rules.

### Extended EPG Options

Suggested fields:

- use_ext_epg_info
- ext_epg_info
- ignore_missing_epg_cats
- content_descriptors

Reason:

These fields are metadata/category-specific. They are important for advanced EPG and person/genre workflows, but should stay isolated from the basic SearchTimer model.

### Favorite and Validity Options

Suggested fields:

- use_in_favorites
- use_as_searchtimer_from
- use_as_searchtimer_til

Reason:

These fields define lifecycle and favorite visibility behavior.

### Action and Cleanup Options

Suggested fields:

- pause_on_recs
- switch_min_before
- unmute_sound_on_switch
- del_recs_after_days
- del_after_count_recs
- del_after_days_of_first_rec

Reason:

These fields are side-effect and cleanup behavior. They should be modeled before write operations, because create/update must preserve them.

---

## Decision

SearchTimer is broad enough for listing and read-side API integration.

Before snapshot, change feed and SSE are implemented, VDR-Suite should add the remaining four domain groups:

1. SearchTimerMatchOptions
2. SearchTimerExtendedEpgOptions
3. SearchTimerValidityOptions
4. SearchTimerActionOptions

After the real RESTfulAPI create/update/delete validation, SearchTimer write-side enrichment must be handled as a separate compatibility track before broader runtime integration:

1. SearchTimer epgsearch / Live compatibility analysis.
2. Safe create/update body enrichment.
3. Conditional rule groups for channel, time, duration, repeat, blacklist and extended EPG behavior.
4. SearchTimer snapshot.
5. SearchTimer change detection.
6. SearchTimer change feed.
7. SearchTimer SSE events.

---

## LIVE Superset Impact

This audit confirms the current path:

- LIVE remains the functional reference.
- VDR-Suite does not proxy OSD workflows.
- VDR-Suite models SearchTimer as a backend-neutral domain object.
- Advanced person, genre and metadata workflows should build on the SearchTimer domain model rather than copying LIVE forms.

---

## Back

- [Back to Development Index](index.md)
- [Back to SearchTimer Backend Contract](searchtimer-backend-contract.md)
- [Back to SearchTimer Real Payload Validation](searchtimer-real-payload-validation.md)
- [Back to README](../../README.md)
