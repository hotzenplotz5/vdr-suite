# SearchTimer Automation Foundation Planning

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [Live Parity Discovery Foundation Completion](live-parity-discovery-foundation-completion.md)
- [SearchTimer Workflow Foundation Completion](searchtimer-workflow-foundation-completion.md)
- [SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [SearchTimer Verified Execution REST Contract](searchtimer-verified-execution-rest-contract.md)
- [SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [SearchTimer End-to-End Verified Execution Test](searchtimer-end-to-end-verified-execution-test.md)

---

## Purpose

Phase 52.0 starts the SearchTimer automation foundation as a planning-only phase.

The goal is to define the automation boundary before any scheduled evaluation, candidate timer creation or execution is implemented.

---

## Existing Foundation

The implementation already has safe building blocks that Phase 52 can build on:

- SearchTimerWorkflowPlanningService
- SearchTimerWorkflowExecutionService
- SearchTimerWorkflowProductionPolicyGate
- SearchTimerWorkflowExecutionResult
- backend readback verification
- guarded executor invocation
- production policy gates
- backend write allowlists
- real execution enablement switches
- Live parity discovery helper surfaces from Phase 51

---

## Automation Boundary

Automation must stay separated into explicit steps:

1. evaluation planning
2. candidate matching
3. duplicate analysis
4. candidate timer proposal
5. validation and dry-run result
6. explicit execution handoff

The foundation must not jump directly from a matched SearchTimer to a real timer mutation.

---

## Phase 52 Initial Roadmap

Recommended sequence:

- Phase 52.1 - SearchTimer automation evaluation plan model
- Phase 52.2 - SearchTimer automation match candidate model
- Phase 52.3 - SearchTimer automation duplicate detection model
- Phase 52.4 - SearchTimer automation candidate timer proposal model
- Phase 52.5 - SearchTimer automation dry-run result serializer
- Phase 52.6 - SearchTimer automation read-only service boundary
- Phase 52.7 - SearchTimer automation REST preview contract
- Phase 52.8 - SearchTimer automation daemon scheduling plan
- Phase 52.9 - SearchTimer automation safety review

---

## Hard Safety Rules

Phase 52 starts with these hard rules:

- no real timer creation
- no real timer update
- no real timer delete
- no epgsearch mutation
- no automatic execution without explicit gated phase
- no daemon scheduler that performs writes
- no hidden production enablement

---

## Data Inputs

Read-only automation may later use:

- SearchTimer rules
- EPG events
- Live parity discovery helper lists
- existing timers
- existing recordings
- backend capabilities
- backend readback verification

---

## Next Phase

Phase 52.1 - SearchTimer automation evaluation plan model

The next phase should add a small read-only domain model describing what an automation evaluation would do, without running it.

---

## Verification

Targeted verification:

- make test-docs
- make test-phase
- make daemon

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
