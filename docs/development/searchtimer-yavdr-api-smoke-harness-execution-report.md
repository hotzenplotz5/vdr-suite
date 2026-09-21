# SearchTimer yaVDR Local API Smoke Harness Execution Report

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [SearchTimer yaVDR Local API Smoke Harness](searchtimer-yavdr-api-smoke-harness.md)
- [SearchTimer yaVDR Smoke-Test Execution Report](searchtimer-yavdr-smoke-test-execution-report.md)

---

## Purpose

This report records the successful local VDR-Suite API smoke harness execution introduced by Phase 50.38.

It proves that the Phase 50.36 smoke script can target a local VDR-Suite HTTP endpoint instead of accidentally targeting the VDR-Rectools/PHP endpoint on port 8080 or the direct RESTfulAPI endpoint on port 8002.

---

## Harness Build

Command:

    make searchtimer-yavdr-api-smoke-harness-helper

Observed result:

    SearchTimer yaVDR local VDR-Suite API smoke harness
    It serves:
      POST /api/searchtimers/real-test
      POST /api/vdr/searchtimers/real-test
    Defaults:
      --host 127.0.0.1
      --port 18080

Conclusion:

- The local harness compiled successfully.
- The help output confirmed the expected VDR-Suite real-test routes.
- The help output confirmed that backend mutation remains blocked by the production policy gate.

---

## End-to-End Local Harness Run

Command:

    make searchtimer-yavdr-api-smoke-harness-run

Observed smoke input:

    baseUrl=http://127.0.0.1:18080
    endpoint=/api/searchtimers/real-test
    operation=create
    backendId=home-vdr
    httpStatus=200

Observed checks:

    PASS HTTP 200 - status=200
    PASS JSON parse
    PASS success=false
    PASS executed=false
    PASS blocked=true
    PASS dryRunOnly=true
    PASS executionMode=execute
    PASS executorOptInProvided=true
    PASS executorInjected=true
    PASS executorInvocationAttempted=false
    PASS production policy gate closed - dispatchStage=production-policy-gate-closed
    PASS operator warning: no mutation
    PASS audit trail: final policy stage
    RESULT: OK

Conclusion:

- The local VDR-Suite HTTP harness accepted the smoke request.
- The smoke script received HTTP 200.
- The result was parsed as JSON.
- The workflow reached the expected production policy gate.
- The result stayed blocked.
- No executor invocation was attempted.
- No backend mutation was performed.

---

## Verified Response Traits

The JSON response included:

    dispatchStage=production-policy-gate-closed
    message=production policy gate is closed for real backend mutation
    executorInvocationAttempted=false
    realExecutionEnabled=false
    realExecutionPolicyAllowed=false
    dryRunOnly=true
    blocked=true
    executed=false
    success=false

The audit trail included:

    executionMode=execute
    commandRequestMapped=true
    explicitOperatorConfirmation=true
    executorOptInProvided=true
    executorInjected=true
    policyStage=production-policy-gate-closed
    policyAllowed=false
    guardStage=production-policy-gate-closed
    guardPassed=false
    killSwitchStage=production-policy-gate-closed
    killSwitchOpen=false
    killSwitchPassed=false
    executorInvocationAttempted=false
    executorResultMapped=false

---

## Safety Boundary

This phase does not enable real SearchTimer mutation through VDR-Suite.

It proves the local VDR-Suite API safety endpoint can be reached through HTTP and still refuses production mutation.

Direct RESTfulAPI mutation remains covered by the separate direct real smoke helper.

---

## Follow-Up

The next safe planning step is mandatory backend readback verification before any future production mutation path is opened.

That follow-up should define how VDR-Suite verifies create and update results against backend state before a production execution path can be considered safe.

---

## Back

- [Back to SearchTimer yaVDR Local API Smoke Harness](searchtimer-yavdr-api-smoke-harness.md)
- [Back to SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [Back to Development Index](index.md)
- [Back to README](../../README.md)
