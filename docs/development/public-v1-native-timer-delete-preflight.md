# Public API v1 native Timer DELETE — bounded preflight

Status (2026-10-19): source-only read-only preflight, no public DELETE
endpoint, no running-daemon change, no native mutation.

The existing GET /api/v1/native-timers snapshot may show native Timers
created outside VDR-Suite. The existing Phase-64 preparation, dispatch,
Agent and SuiteBridge deletion chain accepts managed or adopted
NativeTimerBindings, while the Agent contract requires a nonempty
TimerAssignment. External bindings are deliberately rejected.

NativeTimerDeletePublicPreflight.h now checks a *separately resolved*
durable binding against an exact backend, backend generation, binding
identity, backend-native Timer ID, binding revision, canonical observed
state fingerprint and server-owned minimum observation time. Invalid,
stale, missing, drifted, recording and pending timers fail closed.
External ownership returns adoptionRequired; ambiguous and orphaned
ownership is rejected. This pure function neither verifies RBAC nor
reserves a MutationOperation, dispatches a command, promotes ownership,
or asserts native absence. An independent exact grant and current
provider/lease fence are mandatory before a future call.

Next required integration gates:
1. Public native Timer detail read with authoritative binding identity
   and a strong ETag (not a mutable VDR list index).
2. Separate authorized external Timer adoption plus durable assignment.
   Never silently promote external ownership during DELETE.
3. Public DELETE method routed via SecurityHttpGate with exact backend
   timers.delete, strong If-Match, idempotency, operation lookup/replay
   and OpenAPI contract; unavailable until wired end-to-end.
4. Existing Phase-64 Operation -> Agent -> SuiteBridge dispatch with
   native lock-time CAS, provider/generation fence, post-dispatch absence
   readback, and outcome_unknown reconciliation. No blind retries.

The test-fast regression covers exact identity, freshness, revision,
generation, fingerprint, external ownership and unsafe states. This
guard is not a production authorization or public API milestone.
