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
