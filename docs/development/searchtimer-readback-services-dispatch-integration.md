# SearchTimer Readback Services Dispatch Integration

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [SearchTimer Create Readback Verification Service](searchtimer-create-readback-verification-service.md)
- [SearchTimer Update Readback Verification Service](searchtimer-update-readback-verification-service.md)
- [SearchTimer Delete Absence Verification Service](searchtimer-delete-absence-verification-service.md)
- [SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [SearchTimer Verified Execution REST Contract](searchtimer-verified-execution-rest-contract.md)
- [SearchTimer End-to-End Verified Execution Test](searchtimer-end-to-end-verified-execution-test.md)

---

## Purpose

Phase 50.47 integrates the SearchTimer readback verification services into the command dispatch path.

Create, update and delete executor results can now be followed by backend readback verification and attached to the workflow execution result.

Production mutation remains closed.

---

## Integration Point

The integration point is `SearchTimerWorkflowCommandDispatchService` after successful command executor result mapping.

The dispatch service now routes:

- successful create results to `SearchTimerWorkflowCreateReadbackVerificationService`
- successful update results to `SearchTimerWorkflowUpdateReadbackVerificationService`
- successful delete results to `SearchTimerWorkflowDeleteAbsenceVerificationService`

---

## Dispatch Options

`SearchTimerWorkflowCommandDispatchOptions` now carries an optional `ISearchTimerDataSource`.

A controlled test option can inject both command executor and readback data source.

This keeps readback integration testable without opening production mutation.

---

## Result Semantics

When a write operation requires backend readback and the executor succeeds:

- a readback verification result is attached
- `backendReadbackVerificationAttached` becomes true
- `backendReadbackVerified()` reflects the nested verification result
- failed readback forces final workflow success to false
- failed readback sets dispatch stage to `backend-readback-verification-failed`

---

## Delete Follow-Up

Delete now has a readback follow-up step like create and update.

For delete, the follow-up verifies absence of the deleted backend-native id.

---

## End-to-End Verified Execution Test

Phase 50.49 adds a dedicated end-to-end test for verified SearchTimer execution.

The test follows request planning, controlled executor invocation, readback verification and final execution-result semantics for create, update and delete.

Full test documentation:

- [SearchTimer End-to-End Verified Execution Test](searchtimer-end-to-end-verified-execution-test.md)

---

## REST Contract

Phase 50.48 adds REST-facing assertions and documentation for the verified execution response fields.

Full contract documentation:

- [SearchTimer Verified Execution REST Contract](searchtimer-verified-execution-rest-contract.md)

---

## Safety Boundary

This phase does not enable production mutation.

It only wires already-existing readback verification services into controlled dispatch flow.

---

## Back

- [Back to SearchTimer Delete Absence Verification Service](searchtimer-delete-absence-verification-service.md)
- [Back to SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [Back to Development Index](index.md)
