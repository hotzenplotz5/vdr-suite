# SearchTimer Automation Match Candidate Model

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Automation Foundation Planning](searchtimer-automation-foundation-planning.md)
- [SearchTimer Automation Evaluation Plan Model](searchtimer-automation-evaluation-plan-model.md)

---

## Purpose

Phase 52.2 adds the first read-only match candidate model for SearchTimer automation.

The model represents a possible SearchTimer-to-EPG-event match without performing matching, duplicate detection, timer proposal creation or backend mutation.

---

## Model

Class:

- SearchTimerAutomationMatchCandidate

Factory:

- createReadOnly(backendId, searchTimerId, eventId)

Core fields:

- backendId
- searchTimerId
- searchTimerName
- eventId
- eventTitle
- channelId
- eventStartTime
- eventDuration
- matchScore
- matchReasons

---

## Safety Contract

Every candidate is currently read-only:

- dryRunOnly() is always true
- mutationAllowed() is always false
- timerProposalCreated() is always false
- requiresDuplicateCheck() is always true

This prevents a matched candidate from becoming an executable timer action.

---

## Validation

A valid candidate requires:

- non-empty backendId
- non-empty searchTimerId
- non-empty eventId

Match scores are clamped to the range 0..100.

Negative event start times and durations are clamped to zero.

---

## Boundaries

This phase intentionally does not add:

- SearchTimer evaluation
- EPG matching service
- duplicate detection
- candidate timer proposal
- REST endpoint
- daemon scheduler
- timer creation
- epgsearch mutation

---

## Verification

Targeted verification:

- make test-search-timer-automation-match-candidate
- make test-search-timer-automation-evaluation-plan
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 52.3 - SearchTimer automation duplicate detection model

The next phase should add a read-only duplicate risk model without making automatic decisions.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
