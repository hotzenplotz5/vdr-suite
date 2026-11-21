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

## Public native Timer detail read (2026-10-19 source integration)

The public GET route is now present in source at
\`/api/v1/native-timers/{timerId}?backendId=...\`, authenticated by
\`timers.view\` for the exact backend. A Daemon callback checks the
currently visible backend snapshot, unique exact Timer ID, Agent
generation, durable binding and full canonical native observed-state
fingerprint using \`NativeTimerPublicDetailRevision\`.

- \`binding_required\` means snapshot read succeeded but no binding
  exists: **no ETag**, and never mutation authority.
- \`adoption_required\` means a verified externally-owned binding:
  the strong ETag may be used for conditional reads, but DELETE
  remains forbidden until a separate explicitly authorized adoption.
- \`verified\` means a managed/adopted binding matches the snapshot:
  the conditional ETag is derived from durable binding ID, backend
  generation and binding revision, not an unverified VDR timer index.
- Any stale generation, fingerprint drift, deleted state or unsafe
  observation returns 503 instead of a guessed write precondition.

If-None-Match works only with the bound strong ETag (304 for a
match; malformed condition is 400). API runtime regression and
daemon translation-unit syntax-only test are wired into test-fast.
There is still no public DELETE method: 405 is the explicit result.
Do not expose native mutation or claim hardware acceptance on the
basis of this GET work alone.
