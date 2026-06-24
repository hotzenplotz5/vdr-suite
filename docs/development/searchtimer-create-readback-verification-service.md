# SearchTimer Create Readback Verification Service

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Mandatory Backend Readback Verification Plan](searchtimer-backend-readback-verification-plan.md)
- [SearchTimer Backend Readback Verification Result Model](searchtimer-backend-readback-verification-result-model.md)
- [SearchTimer Update Readback Verification Service](searchtimer-update-readback-verification-service.md)

---

## Purpose

Phase 50.42 adds the first concrete readback verification service for SearchTimer create workflows.

The service checks whether a successful create result can be confirmed by backend readback through `ISearchTimerDataSource`.

It does not enable production mutation.

---

## Source

Header:

    core/vdr/include/SearchTimerWorkflowCreateReadbackVerificationService.h

Implementation:

    core/vdr/src/SearchTimerWorkflowCreateReadbackVerificationService.cpp

Test:

    core/vdr/tests/test_search_timer_workflow_create_readback_verification_service.cpp

Make target:

    make test-search-timer-workflow-create-readback-verification-service

---

## Verification Behavior

The service verifies a successful create by reading SearchTimer state back from the backend data source.

A create readback is verified only when exactly one matching SearchTimer is found.

The match checks:

- backend id
- backend-native id when the expected result has one
- name
- query
- state

The service reports failure when:

- create failed before readback
- no readback data source is available
- no matching SearchTimer is found
- more than one matching SearchTimer is found
- backend-native id differs from the expected id

---

## Safety Boundary

This service consumes an already existing create result and a readback data source.

It does not call a backend command executor.

It does not open the production policy gate.

It is preparation for future verified write execution.

---

## Update Readback Service

Phase 50.43 adds the sibling update-readback verification service.

Full service documentation:

- [SearchTimer Update Readback Verification Service](searchtimer-update-readback-verification-service.md)

---
## Back

- [Back to SearchTimer Backend Readback Verification Result Model](searchtimer-backend-readback-verification-result-model.md)
- [Back to SearchTimer Mandatory Backend Readback Verification Plan](searchtimer-backend-readback-verification-plan.md)
- [Back to Development Index](index.md)
