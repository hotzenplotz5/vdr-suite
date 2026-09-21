# SearchTimer Title-Only REST Controller Contract

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Title-Only Request Parser Contract](searchtimer-title-only-request-parser-contract.md)
- [SearchTimer Title-Only RESTfulAPI Field Mapping](searchtimer-title-only-restfulapi-field-mapping.md)

---

## Purpose

Phase 53.2 verifies the REST controller handoff for title-only SearchTimer creation.

The controller must parse the incoming JSON body and pass the preserved compareTitle, compareSubtitle and compareSummary flags into the create path.

---

## Covered Path

- SearchTimerController::createSearchTimer
- SearchTimerCreateRequestParser
- SearchTimerCreateService
- ISearchTimerCommandExecutor create handoff

---

## Title-Only Contract

The controller test now verifies this external JSON body:

- query=Amerika
- mode=phrase
- compareTitle=true
- compareSubtitle=false
- compareSummary=false
- compareCategories=false

The captured create request must preserve exactly those flags.

---

## Safety Boundary

This phase changes test coverage only.

It does not add:

- new REST endpoint
- scheduler
- automatic execution
- backend write enablement
- production mutation policy changes

---

## Verification

Targeted verification:

- make test-search-timer-controller
- make test-search-timer-create-request-parser
- make test-restful-api-search-timer-command-executor
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 53.3 - SearchTimer title-only update parser contract

The next phase should verify the same field preservation on the update parser path.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
