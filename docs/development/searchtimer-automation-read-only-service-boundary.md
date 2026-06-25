# SearchTimer Automation Read-Only Service Boundary

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Automation Foundation Planning](searchtimer-automation-foundation-planning.md)
- [SearchTimer Automation Evaluation Plan Model](searchtimer-automation-evaluation-plan-model.md)
- [SearchTimer Automation Match Candidate Model](searchtimer-automation-match-candidate-model.md)
- [SearchTimer Automation Duplicate Detection Model](searchtimer-automation-duplicate-detection-model.md)
- [SearchTimer Automation Candidate Timer Proposal Model](searchtimer-automation-candidate-timer-proposal-model.md)
- [SearchTimer Automation Dry-Run Result Serializer](searchtimer-automation-dry-run-result-serializer.md)

---

## Purpose

Phase 52.6 adds the first read-only service boundary for SearchTimer automation.

The service aggregates existing automation model objects into a SearchTimerAutomationDryRunResult without performing matching, scheduling, REST dispatch or backend mutation.

---

## Added Type

- SearchTimerAutomationReadOnlyService

Main method:

- evaluate(plan, matchCandidates, duplicateDetections, candidateTimerProposals, evaluatedSearchTimerCount)

---

## Behavior

The service:

- creates a dry-run result from the evaluation plan
- validates the plan
- attaches supplied match candidates
- attaches supplied duplicate detections
- attaches supplied candidate timer proposals
- records warnings for count mismatches
- records warnings for clamped evaluated counts
- appends audit entries

---

## Safety Contract

The service returns only read-only dry-run results:

- dryRunOnly() remains true
- mutationAllowed() remains false
- timerCreationAllowed() remains false
- backendWriteAllowed() remains false
- automaticExecutionAllowed() remains false
- requiresExplicitExecutionHandoff() remains true

This phase does not execute a matcher and does not create timers.

---

## Boundaries

This phase intentionally does not add:

- EPG matching service
- duplicate detection execution
- timer proposal generation logic
- REST endpoint
- daemon scheduler
- RESTfulAPI write call
- epgsearch mutation
- production execution enablement

---

## Verification

Targeted verification:

- make test-search-timer-automation-read-only-service
- make test-search-timer-automation-dry-run-result-json-serializer
- make test-search-timer-automation-candidate-timer-proposal
- make test-search-timer-automation-duplicate-detection
- make test-search-timer-automation-match-candidate
- make test-search-timer-automation-evaluation-plan
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 52.7 - SearchTimer automation REST preview contract

The next phase should expose a read-only REST preview contract for automation dry-runs without scheduling or backend writes.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
