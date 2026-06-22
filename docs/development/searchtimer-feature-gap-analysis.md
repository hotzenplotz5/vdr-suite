# SearchTimer Feature Gap Analysis

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [SearchTimer Completeness Re-Audit](searchtimer-completeness-reaudit.md)
- [SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)
- [SearchTimer Completeness Audit](searchtimer-completeness-audit.md)

---

## Purpose

This document records the SearchTimer feature gap that was identified after Phase 47.58.

It is preserved as the historical analysis that drove the write-enrichment sequence from Phase 47.60 through Phase 47.63.

For the current post-enrichment status, use:

- [SearchTimer Completeness Re-Audit](searchtimer-completeness-reaudit.md)

---

## Original Gap Summary

The SearchTimer read-side domain model was already broad.

It included:

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

After Phase 47.58, create/update write support still missed four groups:

- match options
- extended EPG options
- validity options
- action options

---

## Resolution Matrix

| Feature group | Original status after Phase 47.58 | Resolution |
| --- | --- | --- |
| Match options | write gap | implemented in Phase 47.60 |
| Extended EPG options | write gap | implemented in Phase 47.61 |
| Validity options | write gap | implemented in Phase 47.62 |
| Action options | write gap | implemented in Phase 47.63 |

---

## Implemented Field Groups

### Match options

RESTfulAPI field names:

- mode
- match_case
- tolerance
- summary_match

VDR-Suite request field names:

- matchMode
- matchCase
- matchTolerance
- summaryMatch

Implemented in:

- Phase 47.60 - SearchTimer match option write enrichment

### Extended EPG options

RESTfulAPI field names:

- use_ext_epg_info
- ext_epg_info
- ignore_missing_epg_cats
- content_descriptors

VDR-Suite request field names:

- useExtendedEpgInfo
- extendedEpgInfo
- ignoreMissingEpgCategories
- contentDescriptors

Implemented in:

- Phase 47.61 - SearchTimer extended EPG write enrichment

### Validity options

RESTfulAPI field names:

- use_in_favorites
- use_as_searchtimer_from
- use_as_searchtimer_til

VDR-Suite request field names:

- useInFavorites
- activeFrom
- activeUntil

Implemented in:

- Phase 47.62 - SearchTimer validity window write enrichment

### Action options

RESTfulAPI field names:

- pause_on_recs
- switch_min_before
- unmute_sound_on_switch
- del_recs_after_days
- del_after_count_recs
- del_after_days_of_first_rec

VDR-Suite request field names:

- pauseOnRecordings
- switchMinutesBefore
- unmuteSoundOnSwitch
- deleteRecordingsAfterDays
- deleteAfterCountRecordings
- deleteAfterDaysOfFirstRecording

Implemented in:

- Phase 47.63 - SearchTimer action option write enrichment

---

## Parser Finding

Phase 47.58 uncovered a real parser limitation.

The request parsers originally treated commas inside quoted strings as field separators.

The parser now preserves quoted commas so values such as blacklistIds = 3,7 remain intact.

This remains important for fields that carry comma-separated or encoded value lists.

---

## Current Recommendation

The original write gaps have been closed.

The next step is no longer another blind field-enrichment phase.

Recommended next document:

- [SearchTimer Completeness Re-Audit](searchtimer-completeness-reaudit.md)

Implemented in:

- Phase 47.65 - SearchTimer full payload real VDR validation

Recommended next implementation phase:

- Phase 47.66 - SearchTimer real VDR compatibility findings

---

## Back

- [Back to Development Index](index.md)
- [Back to SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [Back to SearchTimer Completeness Re-Audit](searchtimer-completeness-reaudit.md)
- [Back to SearchTimer Completeness Audit](searchtimer-completeness-audit.md)
