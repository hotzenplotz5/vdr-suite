# EPGSearch Result Model Audit

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [EPGSearch Capability Matrix](epgsearch-capability-matrix.md)
- [Live / EPGSearch Feature Inventory](live-feature-inventory.md)
- [Current Status](current-status.md)

---

## Purpose

Phase 48.3 audits the existing EPGSearch result domain model.

The original expectation was that a new result model might be required after the Phase 48.2 query model. Source inspection showed that VDR-Suite already contains:

- `EpgSearchMatch`
- `EpgSearchResult`

Therefore this phase documents whether the existing model is sufficient for the Live/EPGSearch roadmap.

---

## Existing Model

### EpgSearchMatch

Current responsibilities:

- wraps a `VdrEvent`
- carries optional backend identity
- carries optional matched field names

This is a good backend-neutral foundation because the event remains the primary domain object and backend-specific match metadata is kept separate.

### EpgSearchResult

Current responsibilities:

- stores a list of `EpgSearchMatch`
- stores total count
- stores returned count through match vector size
- stores limit
- stores offset
- provides empty result construction

This is a good result container for paged EPGSearch-style responses.

---

## Capability Fit

| Requirement | Existing model status | Notes |
| --- | --- | --- |
| Event payload | covered | via `VdrEvent` inside `EpgSearchMatch` |
| Backend identity | covered | via optional backendId |
| Match metadata | partially covered | matchedFields are present |
| Total result count | covered | `totalCount` |
| Pagination | covered | `limit` and `offset` |
| Empty result | covered | `EpgSearchResult::empty` |
| Extended EPG category match data | not explicit | can be added later if needed |
| Search score / rank | missing | not required yet |
| Conflict metadata | missing | should remain separate domain |
| Timer/searchtimer relation flags | missing | should be evaluated in a later phase |
| TVScraper enrichment | missing | should not be added to base result model yet |

---

## Decision

Do not replace `EpgSearchResult`.

The existing model is sufficient for the next EPGSearch phases.

Phase 48.3 should not add new product code. It documents the result-domain audit and prevents duplicate model creation.

---

## Recommended Next Phase

Phase 48.4 - EPGSearch service interface

Rationale:

- Query model now exists.
- Result model already exists.
- The next missing abstraction is a backend-neutral service boundary.

Expected interface shape:

- `EpgSearchResult search(const EpgSearchQuery& query) const;`

No REST endpoint or real adapter should be added before the service boundary exists.

## Back

- [Back to Development Index](index.md)
- [Back to EPGSearch Capability Matrix](epgsearch-capability-matrix.md)
