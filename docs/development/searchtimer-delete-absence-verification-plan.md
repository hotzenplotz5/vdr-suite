# SearchTimer Delete Absence Verification Plan

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Mandatory Backend Readback Verification Plan](searchtimer-backend-readback-verification-plan.md)
- [SearchTimer Backend Readback Verification Result Model](searchtimer-backend-readback-verification-result-model.md)
- [SearchTimer Create Readback Verification Service](searchtimer-create-readback-verification-service.md)
- [SearchTimer Update Readback Verification Service](searchtimer-update-readback-verification-service.md)
- [SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [SearchTimer Delete Absence Verification Service](searchtimer-delete-absence-verification-service.md)

---

## Purpose

Phase 50.45 defines the delete absence verification plan for future SearchTimer delete workflows.

Create and update verification prove that a desired object exists with expected state.

Delete verification must prove the opposite: the targeted backend-native SearchTimer must no longer be present after a successful delete command.

This phase does not enable production mutation.

---

## Baseline

The delete command result already carries backend identity and backend-native identity.

The backend readback verification result model can already report required, attempted, successful, matched, ambiguous, errors and audit trail.

The verified execution result integration can carry a nested readback verification result.

---

## Mandatory Delete Verification Rule

A future SearchTimer delete workflow must not report verified success only because the delete executor returned success.

When delete readback is required:

1. The delete executor must return a successful `SearchTimerDeleteResult`.
2. The delete result must contain the expected backend id.
3. The delete result must contain the expected backend-native id.
4. The verifier must read SearchTimer state back from `ISearchTimerDataSource`.
5. The verifier must search the same backend for the deleted backend-native id.
6. Verified delete success is allowed only when that backend-native id is absent.

---

## Expected Absence Semantics

A delete absence verification passes only when:

- readback is required
- readback was attempted
- the backend readback succeeded
- no SearchTimer with the deleted backend-native id is present
- the result is not ambiguous
- no errors are present

The verifier should still report candidate counts and audit details so the operator can inspect what happened.

---

## Failure Cases

Delete absence verification must fail when:

- delete execution failed
- backend id is missing
- backend-native id is missing
- no readback data source is available
- backend readback fails
- a SearchTimer with the deleted backend-native id is still visible
- multiple conflicting candidates make absence impossible to prove

---

## Timing and Retry Policy

Delete absence can be affected by backend timing, caching or eventual consistency.

The first implementation should remain deterministic and single-readback by default.

Retries should be added only as an explicit later policy and must be visible in the audit trail.

A future retry-aware variant should record:

- number of attempts
- delay policy
- final candidate count
- final decision reason

---

## Matching Policy

Delete verification must prefer backend-native id matching over text matching.

Name and query are not sufficient for delete absence verification because another SearchTimer may legitimately share the same text.

The target identity is:

- backend id
- backend-native id

Text fields may be logged for diagnostics, but must not replace native identity.

---

## Planned Service

The next phase should add:

    SearchTimerWorkflowDeleteAbsenceVerificationService

The service should consume:

- `SearchTimerDeleteResult`
- `ISearchTimerDataSource`

The service should produce:

- `SearchTimerWorkflowBackendReadbackVerificationResult`

---

## Implemented Service

Phase 50.46 implements the delete absence verification service planned here.

Full service documentation:

- [SearchTimer Delete Absence Verification Service](searchtimer-delete-absence-verification-service.md)

---
## Non-Goals

Phase 50.45 does not:

- implement the delete absence service
- enable production delete mutation
- add retry timing
- change the command executor
- call RESTfulAPI directly

---

## Back

- [Back to SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [Back to SearchTimer Update Readback Verification Service](searchtimer-update-readback-verification-service.md)
- [Back to Development Index](index.md)
