# SearchTimer Automation Candidate Timer Proposal Model

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

---

## Purpose

Phase 52.4 adds the first read-only candidate timer proposal model for SearchTimer automation.

The model translates a match candidate and duplicate detection result into a frontend-visible timer proposal without creating a timer and without enabling backend writes.

---

## Model

Class:

- SearchTimerAutomationCandidateTimerProposal

Factory:

- fromCandidateAndDuplicateDetection(candidate, duplicateDetection)

Core fields:

- backendId
- searchTimerId
- searchTimerName
- eventId
- eventTitle
- channelId
- eventStartTime
- eventDuration
- proposedStartTime
- proposedEndTime
- startMarginMinutes
- stopMarginMinutes
- recordingDirectory
- priority
- lifetime
- duplicateRiskName
- requiresOperatorReview
- existingTimerId
- existingRecordingPath
- proposalReasons
- blockReasons

---

## Safety Contract

Every proposal is currently read-only:

- dryRunOnly() is always true
- mutationAllowed() is always false
- timerCreationAllowed() is always false
- backendWriteAllowed() is always false
- automaticExecutionAllowed() is always false
- requiresExplicitExecutionHandoff() is always true

This phase creates a proposal object only. It does not create or modify VDR timers.

---

## Duplicate Risk Handling

The proposal imports duplicate risk information from SearchTimerAutomationDuplicateDetection.

High and blocking duplicate risk block automatic proposal allowance.

Existing timer and existing recording references are preserved for display and later dry-run JSON.

---

## Validation

A valid proposal requires:

- non-empty backendId
- non-empty searchTimerId
- non-empty eventId
- non-empty channelId
- proposedEndTime greater than proposedStartTime

Margins below zero are clamped to zero.

Priority and lifetime are clamped to 0..99.

---

## Boundaries

This phase intentionally does not add:

- proposal service
- dry-run result serializer
- REST endpoint
- daemon scheduler
- timer creation
- RESTfulAPI write call
- epgsearch mutation

---

## Verification

Targeted verification:

- make test-search-timer-automation-candidate-timer-proposal
- make test-search-timer-automation-duplicate-detection
- make test-search-timer-automation-match-candidate
- make test-search-timer-automation-evaluation-plan
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 52.5 - SearchTimer automation dry-run result serializer

The next phase should serialize the evaluation plan, candidates, duplicate risks and proposals into a read-only dry-run JSON result.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
