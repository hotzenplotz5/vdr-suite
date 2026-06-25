# SearchTimer Automation Daemon Scheduling Plan

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Automation REST Preview Contract](searchtimer-automation-rest-preview-contract.md)

---

## Purpose

Phase 52.8 adds a scheduling plan model for future SearchTimer automation daemon work.

The model documents and verifies daemon scheduling boundaries without enabling any runtime scheduler, background thread, timer creation or backend mutation.

---

## Added Type

- SearchTimerAutomationDaemonSchedulingPlan

Factories:

- disabled(backendId)
- previewOnly(backendId, intervalMinutes)

---

## Safety Contract

The scheduling plan hard-codes these invariants:

- schedulerRuntimeAllowed() is false
- automaticExecutionAllowed() is false
- backendWriteAllowed() is false
- timerCreationAllowed() is false
- mutationAllowed() is false
- dryRunOnly() is true
- requiresExplicitExecutionHandoff() is true
- requireOperatorReviewBeforeExecution() is true

This keeps scheduling design separate from execution enablement.

---

## Planned Scheduling Inputs

The model captures future scheduling input without using it to execute anything:

- backendId
- enabled
- previewOnly
- intervalMinutes
- maximumCandidatesPerRun
- requireFreshSnapshot
- maximumSnapshotAgeSeconds
- requireOperatorReviewForDuplicates
- safetyReasons
- auditTrail

---

## Boundaries

This phase intentionally does not add:

- daemon scheduler
- background thread
- cron integration
- repeated automation loop
- RESTfulAPI write call
- epgsearch mutation
- automatic timer creation
- production execution enablement

---

## Verification

Targeted verification:

- make test-search-timer-automation-daemon-scheduling-plan
- make test-search-timer-automation-preview-controller
- make test-search-timer-automation-read-only-service
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 52.9 - SearchTimer automation safety review

The next phase should consolidate the automation safety boundaries before any further scheduling or execution work.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
