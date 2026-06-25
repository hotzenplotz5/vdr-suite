# SearchTimer Title-Only Workflow Request Contract

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Title-Only Update Controller Contract](searchtimer-title-only-update-controller-contract.md)
- [SearchTimer Title-Only Update Parser Contract](searchtimer-title-only-update-parser-contract.md)
- [SearchTimer Title-Only REST Controller Contract](searchtimer-title-only-rest-controller-contract.md)
- [SearchTimer Title-Only Request Parser Contract](searchtimer-title-only-request-parser-contract.md)
- [SearchTimer Title-Only RESTfulAPI Field Mapping](searchtimer-title-only-restfulapi-field-mapping.md)

---

## Purpose

Phase 53.5 adds title-only field preservation to the workflow request and planning contract.

The workflow parser must preserve compareTitle, compareSubtitle and compareSummary before any future automation or execution path maps workflow plans into concrete SearchTimer create/update requests.

---

## Covered Path

- SearchTimerWorkflowRequest
- SearchTimerWorkflowValidationRequestParser
- SearchTimerWorkflowExecutionPlan::fromRequest
- workflow request, parser and execution plan tests

---

## Title-Only Contract

The workflow request path now preserves:

- compareTitle=true
- compareSubtitle=false
- compareSummary=false
- compareCategories=false

for both create and update workflow requests.

---

## Safety Boundary

This phase changes request/plan state and test coverage only.

It does not add:

- new REST endpoint
- scheduler
- automatic execution
- backend write enablement
- production mutation policy changes

---

## Verification

Targeted verification:

- make test-search-timer-workflow-request
- make test-search-timer-workflow-execution-plan
- make test-search-timer-workflow-validation-request-parser
- make test-search-timer-controller
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 53.6 - SearchTimer title-only workflow command mapper contract

The next phase should verify that workflow execution plans map preserved title-only flags into concrete SearchTimerCreateRequest and SearchTimerUpdateRequest objects.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
