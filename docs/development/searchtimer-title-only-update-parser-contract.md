# SearchTimer Title-Only Update Parser Contract

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Title-Only REST Controller Contract](searchtimer-title-only-rest-controller-contract.md)
- [SearchTimer Title-Only Request Parser Contract](searchtimer-title-only-request-parser-contract.md)
- [SearchTimer Title-Only RESTfulAPI Field Mapping](searchtimer-title-only-restfulapi-field-mapping.md)

---

## Purpose

Phase 53.3 strengthens the SearchTimer update request parser contract for title-only updates.

The update path must preserve compareTitle, compareSubtitle and compareSummary the same way as the create path.

---

## Covered JSON Contract

The update parser now has explicit tests for:

- compareTitle=true
- compareSubtitle=false
- compareSummary=false
- compareCategories=false

and for the inverse subtitle/summary combination:

- compareTitle=false
- compareSubtitle=true
- compareSummary=true
- compareCategories=false

---

## Example

A title-only update body:

- backendNativeId=searchtimer-amerika
- query=Amerika
- mode=phrase
- compareTitle=true
- compareSubtitle=false
- compareSummary=false

must produce a SearchTimerUpdateRequest with exactly those boolean values.

---

## Safety Boundary

This phase changes test coverage only.

It does not add:

- new runtime behavior
- new REST endpoint
- scheduler
- automatic execution
- backend write enablement
- production mutation policy changes

---

## Verification

Targeted verification:

- make test-search-timer-update-request-parser
- make test-search-timer-create-request-parser
- make test-restful-api-search-timer-command-executor
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 53.4 - SearchTimer title-only update controller contract

The next phase should verify that the REST controller preserves parsed title-only flags on the update path.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
