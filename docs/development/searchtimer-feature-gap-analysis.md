# SearchTimer Feature Gap Analysis

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)
- [SearchTimer Completeness Audit](searchtimer-completeness-audit.md)

---

## Purpose

This document records the SearchTimer feature gap after Phase 47.58.

The goal is to decide the next implementation phases from verified architecture rather than adding fields blindly.

---

## Current Architecture Summary

The SearchTimer read-side domain model is already broad.

It includes:

- recording options
- schedule options
- filter options
- comparison options
- repeat options
- channel options
- series options
- blacklist options
- match options
- extended EPG options
- validity options
- action options

The write path is intentionally narrower.

Create and update requests currently cover:

- base identity and query fields
- recording target fields
- schedule fields
- channel constraints
- time, duration and day-of-week constraints
- repeat handling
- comparison behavior
- series recording behavior
- blacklist behavior

---

## Field Matrix

| Feature group | Domain read model | RESTfulAPI read mapper | Result JSON | Create/Update request | Command executor write body | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Base SearchTimer identity | yes | yes | yes | yes | yes | complete |
| Recording options | yes | yes | yes | yes | yes | complete |
| Schedule options | yes | yes | yes | yes | yes | complete |
| Channel options | yes | yes | yes | yes | yes | complete |
| Time and duration options | partial read model | yes | partial | yes | yes | write supported; read model still minimal for start/stop/day-of-week details |
| Repeat options | yes | yes | yes | yes | yes | complete |
| Comparison options | yes | yes | yes | yes | yes | complete |
| Series recording options | yes | yes | yes | yes | yes | complete |
| Blacklist options | yes | yes | yes | yes | yes | complete after Phase 47.58 |
| Match options | yes | yes | yes | no | no | write gap |
| Extended EPG options | yes | yes | yes | no | no | write gap |
| Validity options | yes | yes | yes | no | no | write gap |
| Action options | yes | yes | yes | no | no | write gap |

---

## Confirmed Write Gaps

The following groups are already read, mapped into the domain and serialized back to REST clients.

They are not yet accepted in create/update requests and are not yet written to RESTfulAPI:

### Match options

RESTfulAPI field names:

- mode
- match_case
- tolerance
- summary_match

Suggested VDR-Suite request field names:

- matchMode
- matchCase
- matchTolerance
- summaryMatch

Implemented in:

- Phase 47.60 - SearchTimer match option write enrichment

Implemented in:

- Phase 47.61 - SearchTimer extended EPG write enrichment

Implemented in:

- Phase 47.62 - SearchTimer validity window write enrichment

Recommended next phase:

- Phase 47.63 - SearchTimer action option write enrichment

### Extended EPG options

RESTfulAPI field names:

- use_ext_epg_info
- ext_epg_info
- ignore_missing_epg_cats
- content_descriptors

Suggested VDR-Suite request field names:

- useExtendedEpgInfo
- extendedEpgInfo
- ignoreMissingEpgCategories
- contentDescriptors

Implemented in:

- Phase 47.60 - SearchTimer match option write enrichment

Implemented in:

- Phase 47.61 - SearchTimer extended EPG write enrichment

Implemented in:

- Phase 47.62 - SearchTimer validity window write enrichment

Recommended next phase:

- Phase 47.63 - SearchTimer action option write enrichment

### Validity options

RESTfulAPI field names:

- use_in_favorites
- use_as_searchtimer_from
- use_as_searchtimer_til

Suggested VDR-Suite request field names:

- useInFavorites
- activeFrom
- activeUntil

Recommended phase:

- Phase 47.63 - SearchTimer action option write enrichment

### Action options

RESTfulAPI field names:

- pause_on_recs
- switch_min_before
- unmute_sound_on_switch
- del_recs_after_days
- del_after_count_recs
- del_after_days_of_first_rec

Suggested VDR-Suite request field names:

- pauseOnRecordings
- switchMinutesBefore
- unmuteSoundOnSwitch
- deleteRecordingsAfterDays
- deleteAfterCountRecordings
- deleteAfterDaysOfFirstRecording

Recommended phase:

- Phase 47.63 - SearchTimer action option write enrichment

---

## Important Parser Finding

Phase 47.58 uncovered a real parser limitation.

The request parsers originally treated commas inside quoted strings as field separators.

The parser now preserves quoted commas so values such as blacklistIds = 3,7 remain intact.

This is relevant for future fields because several epgsearch options may use comma-separated or encoded value lists.

---

## Real VDR Validation Strategy

Real VDR validation should remain part of the SearchTimer workflow.

Unit tests prove request parsing and executor body generation.

The real VDR smoke helper proves that RESTfulAPI and epgsearch accept the payload and persist expected values.

For future write-enrichment phases, the recommended sequence is:

1. Extend request structs.
2. Extend create/update parsers.
3. Extend the command executor body.
4. Add parser tests.
5. Add command executor body tests.
6. Run daemon and API router tests.
7. Validate selected fields with the real VDR smoke helper when feasible.

---

## Recommended Next Phase

Phase 47.63 - SearchTimer action option write enrichment

Rationale:

- Match options are already present in the domain.
- Match options are already read by the RESTfulAPI mapper.
- Match options are already serialized in SearchTimerResult JSON.
- Match options are a compact field group and a good next candidate after blacklist support.

---

## Back

- [Back to Development Index](index.md)
- [Back to SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [Back to SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)
- [Back to SearchTimer Completeness Audit](searchtimer-completeness-audit.md)
