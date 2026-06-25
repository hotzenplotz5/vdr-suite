# SearchTimer Automation Dry-Run Result Serializer

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

---

## Purpose

Phase 52.5 adds a frontend-visible JSON contract for SearchTimer automation dry-runs.

The serializer exposes the evaluation plan, match candidates, duplicate detections and candidate timer proposals without adding REST endpoints, scheduling or backend mutation.

---

## Added Types

- SearchTimerAutomationDryRunResult
- SearchTimerAutomationDryRunResultJsonSerializer

---

## JSON Contract

The dry-run JSON includes:

- success
- dryRunOnly
- mutationAllowed
- timerCreationAllowed
- backendWriteAllowed
- automaticExecutionAllowed
- requiresExplicitExecutionHandoff
- backendId
- candidateLimit
- inclusion flags
- evaluatedSearchTimerCount
- matchedCandidateCount
- duplicateRiskCount
- proposalCount
- allowedProposalCount
- blockedProposalCount
- validationErrors
- warnings
- errors
- auditTrail
- matchCandidates
- duplicateDetections
- candidateTimerProposals

---

## Safety Contract

Every dry-run result is currently read-only:

- dryRunOnly() is always true
- mutationAllowed() is always false
- timerCreationAllowed() is always false
- backendWriteAllowed() is always false
- automaticExecutionAllowed() is always false
- requiresExplicitExecutionHandoff() is always true

The serializer only renders state. It does not execute matching, create timers or call any backend.

---

## Boundaries

This phase intentionally does not add:

- automation service
- REST endpoint
- daemon scheduler
- timer creation
- RESTfulAPI write call
- epgsearch mutation
- production execution enablement

---

## Verification

Targeted verification:

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

Phase 52.6 - SearchTimer automation read-only service boundary

The next phase should introduce a read-only service boundary that accepts the existing model objects and produces a dry-run result without performing backend writes.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
