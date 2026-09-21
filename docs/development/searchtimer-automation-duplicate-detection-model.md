# SearchTimer Automation Duplicate Detection Model

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

---

## Purpose

Phase 52.3 adds the first read-only duplicate detection model for SearchTimer automation.

The model records duplicate risk for a match candidate without making automatic decisions and without creating timer proposals.

---

## Model

Classes and enums:

- SearchTimerAutomationDuplicateDetection
- SearchTimerAutomationDuplicateRiskLevel

Factory:

- forCandidate(SearchTimerAutomationMatchCandidate)

Core fields:

- backendId
- searchTimerId
- eventId
- eventTitle
- channelId
- eventStartTime
- eventDuration
- riskLevel
- existingTimerId
- existingRecordingPath
- titleSimilarity
- timeOverlapSeconds
- duplicateReasons

---

## Risk Levels

Supported risk levels:

- none
- low
- medium
- high
- blocking

Medium, high and blocking risk require operator review.

High and blocking risk block automatic proposal.

---

## Safety Contract

Every duplicate detection result is currently read-only:

- dryRunOnly() is always true
- mutationAllowed() is always false
- automaticDecisionAllowed() is always false
- timerProposalCreated() is always false

This prevents duplicate analysis from becoming an automatic execution decision.

---

## Validation

A valid duplicate detection result requires:

- non-empty backendId
- non-empty searchTimerId
- non-empty eventId

Title similarity is clamped to the range 0..100.

Negative overlap values are clamped to zero.

---

## Boundaries

This phase intentionally does not add:

- duplicate detection service
- automatic duplicate decisions
- candidate timer proposal
- REST endpoint
- daemon scheduler
- timer creation
- epgsearch mutation

---

## Verification

Targeted verification:

- make test-search-timer-automation-duplicate-detection
- make test-search-timer-automation-match-candidate
- make test-search-timer-automation-evaluation-plan
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 52.4 - SearchTimer automation candidate timer proposal model

The next phase should add a read-only candidate timer proposal model without creating timers.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
