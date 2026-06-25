# SearchTimer Title-Only Request Parser Contract

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Title-Only RESTfulAPI Field Mapping](searchtimer-title-only-restfulapi-field-mapping.md)

---

## Purpose

Phase 53.1 strengthens the REST request parser contract for title-only SearchTimer creation.

The goal is to guarantee that external JSON bodies preserve title, subtitle and summary search flags before the request reaches the RESTfulAPI executor.

---

## Covered JSON Contract

The parser now has explicit tests for:

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

A request body for title-only Amerika:

- query=Amerika
- mode=phrase
- compareTitle=true
- compareSubtitle=false
- compareSummary=false

must produce a SearchTimerCreateRequest with exactly those boolean values.

---

## Safety Boundary

This phase changes tests and build coverage only.

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

- make test-search-timer-create-request-parser
- make test-restful-api-search-timer-command-executor
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 53.2 - SearchTimer title-only REST controller contract

The next phase should verify that the REST controller passes the parsed title-only flags into the workflow/create path without changing them.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
