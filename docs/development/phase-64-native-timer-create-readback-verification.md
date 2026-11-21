# Phase 64 — Managed Native Timer CREATE Readback Verification

This slice verifies the result of one managed native Timer CREATE without inventing a backend-native identity and without retrying the native mutation.

The verifier consumes a durable `NativeTimerCreateReadbackExpectation` and evidence from one authoritative complete inventory. The evidence may contain unrelated managed Timer correlations, but successful verification requires exactly one managed correlation matching both the reserved `timerAssignmentId` and reserved `nativeTimerBindingId`.

A matching candidate is not sufficient by itself. Backend identity, backend generation, readback freshness and the managed `NativeTimerSpecification` must also match. Observation-only facts such as recording/pending status are retained in the resulting `NativeTimerBinding` but are not part of the desired-write specification comparison.

On verified CREATE, the service persists the first managed `NativeTimerBinding` with the native identity discovered by readback, the authoritative observed fingerprint/state and `lastVerifiedOperationId`. Repository uniqueness still enforces both backend-native identity and one managed binding per assignment.

Replaying the same verified operation is idempotent and returns `alreadyVerified`. Zero matching correlations return `correlationNotFound`; multiple matches return `correlationAmbiguous`; a desired-state mismatch returns `reconciliationRequired`. Existing binding/native/assignment conflicts fail closed.

Both `executed_unverified` and `outcome_unknown` expectations can converge through authoritative readback. An `outcome_unknown` result therefore never causes a blind retry: the old CREATE is resolved from evidence first.

This component performs no Agent dispatch, SuiteBridge call, VDR mutation, provider selection or reassignment. Runtime wiring is deliberately deferred until the surrounding managed fulfillment pipeline has durable dispatch/start/recovery semantics.
