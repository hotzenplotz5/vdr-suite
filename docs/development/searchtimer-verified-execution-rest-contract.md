# SearchTimer Verified Execution REST Contract

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Readback Services Dispatch Integration](searchtimer-readback-services-dispatch-integration.md)
- [SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [SearchTimer End-to-End Verified Execution Test](searchtimer-end-to-end-verified-execution-test.md)

---

## Purpose

Phase 50.48 documents and tests the REST-facing verified execution contract for SearchTimer workflow execution responses.

The contract makes executor state and backend readback verification state visible to clients.

---

## Required Response Fields

SearchTimer workflow execution responses must expose:

- `executorResultSuccessful`
- `backendReadbackVerificationAttached`
- `backendReadbackVerified`
- `backendReadbackVerification`
- final `success`
- final `dispatchStage`

---

## Contract Semantics

`executorResultSuccessful` reports whether the backend command executor result itself succeeded.

`backendReadbackVerificationAttached` reports whether a concrete readback verification result was attached.

`backendReadbackVerified` reports whether the attached readback verification passed.

Final `success` remains the client-facing workflow success and may be false when executor success is followed by failed required readback verification.

---

## Delete Contract

Delete workflows now expose `followUpStep` as `readback` and `requiresBackendReadback` as true.

The delete readback contract verifies absence of the deleted backend-native id.

---

## End-to-End Test Coverage

Phase 50.49 adds a dedicated end-to-end verified execution test for the REST-visible execution semantics.

Full test documentation:

- [SearchTimer End-to-End Verified Execution Test](searchtimer-end-to-end-verified-execution-test.md)

---

## Safety Boundary

This phase does not enable production mutation.

It strengthens the REST contract and test coverage for verified execution response visibility.

---

## Back

- [Back to SearchTimer Readback Services Dispatch Integration](searchtimer-readback-services-dispatch-integration.md)
- [Back to SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [Back to Development Index](index.md)
