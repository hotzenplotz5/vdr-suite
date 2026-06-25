# SearchTimer Delete Absence Verification Service

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Delete Absence Verification Plan](searchtimer-delete-absence-verification-plan.md)
- [SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)

---

## Purpose

Phase 50.46 implements the SearchTimer delete absence verification service.

The service verifies that a successfully deleted backend-native SearchTimer id is no longer visible in backend readback state.

It does not enable production mutation.

---

## Source

Header:

    core/vdr/include/SearchTimerWorkflowDeleteAbsenceVerificationService.h

Implementation:

    core/vdr/src/SearchTimerWorkflowDeleteAbsenceVerificationService.cpp

Test:

    core/vdr/tests/test_search_timer_workflow_delete_absence_verification_service.cpp

Make target:

    make test-search-timer-workflow-delete-absence-verification-service

---

## Verification Behavior

The service consumes a `SearchTimerDeleteResult` and an `ISearchTimerDataSource`.

A delete absence verification passes only when no SearchTimer with the deleted backend id and backend-native id is found by readback.

The service fails when:

- the delete result is unsuccessful
- backend id is missing
- backend-native id is missing
- the readback data source is unavailable
- the deleted backend-native id is still visible
- multiple candidates with the deleted backend-native id are visible

---

## Audit Trail

The service records candidate counts and the final absence decision in the audit trail.

Important audit keys:

- `readbackReturnedCount`
- `sameNativeIdCount`
- `absenceVerified`

---

## Safety Boundary

The service does not call a command executor.

It only reads backend SearchTimer state and produces `SearchTimerWorkflowBackendReadbackVerificationResult`.

---

## Back

- [Back to SearchTimer Delete Absence Verification Plan](searchtimer-delete-absence-verification-plan.md)
- [Back to SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [Back to Development Index](index.md)
