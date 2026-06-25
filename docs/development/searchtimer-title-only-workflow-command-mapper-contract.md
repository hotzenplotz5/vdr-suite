# SearchTimer Title-Only Workflow Command Mapper Contract

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Title-Only Workflow Request Contract](searchtimer-title-only-workflow-request-contract.md)
- [SearchTimer Title-Only RESTfulAPI Field Mapping](searchtimer-title-only-restfulapi-field-mapping.md)

---

## Purpose

Phase 53.6 verifies that preserved workflow compare fields are mapped into concrete SearchTimer create and update command requests.

Without this mapping, workflow automation would still lose title-only intent before backend command execution.

---

## Covered Path

- SearchTimerWorkflowExecutionPlan
- SearchTimerWorkflowCommandRequestMapper::buildCreateRequest
- SearchTimerWorkflowCommandRequestMapper::buildUpdateRequest
- SearchTimerCreateRequest
- SearchTimerUpdateRequest

---

## Title-Only Contract

For workflow create and update plans, the command mapper must preserve:

- compareTitle=true
- compareSubtitle=false
- compareSummary=false
- compareCategories=false

into the concrete command request objects.

---

## Safety Boundary

This phase changes mapping and test coverage only.

It does not add new REST endpoints, scheduling, automatic execution, backend write enablement or production mutation policy changes.

---

## Verification

- make test-search-timer-workflow-command-request-mapper
- make test-search-timer-workflow-request
- make test-search-timer-workflow-execution-plan
- make test-search-timer-workflow-validation-request-parser
- make test-restful-api-search-timer-command-executor
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 53.7 - SearchTimer title-only workflow execution dispatch contract

The next phase should verify the dispatch path from workflow execution into the command executor keeps the mapped title-only command request intact while production mutation remains gated.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
