# SearchTimer Completeness Re-Audit

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [SearchTimer Feature Gap Analysis](searchtimer-feature-gap-analysis.md)
- [SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)

---

## Purpose

This document re-audits SearchTimer completeness after the write-enrichment sequence through Phase 47.63.

The previous feature-gap analysis identified four write-side gaps:

- match options
- extended EPG options
- validity options
- action options

All four groups are now implemented in the create/update write path.

---

## Current Completion Matrix

| Feature group | Read model | RESTfulAPI read mapper | Result JSON | Create request | Update request | RESTfulAPI write body | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Base SearchTimer identity | yes | yes | yes | yes | yes | yes | complete |
| Query/search text | yes | yes | yes | yes | yes | yes | complete |
| Recording directory | yes | yes | yes | yes | yes | yes | complete |
| Priority and lifetime | yes | yes | yes | yes | yes | yes | complete |
| Margin start/stop | yes | yes | yes | yes | yes | yes | complete |
| VPS flag | yes | yes | yes | yes | yes | yes | complete |
| Channel constraints | yes | yes | yes | yes | yes | yes | complete |
| Time constraints | write-focused | yes | partial | yes | yes | yes | write complete; read model still minimal |
| Duration constraints | write-focused | yes | partial | yes | yes | yes | write complete; read model still minimal |
| Day-of-week constraints | write-focused | yes | partial | yes | yes | yes | write complete; read model still minimal |
| Repeat handling | yes | yes | yes | yes | yes | yes | complete |
| Comparison behavior | yes | yes | yes | yes | yes | yes | complete |
| Series recording | yes | yes | yes | yes | yes | yes | complete |
| Blacklist options | yes | yes | yes | yes | yes | yes | complete |
| Match options | yes | yes | yes | yes | yes | yes | complete after Phase 47.60 |
| Extended EPG options | yes | yes | yes | yes | yes | yes | complete after Phase 47.61 |
| Validity window options | yes | yes | yes | yes | yes | yes | complete after Phase 47.62 |
| Action options | yes | yes | yes | yes | yes | yes | complete after Phase 47.63 |

---

## Confirmed Write-Side Completion

The SearchTimer write body now covers the feature groups that were previously missing:

### Match options

RESTfulAPI fields:

- mode
- match_case
- tolerance
- summary_match

VDR-Suite request fields:

- matchMode
- matchCase
- matchTolerance
- summaryMatch

Implemented in:

- Phase 47.60 - SearchTimer match option write enrichment

### Extended EPG options

RESTfulAPI fields:

- use_ext_epg_info
- ext_epg_info
- ignore_missing_epg_cats
- content_descriptors

VDR-Suite request fields:

- useExtendedEpgInfo
- extendedEpgInfo
- ignoreMissingEpgCategories
- contentDescriptors

Implemented in:

- Phase 47.61 - SearchTimer extended EPG write enrichment

### Validity window options

RESTfulAPI fields:

- use_in_favorites
- use_as_searchtimer_from
- use_as_searchtimer_til

VDR-Suite request fields:

- useInFavorites
- activeFrom
- activeUntil

Implemented in:

- Phase 47.62 - SearchTimer validity window write enrichment

### Action options

RESTfulAPI fields:

- pause_on_recs
- switch_min_before
- unmute_sound_on_switch
- del_recs_after_days
- del_after_count_recs
- del_after_days_of_first_rec

VDR-Suite request fields:

- pauseOnRecordings
- switchMinutesBefore
- unmuteSoundOnSwitch
- deleteRecordingsAfterDays
- deleteAfterCountRecordings
- deleteAfterDaysOfFirstRecording

Implemented in:

- Phase 47.63 - SearchTimer action option write enrichment

---

## Remaining Technical Gaps

No major write-side SearchTimer feature group remains open from the Phase 47.59 gap analysis.

The remaining gaps are not simple field-enrichment gaps. They are integration and validation gaps:

1. Real VDR validation round 2 for the enriched full payload.
2. SearchTimer create/update/delete integration tests through the daemon API.
3. SearchTimer preview behavior against real EPG data.
4. SearchTimer list/detail read API hardening.
5. SearchTimer result-to-timer/recording relationship modeling.
6. Capability reporting for backend-specific SearchTimer support.

---

## Recommendation

The write-enrichment sequence should be considered complete after Phase 47.64.

The next implementation focus should move from adding individual fields to validating the full SearchTimer lifecycle.

Recommended next phase:

Phase 47.67 - Add real VDR read-only regression helper

Purpose:

- run the real VDR smoke helper with the enriched full payload
- verify create, readback, update and delete behavior
- document which advanced fields persist exactly and which RESTfulAPI/epgsearch normalizes
- prepare the transition from field completeness to lifecycle completeness

---

## Back

- [Back to Development Index](index.md)
- [Back to SearchTimer Feature Gap Analysis](searchtimer-feature-gap-analysis.md)
- [Back to SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [Back to SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)
