# SearchTimer yaVDR Local API Smoke Harness

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [SearchTimer yaVDR Smoke-Test Execution Report](searchtimer-yavdr-smoke-test-execution-report.md)

---

## Purpose

Phase 50.38 adds a local HTTP harness for the VDR-Suite SearchTimer real-test safety endpoint.

The harness exists because the yaVDR environment already uses port 8080 for the VDR-Rectools/PHP dashboard, while the direct VDR RESTfulAPI endpoint runs on port 8002.

The harness gives the Phase 50.36 smoke script a local VDR-Suite endpoint that is not Rectools and not direct RESTfulAPI.

---

## Tool

Source:

    apps/tools/searchtimer_yavdr_api_smoke_harness.cpp

Build helper:

    make searchtimer-yavdr-api-smoke-harness-helper

End-to-end local harness run:

    make searchtimer-yavdr-api-smoke-harness-run

---

## Served Routes

The harness serves:

    POST /api/searchtimers/real-test
    POST /api/vdr/searchtimers/real-test

All other paths return JSON not-found responses.

---

## Safety Behavior

The harness calls SearchTimerController::realTestSearchTimerWorkflow().

Expected smoke-test outcome:

    RESULT: OK
    dispatchStage=production-policy-gate-closed
    executorInvocationAttempted=false

The harness does not enable production backend mutation.

The production policy gate remains closed.

---

## Relationship to Direct RESTfulAPI Smoke Test

The direct RESTfulAPI smoke test on port 8002 proves that the VDR RESTfulAPI SearchTimer lifecycle can create, read back, update and delete a temporary SearchTimer.

The local VDR-Suite API harness proves that the VDR-Suite safety endpoint can be reached through HTTP and still blocks mutation as expected.

These are intentionally separate tests.

---

## Back

- [Back to SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [Back to Development Index](index.md)
- [Back to README](../../README.md)
