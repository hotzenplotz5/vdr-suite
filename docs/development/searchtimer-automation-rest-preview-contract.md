# SearchTimer Automation REST Preview Contract

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Automation Foundation Planning](searchtimer-automation-foundation-planning.md)
- [SearchTimer Automation Read-Only Service Boundary](searchtimer-automation-read-only-service-boundary.md)

---

## Purpose

Phase 52.7 exposes the read-only SearchTimer automation dry-run result through a REST preview contract.

The endpoint renders a dry-run JSON result without matching execution, scheduling, timer creation or backend mutation.

---

## Endpoints

- GET /api/searchtimers/automation/preview
- GET /api/vdr/searchtimers/automation/preview

Supported query parameters:

- backend
- limit

---

## Safety Contract

- dryRunOnly remains true
- mutationAllowed remains false
- timerCreationAllowed remains false
- backendWriteAllowed remains false
- automaticExecutionAllowed remains false
- requiresExplicitExecutionHandoff remains true

The controller only renders a preview. It does not create or modify VDR timers.

---

## Runtime Wiring

Daemon runtime now wires the preview controller so the read-only endpoint is available from the daemon API.

---

## Boundaries

This phase intentionally does not add:

- EPG matching execution
- duplicate detection execution
- candidate timer generation logic
- scheduler
- RESTfulAPI write call
- epgsearch mutation
- production execution enablement

---

## Verification

- make test-search-timer-automation-preview-controller
- make test-api-router
- make test-search-timer-automation-read-only-service
- make test-search-timer-automation-dry-run-result-json-serializer
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 52.8 - SearchTimer automation daemon scheduling plan

The next phase should document scheduler boundaries and safety switches before any scheduled execution is introduced.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
