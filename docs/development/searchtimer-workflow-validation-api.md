# SearchTimer Workflow Validation API

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer User Workflow Foundation](searchtimer-user-workflow-foundation.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)
- [SearchTimer Real Payload Validation](searchtimer-real-payload-validation.md)
- [SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [SearchTimer yaVDR Smoke-Test Execution Report](searchtimer-yavdr-smoke-test-execution-report.md)
- [SearchTimer yaVDR Local API Smoke Harness](searchtimer-yavdr-api-smoke-harness.md)
- [SearchTimer yaVDR Local API Smoke Harness Execution Report](searchtimer-yavdr-api-smoke-harness-execution-report.md)
- [SearchTimer Mandatory Backend Readback Verification Plan](searchtimer-backend-readback-verification-plan.md)
- [SearchTimer Backend Readback Verification Result Model](searchtimer-backend-readback-verification-result-model.md)
- [SearchTimer Create Readback Verification Service](searchtimer-create-readback-verification-service.md)
- [SearchTimer Update Readback Verification Service](searchtimer-update-readback-verification-service.md)

---

## Purpose

This document defines the SearchTimer workflow validation API introduced by Phase 50.4 and documented in Phase 50.5.

The endpoint validates user workflow intent before a SearchTimer create, update or delete workflow is executed.

It does not write to VDR, epgsearch, RESTfulAPI or any backend.

---

## Endpoints

Primary route:

    POST /api/searchtimers/validate

Compatibility alias:

    POST /api/vdr/searchtimers/validate

Both routes return the same response shape.

---

## Safety Boundary

The validation endpoint is deliberately validate-only.

It must not:

- create a SearchTimer
- update a SearchTimer
- delete a SearchTimer
- call a SearchTimer command executor
- mutate backend state
- hide missing backend-native identity for update or delete workflows

It may:

- parse the submitted workflow intent
- classify the operation as read-only or write
- validate required fields
- return warnings for write intent
- return readback-after-write recommendations for create and update workflows

This allows clients to show operator-facing validation results before the real write endpoint is used.

---

## Request Parser Boundary

Phase 50.6 extracts workflow validation body parsing into SearchTimerWorkflowValidationRequestParser.

The controller should use this parser boundary instead of owning request-body parsing helpers directly.

This keeps the REST controller focused on orchestration and response construction while the parser remains separately testable.

---

## Request Fields

The request is a flat JSON object.

Supported fields:

| Field | Required for | Meaning |
| --- | --- | --- |
| operation | all operations | Workflow operation name. |
| action | optional alias | Alternative name for operation. |
| backendId | preview, create, readback, update, delete | Backend identity. |
| backendNativeId | readback, update, delete | Backend-native SearchTimer identity. |
| name | preview, create, update | Operator-facing SearchTimer name. |
| query | preview, create, update | SearchTimer query text. |
| active | create, update | Desired active state. Defaults to true if omitted. |

Supported operation values:

| Operation | Read-only | Write intent | Requires backendNativeId | Wants readback after write |
| --- | --- | --- | --- | --- |
| list | yes | no | no | no |
| preview | yes | no | no | no |
| create | no | yes | no | yes |
| readback | yes | no | yes | no |
| update | no | yes | yes | yes |
| delete | no | yes | yes | no |
| remove | no | yes | yes | no |

The remove operation is accepted as an alias for delete intent.

Unknown or missing operation values produce an invalid result.

---

## List Validation Example

Request:

    {
      "operation": "list"
    }

Expected response traits:

    {
      "valid": true,
      "operation": "list",
      "readOnly": true,
      "writeOperation": false,
      "wantsReadbackAfterWrite": false,
      "errors": []
    }

A backendId may be supplied to validate a backend-scoped list intent, but it is not required for list.

---

## Preview Validation Example

Request:

    {
      "operation": "preview",
      "backendId": "home-vdr",
      "name": "Terra X Suche",
      "query": "Terra X"
    }

Expected response traits:

    {
      "valid": true,
      "operation": "preview",
      "readOnly": true,
      "writeOperation": false,
      "backendId": "home-vdr",
      "errors": []
    }

Preview is read-only. It validates candidate query intent before a real SearchTimer is created.

---

## Create Validation Example

Request:

    {
      "operation": "create",
      "backendId": "home-vdr",
      "name": "Terra X Suche",
      "query": "Terra X",
      "active": true
    }

Expected response traits:

    {
      "valid": true,
      "operation": "create",
      "readOnly": false,
      "writeOperation": true,
      "wantsReadbackAfterWrite": true,
      "backendId": "home-vdr",
      "warnings": [
        "write operation requires explicit operator intent",
        "backend readback is recommended after this operation"
      ],
      "errors": []
    }

Create validation confirms that the workflow intent is structurally complete.

It does not create the SearchTimer.

---

## Readback Validation Example

Request:

    {
      "operation": "readback",
      "backendId": "home-vdr",
      "backendNativeId": "searchtimer-42"
    }

Expected response traits:

    {
      "valid": true,
      "operation": "readback",
      "readOnly": true,
      "writeOperation": false,
      "backendId": "home-vdr",
      "backendNativeId": "searchtimer-42",
      "errors": []
    }

Readback is read-only and requires backend-native identity.

---

## Update Validation Example

Request:

    {
      "operation": "update",
      "backendId": "home-vdr",
      "backendNativeId": "searchtimer-42",
      "name": "Terra X Suche aktualisiert",
      "query": "Terra X aktuell",
      "active": false
    }

Expected response traits:

    {
      "valid": true,
      "operation": "update",
      "readOnly": false,
      "writeOperation": true,
      "wantsReadbackAfterWrite": true,
      "backendId": "home-vdr",
      "backendNativeId": "searchtimer-42",
      "warnings": [
        "write operation requires explicit operator intent",
        "backend readback is recommended after this operation"
      ],
      "errors": []
    }

Update validation requires backend-native identity so clients cannot accidentally update an ambiguous backend object.

---

## Delete Validation Example

Request:

    {
      "operation": "delete",
      "backendId": "home-vdr",
      "backendNativeId": "searchtimer-42"
    }

Expected response traits:

    {
      "valid": true,
      "operation": "delete",
      "readOnly": false,
      "writeOperation": true,
      "backendId": "home-vdr",
      "backendNativeId": "searchtimer-42",
      "warnings": [
        "write operation requires explicit operator intent"
      ],
      "errors": []
    }

Delete validation requires backend-native identity.

It does not delete the SearchTimer.

---

## Invalid Update Example

Request:

    {
      "operation": "update",
      "backendId": "home-vdr",
      "name": "Terra X Suche aktualisiert",
      "query": "Terra X aktuell"
    }

Expected response traits:

    {
      "valid": false,
      "operation": "update",
      "errors": [
        "backendNativeId is required"
      ]
    }

Clients should block the real update action until validation succeeds and the operator explicitly confirms the write.

---

## Response Fields

| Field | Type | Meaning |
| --- | --- | --- |
| valid | boolean | True when there are no validation errors. |
| operation | string | Normalized operation name. |
| readOnly | boolean | True for list, preview and readback. |
| writeOperation | boolean | True for create, update and delete. |
| wantsReadbackAfterWrite | boolean | True for create and update. |
| backendId | string | Backend identity supplied by the request. |
| backendNativeId | string | Backend-native SearchTimer identity supplied by the request. |
| warnings | string array | Operator-facing warnings. |
| errors | string array | Validation errors that block workflow execution. |

The endpoint currently returns HTTP 200 for both valid and invalid validation results.

The validation state is represented by the valid field and the errors array.

---

## Execution Plan Boundary

Phase 50.7 adds SearchTimerWorkflowExecutionPlan as a backend-neutral planning model after request parsing and validation.

The plan describes intended workflow steps such as list, preview, create, readback, update and delete without executing backend writes.

Create and update plans include a readback follow-up step.

Write plans require explicit operator confirmation.

Invalid requests produce no executable primary step.

---

## Planning Service Boundary

Phase 50.8 adds SearchTimerWorkflowPlanningService as the first service boundary above validation and plan modeling.

The planning service accepts a SearchTimerWorkflowRequest and returns a SearchTimerWorkflowExecutionPlan.

It still does not execute backend operations.

Invalid requests produce a non-executable plan.

Valid create and update requests produce plans with a readback follow-up step.

---

## Planning JSON Contract

Phase 50.9 adds SearchTimerWorkflowExecutionPlanJsonSerializer so clients can inspect planned workflow steps before execution.

The planning JSON includes:
- valid
- operation
- primaryStep
- followUpStep
- hasExecutionWork
- hasFollowUpStep
- readOnly
- writeOperation
- requiresExplicitOperatorConfirmation
- requiresBackendReadback
- backendId
- backendNativeId
- name
- query
- active

The contract remains planning-only and does not execute backend operations.

---

## Planning REST Endpoint

Phase 50.10 exposes the planning JSON through REST:
- POST /api/searchtimers/plan
- POST /api/vdr/searchtimers/plan

The endpoint parses the workflow request body, builds a SearchTimerWorkflowExecutionPlan and returns the planning JSON.

It does not call a SearchTimer command executor.

It does not create, update or delete SearchTimers.

Invalid requests return HTTP 200 with valid=false and hasExecutionWork=false in the planning JSON.

---

## Execution Skeleton Boundary

Phase 50.11 adds a SearchTimerWorkflowExecutionService skeleton.

The skeleton consumes SearchTimerWorkflowExecutionPlan objects and returns SearchTimerWorkflowExecutionResult objects.

The skeleton does not call a SearchTimer command executor.

The skeleton does not mutate a backend.

Invalid or non-executable plans are blocked.

Write plans require explicit operator confirmation before they are accepted by the skeleton.

Accepted skeleton results are dry-run-only and have executed=false.

---

## Execution Result JSON Contract

Phase 50.12 adds SearchTimerWorkflowExecutionResultJsonSerializer.

The execution result JSON includes:
- success
- executed
- blocked
- dryRunOnly
- confirmationProvided
- requiresExplicitOperatorConfirmation
- requiresBackendReadback
- operation
- primaryStep
- followUpStep
- backendId
- backendNativeId
- message
- warnings
- errors

The JSON contract represents the guarded execution skeleton only.

It does not imply that a backend command was executed.

---

## Execution REST Contract

Phase 50.13 exposes the guarded workflow execution skeleton through REST.

Phase 50.14 documents the endpoint contract, confirmation fields, result semantics and safety boundary.

Endpoints:

- POST /api/searchtimers/execute
- POST /api/vdr/searchtimers/execute

The endpoint parses the same workflow request shape as validation and planning.

### Request Fields

Supported request fields:

- operation or action
- backendId
- backendNativeId
- name
- query
- active
- explicitOperatorConfirmation
- operatorConfirmed
- confirmed

Supported operations:

- list
- preview
- create
- readback
- update
- delete
- remove as alias for delete

Write operations are:

- create
- update
- delete

Read-only operations are:

- list
- preview
- readback

### Confirmation Semantics

Write operations are only accepted by the skeleton when one of these fields is truthy:

- explicitOperatorConfirmation
- operatorConfirmed
- confirmed

Accepted truthy values are:

- true
- 1
- "true"
- "1"

Missing, false or unknown confirmation values block write-operation plans.

Read-only operations do not require explicit operator confirmation.

### Response Semantics

The endpoint always returns HTTP 200 with application/json when the controller is available.

Invalid workflow requests are represented in the JSON result instead of using HTTP error codes.

The endpoint returns the Phase 50.12 execution-result JSON contract.

Important response fields:

- success
- executed
- blocked
- dryRunOnly
- confirmationProvided
- requiresExplicitOperatorConfirmation
- requiresBackendReadback
- operation
- primaryStep
- followUpStep
- backendId
- backendNativeId
- message
- warnings
- errors

### Safety Boundary

The endpoint does not call ISearchTimerCommandExecutor.

The endpoint does not mutate a backend.

The endpoint does not create, update or delete a SearchTimer.

Accepted skeleton results still have:

- executed=false
- dryRunOnly=true

The endpoint is therefore safe for clients to use as a preview of execution behavior before real backend mutation is implemented.

### Command Request Boundary

Phase 50.15 adds a workflow command request mapper.

The mapper converts executable write-operation plans into existing command request objects:

- create plans map to SearchTimerCreateRequest
- update plans map to SearchTimerUpdateRequest
- delete plans map to SearchTimerDeleteRequest

The mapper preserves:

- backendId
- backendNativeId for update and delete
- name for create and update
- query for create and update
- active for create and update

The mapper does not call ISearchTimerCommandExecutor.

The mapper does not mutate a backend.

The mapper is therefore a command boundary preparation step only.

### Command Dispatch Skeleton

Phase 50.16 adds a guarded workflow command dispatch skeleton.

The dispatch skeleton consumes executable workflow plans and uses the command request mapper from Phase 50.15.

For write-operation plans it verifies that a command request can be mapped for:

- create
- update
- delete

The dispatch skeleton still does not call ISearchTimerCommandExecutor.

The dispatch skeleton still does not mutate a backend.

Accepted dispatch skeleton results still have:

- executed=false
- dryRunOnly=true

The dispatch skeleton is therefore a preparation boundary for a later opt-in real command dispatcher.

### Dispatch REST Wiring

Phase 50.17 wires the guarded command dispatch skeleton into the REST execution path.

POST /api/searchtimers/execute and POST /api/vdr/searchtimers/execute now use:

1. SearchTimerWorkflowValidationRequestParser
2. SearchTimerWorkflowPlanningService
3. SearchTimerWorkflowCommandDispatchService
4. SearchTimerWorkflowExecutionResultJsonSerializer

The endpoint still returns the same execution-result JSON contract.

The endpoint still does not call ISearchTimerCommandExecutor.

The endpoint still does not mutate a backend.

Accepted write-operation results continue to report:

- executed=false
- dryRunOnly=true

This makes the REST execution path follow the future real-dispatch architecture while keeping the current runtime safe.

### Dispatch Result Semantics

Phase 50.18 adds explicit result semantics for the guarded dispatch path.

Additional response fields:

- commandRequestMapped
- realExecutionEnabled
- realExecutionPolicyAllowed
- executorOptInProvided
- executorInjected
- executorInvocationGuardPassed
- executorInvocationAttempted
- executorInvocationKillSwitchOpen
- executorInvocationKillSwitchPassed
- executorResultMapped
- executorResultSuccessful
- executorInvocationAuditTrail
- dispatchStage

The dispatchStage field helps clients distinguish:

- validation-blocked
- confirmation-required
- read-only-no-dispatch
- command-request-mapped
- command-request-mapping-failed
- skeleton-accepted
- blocked

For the current guarded dispatch path:

- commandRequestMapped=true means a create, update or delete command request was built from the workflow plan.
- realExecutionEnabled=false means the backend command executor was not called.
- executed=false confirms that no backend mutation was performed.
- dryRunOnly=true confirms that the response belongs to the safe skeleton path.

### Execution Mode Contract

Phase 50.19 adds an explicit execution mode contract.

Accepted request fields:

- executionMode
- mode

Supported values:

- dryRun
- dry-run
- dryrun
- prepare
- execute
- real

Default behavior is prepare.

Mode semantics:

- dryRun accepts the workflow as a dry-run only and does not map a command request.
- prepare maps command requests where possible but still does not call a backend executor.
- execute is reserved for future real backend execution and is currently blocked by the guarded dispatch path.

Current safety guarantees:

- realExecutionEnabled=false
- executed=false
- dryRunOnly=true
- ISearchTimerCommandExecutor is not called
- backend mutation is not performed

### Executor Opt-In Boundary

Phase 50.20 adds an explicit executor opt-in boundary.

The internal command dispatch service now accepts dispatch options.

For executionMode=execute:

- without executor opt-in, the dispatch stage is executor-opt-in-required
- with executor opt-in, the dispatch stage is real-execution-disabled until a real executor is wired

The executor opt-in boundary is intentionally separate from operator confirmation.

This means real backend mutation requires all of the following in a later phase:

- a valid write workflow plan
- explicit operator confirmation
- executionMode=execute
- executor opt-in enabled
- a deliberately wired backend command executor

Current safety guarantees remain unchanged:

- realExecutionEnabled=false
- executed=false
- dryRunOnly=true
- backend mutation is not performed

### Executor Opt-In REST Contract

Phase 50.21 exposes the executor opt-in boundary through the REST execution contract.

Accepted request fields:

- executorOptIn
- executorOptInEnabled
- executorOptInProvided
- enableExecutor
- allowExecutor

These fields are accepted by:

- POST /api/searchtimers/execute
- POST /api/vdr/searchtimers/execute

When executionMode=execute and executor opt-in is not present, the result remains blocked with:

- executorOptInProvided=false
- dispatchStage=executor-opt-in-required

When executionMode=execute and executor opt-in is present, the result still remains blocked until a real executor is deliberately wired:

- executorOptInProvided=true
- realExecutionEnabled=false
- executed=false
- dryRunOnly=true
- dispatchStage=real-execution-disabled

This REST contract does not enable backend mutation.

### Real Executor Policy Boundary

Phase 50.22 adds a central real-execution policy boundary.

The policy is evaluated after:

- workflow validation
- execution planning
- explicit operator confirmation
- command request mapping
- executionMode=execute
- executor opt-in

Current policy behavior:

- non-execute modes do not require the real-execution policy
- execute mode without opt-in still blocks at executor-opt-in-required
- execute mode with opt-in blocks at real-execution-policy-denied

The result includes:

- realExecutionPolicyAllowed=false
- realExecutionEnabled=false
- executed=false
- dryRunOnly=true

This keeps all backend mutation disabled until a later phase deliberately changes both policy and executor wiring.

### Real Executor Injection Skeleton

Phase 50.23 adds an injectable command executor skeleton to the workflow dispatch options.

The skeleton accepts an ISearchTimerCommandExecutor pointer internally but does not call it.

Execution-mode behavior:

- execute without executor opt-in blocks at executor-opt-in-required
- execute with executor opt-in but without an injected executor blocks at real-executor-injection-required
- execute with executor opt-in and an injected executor still blocks at real-execution-policy-denied

The result includes:

- executorInjected=true when an executor pointer is present
- executorInjected=false when REST calls do not inject an executor
- realExecutionPolicyAllowed=false
- realExecutionEnabled=false
- executed=false
- dryRunOnly=true

This phase prepares dependency injection only. It does not perform backend mutation.

### Guarded Executor Invocation Contract

Phase 50.24 adds a guarded executor invocation contract.

The guard evaluates whether a future executor invocation would be allowed.

Required conditions include:

- executionMode=execute
- a write workflow plan
- a mapped command request
- executor opt-in
- an injected command executor
- real-execution policy approval

Current behavior remains intentionally non-mutating:

- executorInvocationGuardPassed=false in REST-visible paths
- executorInvocationAttempted=false in all paths
- realExecutionEnabled=false
- executed=false
- dryRunOnly=true

A direct unit test also verifies that even when a synthetic policy decision would satisfy the guard, invocationAttempted remains false and the fake executor is not called.

### Executor Result Mapping Skeleton

Phase 50.25 adds a result mapper for future executor invocation results.

The mapper can translate synthetic executor results into SearchTimerWorkflowExecutionResult values:

- create result
- update result
- delete result

The mapper can produce executed=true for a synthetic successful executor result in direct unit tests.

The standard REST and dispatch paths still do not invoke the executor:

- executorInvocationAttempted=false
- executorResultMapped=false
- executorResultSuccessful=false
- realExecutionEnabled=false
- executed=false
- dryRunOnly=true

This phase proves that the future result path is representable without enabling backend mutation.

### Executor Invocation Kill-Switch Contract

Phase 50.26 adds the final kill-switch contract before any future executor invocation.

The kill-switch is evaluated after the guarded executor invocation contract.

Current standard behavior:

- executorInvocationKillSwitchOpen=false
- executorInvocationKillSwitchPassed=false
- executorInvocationAttempted=false
- executorResultMapped=false
- executed=false

A direct unit test proves the intended future path:

- guard passed
- kill-switch open
- kill-switch decision allowed

The direct mapper test keeps synthetic successful executor results mapped to executed=true, but only as a representation of a future controlled path.

### Controlled Test Executor Invocation Path

Phase 50.27 adds a controlled test-only executor invocation path.

This path is not exposed through REST.

It requires:

- executionMode=execute
- explicit operator confirmation
- executor opt-in
- an injected command executor
- controlledTestExecutorInvocationEnabled=true
- policy approval for the controlled test path
- guard pass
- open kill-switch

The controlled test path invokes a fake/test executor in unit tests only and maps the result through SearchTimerWorkflowExecutorResultMapper.

The standard REST path remains non-mutating:

- executorInvocationAttempted=false
- executorInvocationKillSwitchOpen=false
- executorResultMapped=false
- executed=false

### Controlled Invocation Audit Trail

Phase 50.28 adds executorInvocationAuditTrail to the workflow execution result.

The audit trail records the decision chain for controlled executor invocation:

- execution mode
- command request mapping state
- explicit confirmation
- executor opt-in
- executor injection
- controlled test invocation flag
- policy stage and policy decision
- guard stage and guard decision
- kill-switch stage and kill-switch decision
- executor invocation attempt state
- executor result mapping state

The controlled test path must show a complete success audit trail.

The normal denied path must show why execution did not proceed.

REST remains non-mutating.

### Real Backend Execution Readiness Review

Phase 50.29 adds a real backend execution readiness review.

The review is machine-readable and does not execute backend commands.

It reports:

- whether the plan is executable
- whether the request is a write operation
- whether execute mode is requested
- whether explicit operator confirmation is present
- whether executor opt-in is present
- whether a command executor is injected
- whether the path is controlled-test-only
- whether a production real-execution policy gate exists
- satisfied conditions
- blockers

The current production readiness result is intentionally false.

The controlled test executor path remains useful for proving the internal execution chain, but it is explicitly reported as not production real execution.

### Production Executor Hardening Plan

Phase 50.30 adds a production executor hardening plan.

The plan is machine-readable and does not execute backend commands.

Satisfied requirements currently include:

- dry-run, prepare and execute mode separation
- executor opt-in boundary
- executor injection boundary
- guard and kill-switch chain
- controlled test executor path
- executor invocation audit trail
- real execution readiness review

Missing production requirements currently include:

- production execution enable switch
- backend write allowlist
- per-backend write permission gate
- production real-execution policy gate
- mandatory backend readback verification
- failure compensation or rollback policy
- REST production execution boundary

The plan intentionally reports readyForProductionExecution=false.

This is the final planning boundary before implementing a real execution enablement switch.

### Real Execution Enablement Switch

Phase 50.31 adds a disabled-by-default production real execution enablement switch.

The switch is a contract boundary only.

It does not expose REST production execution and it does not bypass the remaining production hardening requirements.

The controlled test executor path remains separate from production execution.

Production execution still requires later phases for:

- backend write allowlist
- per-backend write permission
- production policy gate
- mandatory backend readback verification
- failure compensation behavior
- REST production execution boundary

### Update Readback Verification Service

Phase 50.43 adds the update-readback verification service for future verified SearchTimer write execution.

The service checks whether a successful update result is visible in backend state with the expected backend-native id and updated content.

Full service documentation:

- [SearchTimer Update Readback Verification Service](searchtimer-update-readback-verification-service.md)

---
### Create Readback Verification Service

Phase 50.42 adds the create-readback verification service for future verified SearchTimer write execution.

The service checks whether a successful create result is actually visible in backend state through `ISearchTimerDataSource`.

Full service documentation:

- [SearchTimer Create Readback Verification Service](searchtimer-create-readback-verification-service.md)

---
### Backend Readback Verification Result Model

Phase 50.41 adds the first explicit domain result model for future mandatory backend readback verification.

The model is not a production mutation switch.

It prepares later create/update readback verification services by separating executor success from verified backend state.

Full model documentation:

- [SearchTimer Backend Readback Verification Result Model](searchtimer-backend-readback-verification-result-model.md)

---
### Mandatory Backend Readback Verification Plan

Phase 50.40 defines the mandatory backend readback verification plan for future production SearchTimer write execution.

The current dispatch skeleton can map write commands and already reports that backend readback will be required after real execution.

The next implementation step is to turn that warning into an explicit verification result model and later into enforced create/update readback behavior.

Full plan:

- [SearchTimer Mandatory Backend Readback Verification Plan](searchtimer-backend-readback-verification-plan.md)

---
### yaVDR Local API Smoke Harness Execution Report

Phase 50.39 records the successful execution of the Phase 50.38 local VDR-Suite API smoke harness.

Verified endpoint:

- `POST /api/searchtimers/real-test`

Verified result:

- HTTP 200
- `dispatchStage=production-policy-gate-closed`
- `executorInvocationAttempted=false`
- `dryRunOnly=true`
- `executed=false`
- `RESULT: OK`

Full report:

- [SearchTimer yaVDR Local API Smoke Harness Execution Report](searchtimer-yavdr-api-smoke-harness-execution-report.md)

---
### yaVDR Local API Smoke Harness

Phase 50.38 adds a local HTTP harness for the VDR-Suite SearchTimer real-test endpoint.

Tool:

- `apps/tools/searchtimer_yavdr_api_smoke_harness.cpp`

Make targets:

- `make searchtimer-yavdr-api-smoke-harness-helper`
- `make searchtimer-yavdr-api-smoke-harness-run`

The harness serves:

- `POST /api/searchtimers/real-test`
- `POST /api/vdr/searchtimers/real-test`

The run target starts the local harness on `127.0.0.1:18080` and runs the Phase 50.36 smoke script against that local endpoint.

Expected outcome remains non-mutating:

- `dispatchStage=production-policy-gate-closed`
- `executorInvocationAttempted=false`
- `RESULT: OK`

Full documentation:

- [SearchTimer yaVDR Local API Smoke Harness](searchtimer-yavdr-api-smoke-harness.md)

---
### yaVDR Smoke-Test Execution Report

Phase 50.37 records the first successful yaVDR-side SearchTimer real smoke-test execution.

Verified direct RESTfulAPI path:

- RESTfulAPI reachable on `127.0.0.1:8002`
- `/searchtimers.json` returned HTTP 200
- create succeeded
- readback after create succeeded
- update succeeded
- readback after update succeeded
- delete cleanup succeeded
- readback after delete succeeded
- final `/searchtimers.json` returned an empty list again

Observed environment boundary:

- `127.0.0.1:8080` was the VDR-Rectools/PHP dashboard, not VDR-Suite
- no VDR-Suite API process was observed during the smoke run
- the VDR-Suite real-test endpoint therefore still requires a local API harness or daemon process before it can be tested live

Full report:

- [SearchTimer yaVDR Smoke-Test Execution Report](searchtimer-yavdr-smoke-test-execution-report.md)

---
### yaVDR Smoke-Test Script

Phase 50.36 adds an operator-facing smoke-test script for the non-mutating SearchTimer real-test endpoint.

Script:

- `tools/run_searchtimer_yavdr_real_test.py`

Local validation:

- `python3 tools/run_searchtimer_yavdr_real_test.py --self-test`

Example against a running VDR-Suite API:

- `python3 tools/run_searchtimer_yavdr_real_test.py --run --base-url http://127.0.0.1:8080 --backend home-vdr --print-json`

Environment variables:

- `VDR_SUITE_API_BASE_URL`
- `VDR_SUITE_SEARCHTIMER_TEST_BACKEND`
- `VDR_SUITE_SEARCHTIMER_TEST_OPERATION`
- `VDR_SUITE_SEARCHTIMER_TEST_NATIVE_ID`
- `VDR_SUITE_SEARCHTIMER_TEST_NAME`
- `VDR_SUITE_SEARCHTIMER_TEST_QUERY`
- `VDR_SUITE_SMOKE_TIMEOUT`

Safety behavior:

- the script calls the VDR-Suite real-test endpoint, not RESTfulAPI directly
- the endpoint remains non-mutating
- expected final stage is `production-policy-gate-closed`
- expected `executorInvocationAttempted` is `false`
- the script prints warnings and audit trail entries for operator review

---

### yaVDR Real-Test Mode

Phase 50.35 adds a controlled yaVDR real-test mode endpoint.

Routes:

- `POST /api/searchtimers/real-test`
- `POST /api/vdr/searchtimers/real-test`

The endpoint parses the same workflow body as validate/plan/execute, injects a non-mutating real-test executor and returns normal workflow execution JSON with operator-visible warnings and audit trail entries.

Current behavior:

- real backend mutation is not performed
- the production policy gate remains closed
- the response exposes the final blocker
- the response keeps `executorInvocationAttempted=false`
- this endpoint is intended for controlled yaVDR-side smoke tests before any future production mutation path exists

---

### Production Policy Gate

Phase 50.34 adds the production policy gate.

Current behavior:

- the production policy gate contract exists
- the gate is closed by default
- requests that pass switch, allowlist and per-backend permission are still blocked by the closed production policy gate
- real VDR backend mutation remains out of scope
- the next practical step is a controlled yaVDR real-test mode

This completes the current safety gate chain before real environment testing.

### Per-Backend Write Permission Gate

Phase 50.33 adds a per-backend write permission gate.

This is stricter than the backend write allowlist.

Current behavior:

- read-only workflows do not require write permission
- write workflows without a configured permission gate are blocked
- write workflows with a backend not present in the permitted backend set are blocked
- write workflows with a permitted backend can advance to the next production gate
- backend mutation is still not enabled

The permission gate is part of the production execution gate chain and remains separate from the controlled test executor path.

### Backend Write Allowlist

Phase 50.32 adds a backend write allowlist gate.

The allowlist is required for production SearchTimer write execution.

Current behavior:

- read-only workflows do not require the write allowlist
- write workflows without a configured allowlist are blocked
- write workflows with a backend not present in the allowlist are blocked
- write workflows with an allowlisted backend can advance to the next production gate
- backend mutation is still not enabled

The allowlist is part of the production execution gate chain and remains separate from the controlled test executor path.

### Typical Client Flow

Recommended client flow:

1. Build a workflow request from the operator action.
2. Submit it to POST /api/searchtimers/validate.
3. Submit it to POST /api/searchtimers/plan.
4. Display the planned operation, primary step, follow-up step and confirmation requirements.
5. Require explicit operator confirmation when writeOperation is true.
6. Submit the confirmed request to POST /api/searchtimers/execute.
7. Treat executed=false and dryRunOnly=true as proof that no backend mutation happened.
8. Wait for a later real execution phase before using the endpoint for actual backend writes.

---

## Client Behavior

Recommended client behavior:

1. Build a workflow request from the operator action.
2. Submit it to POST /api/searchtimers/validate.
3. Display warnings and errors.
4. Require explicit operator confirmation for writeOperation true.
5. Block execution if valid is false.
6. Prefer readback after create and update when wantsReadbackAfterWrite is true.
7. Only call the real create, update or delete endpoint after validation succeeds.

---

## Relationship to Real Write Endpoints

Validation endpoint:

    POST /api/searchtimers/validate

Real write endpoints:

    POST /api/searchtimers
    POST /api/searchtimers/update
    POST /api/searchtimers/delete

The validation endpoint prepares user intent.

The real write endpoints execute backend operations through the command executor boundary.

---

## Out of Scope

This endpoint does not yet provide:

- authorization policy
- profile-based permission decisions
- conflict resolution
- automatic creation
- automatic update
- automatic deletion
- backend capability negotiation beyond workflow shape
- full Live UI parity

Those concerns belong to later phases.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
