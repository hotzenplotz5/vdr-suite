# SearchTimer Automation Evaluation Plan Model

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Automation Foundation Planning](searchtimer-automation-foundation-planning.md)

---

## Purpose

Phase 52.1 adds the first read-only model for SearchTimer automation.

The model describes how an automation evaluation should be planned without evaluating EPG data, creating candidates or performing mutation.

---

## Model

Class:

- SearchTimerAutomationEvaluationPlan

Factory:

- createReadOnly(backendId)

Core fields:

- backendId
- candidateLimit
- includeInactiveSearchTimers
- includeExistingTimers
- includeRecordings

---

## Safety Contract

Every evaluation plan is currently read-only:

- dryRunOnly() is always true
- mutationAllowed() is always false
- scheduledExecutionAllowed() is always false
- requiresExplicitExecutionHandoff() is always true

This makes the model safe to use before any daemon scheduler or real execution phase exists.

---

## Validation

A valid plan requires:

- non-empty backendId
- positive candidateLimit

Candidate limits below one are clamped to one.

---

## Boundaries

This phase intentionally does not add:

- SearchTimer evaluation
- EPG matching
- duplicate detection
- candidate timer creation
- REST endpoint
- daemon scheduler
- RESTfulAPI write operation
- epgsearch mutation

---

## Verification

Targeted verification:

- make test-search-timer-automation-evaluation-plan
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 52.2 - SearchTimer automation match candidate model

The next phase should add a read-only match candidate domain object without performing matching or timer creation.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
