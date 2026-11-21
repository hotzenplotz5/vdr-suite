# SearchTimer Mandatory Backend Readback Verification Plan

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [SearchTimer yaVDR Local API Smoke Harness](searchtimer-yavdr-api-smoke-harness.md)
- [SearchTimer yaVDR Local API Smoke Harness Execution Report](searchtimer-yavdr-api-smoke-harness-execution-report.md)
- [SearchTimer Backend Readback Verification Result Model](searchtimer-backend-readback-verification-result-model.md)

---

## Purpose

Phase 50.40 defines the mandatory backend readback verification plan for future SearchTimer write execution.

The goal is to prevent a future production mutation path from reporting success only because a backend write command returned success.

A future production write path must prove that VDR-owned SearchTimer state changed as intended by reading the backend state after the write.

---

## Current Architecture Baseline

The current workflow already separates:

- command execution through `ISearchTimerCommandExecutor`
- backend state reading through `ISearchTimerDataSource`
- workflow planning and dispatch through `SearchTimerWorkflowCommandDispatchService`
- result reporting through `SearchTimerWorkflowExecutionResult`

The current dispatch skeleton already carries `requiresBackendReadback` through the result and warns that readback will be required after real execution.

Phase 50.40 does not open mutation.

Phase 50.40 records the required verification design before mutation can be considered.

---

## Mandatory Verification Rule

For every future SearchTimer production write operation:

1. Build and validate the workflow plan.
2. Map the workflow plan to a backend command request.
3. Pass all safety gates.
4. Invoke the backend command executor only when production mutation is explicitly enabled.
5. Perform mandatory backend readback when the plan requires readback.
6. Compare backend state with the expected SearchTimer state.
7. Report the final workflow result from both executor result and readback verification.

A successful executor result alone must not be enough for final success when `requiresBackendReadback=true`.

---

## Operation-Specific Readback Expectations

### Create

A successful create operation must be followed by a backend list/readback query.

The readback must prove that a SearchTimer exists with:

- the expected backend identity
- a stable backend-native id
- the expected query text
- the expected active/inactive state
- the expected relevant match options when supported

If the executor returns a backend-native id, the readback should prefer that id.

If the executor does not return a backend-native id, the readback must use deterministic matching criteria and report ambiguity explicitly.

### Update

A successful update operation must be followed by a backend list/readback query.

The readback must prove that the target SearchTimer reflects the updated values.

The verification must detect:

- missing target after update
- unchanged target after claimed update
- partially applied update
- wrong backend id
- wrong backend-native id
- changed state that does not match the request

### Delete

A delete operation usually has no positive object to compare after completion.

A future delete verification must prove absence when delete verification becomes part of production workflow.

For Phase 50.40, delete absence verification is planned separately because it may require different retry, timing and ambiguity rules than create/update readback.

---

## Verification Result Shape

The next implementation phase should add a backend readback verification result model.

The model should expose at least:

- `required`
- `attempted`
- `successful`
- `matched`
- `ambiguous`
- `expectedBackendId`
- `expectedBackendNativeId`
- `observedBackendNativeId`
- `message`
- `warnings`
- `errors`
- `auditTrail`

This result should later be embedded into `SearchTimerWorkflowExecutionResult` instead of being only a warning string.

---

## Safety Rules

A production SearchTimer write workflow must fail or remain blocked when:

- readback is required but no readback data source is available
- readback is attempted but backend state cannot be read
- readback finds no matching SearchTimer after create or update
- readback finds more than one plausible match and no backend-native id resolves ambiguity
- readback state differs from requested final state
- backend identity differs from the requested backend

The workflow may report partial backend execution separately from verified success, but client-facing success must require verification when readback is mandatory.

---

## Non-Goals

Phase 50.40 does not:

- enable production mutation
- call RESTfulAPI directly
- change the command executor interface
- change the smoke harness behavior
- implement the verification service
- implement delete absence verification

---

## Result Model

Phase 50.41 adds the first backend readback verification result model.

The model records whether readback was required, attempted, successful, matched and ambiguous.

Full model documentation:

- [SearchTimer Backend Readback Verification Result Model](searchtimer-backend-readback-verification-result-model.md)

---
## Follow-Up Phases

Recommended next phases:

- Phase 50.41 - SearchTimer workflow backend readback verification result model
- Phase 50.42 - SearchTimer workflow create readback verification service
- Phase 50.43 - SearchTimer workflow update readback verification service
- Phase 50.44 - SearchTimer workflow verified execution result integration
- Phase 50.45 - SearchTimer workflow delete absence verification plan

---

## Back

- [Back to SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [Back to Development Index](index.md)
- [Back to README](../../README.md)
