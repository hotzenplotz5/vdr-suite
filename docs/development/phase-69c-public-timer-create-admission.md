# Phase 69.C — Public Timer CREATE Admission

## Status

**ACTIVE — bounded public HTTP admission after accepted PR #332.**

Baseline:

```text
main=1f19726a0b387a7b1de3f078dbf836a51254e724
PR #332 head=c1d156408fd3895a33499635ec5475340d6d049b
PR #332 CI=35960066328 / #9130 / SUCCESS (6/6)
69.C=ACTIVE
```

Binding architecture:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049](../adr/ADR-0049-audit-security-event-model.md)
- [Public TimerAssignment Resource](phase-69c-public-timer-assignment-resource.md)
- [Atomic Timer CREATE Admission Foundation](phase-69c-atomic-timer-create-admission.md)

## Scope

This slice opens the first stable public-v1 Timer mutation admission:

```http
POST /api/v1/timer-assignments/{timerAssignmentId}?backend={backendId}
Content-Type: application/json
If-Match: "<opaque strong VDR-Suite ETag>"
Idempotency-Key: <opaque caller key>

{}
```

The route submits CREATE for the already durable TimerAssignment. It does not
create a new TimerIntent or accept VDR/native Timer fields.

The existing pre-v1 compatibility route remains separate:

```text
POST /api/vdr/timers/actions/create
```

This slice does not rename, alias or declare that route stable v1.

## Security ordering

SecurityHttpGate owns authentication and backend-scoped authorization before the
public API admission callback can observe the resource.

The public POST is a protected mutation:

```text
authenticate
-> validate explicit ?backend= scope
-> authorize timers.create@backend
-> browser-session CSRF when applicable
-> PublicApiRuntime HTTP contract
-> backend write policy
-> atomic Timer CREATE admission
```

The backend identity is taken from the validated query scope. It is not accepted
from the JSON body and there is no implicit default backend.

Authorization therefore happens before TimerAssignment existence can be
disclosed.

## Closed request body

The public request representation is deliberately empty:

```json
{}
```

The desired native Timer specification already belongs to the durable internal
TimerAssignment and remains hidden from the public API.

Rules:

- `Content-Type` must be `application/json`; media-type parameters such as
  `charset=utf-8` are accepted;
- request body is bounded to 4096 bytes;
- malformed JSON returns `400 invalid_json`;
- syntactically valid values other than an empty JSON object return
  `422 validation_error`;
- unknown fields are therefore rejected rather than ignored.

No public field exposes or accepts:

- `NativeTimerSpecification`;
- VDR day/weekdays/HHMM syntax;
- backend generation;
- assignment epoch;
- NativeTimerBinding identity;
- caller-generated operation identity.

## Strong If-Match

TimerAssignment GET already returns a strong opaque ETag derived from its durable
assignment revision.

CREATE requires exactly one canonical strong VDR-Suite ETag.

Missing:

```text
428 precondition_required
```

A weak tag, wildcard, list or non-canonical tag is structurally invalid and
returns `400 invalid_request`.

The server can reverse its own canonical `vsr-...` representation to the
original resource revision. This is a server-internal transport operation;
clients still treat the ETag as opaque.

### Why current-state comparison is not done first

The accepted atomic admission owner resolves the durable ADR-0042 idempotency
scope before evaluating current TimerAssignment state.

That ordering is required for response-loss retry:

```text
first request:
  If-Match = selected revision 7
  admission commits
  TimerAssignment becomes provisioning revision 8
  operation response is lost

exact retry:
  same If-Match for revision 7
  same Idempotency-Key
  -> recover existing durable operation
  -> do not mint identities
  -> do not mutate assignment again
  -> do not redispatch
```

Comparing the retry's If-Match directly to current revision 8 before idempotency
resolution would incorrectly return 412 for the exact same logical request.

For a new logical request with a stale revision, the atomic admission owner
returns revision conflict and the public API maps it to:

```text
412 revision_conflict
```

A refreshed mutation must use refreshed state and a new Idempotency-Key as
required by ADR-0048.

## Idempotency-Key

`Idempotency-Key` is required.

This first public contract accepts a non-empty opaque visible-ASCII key of at
most 160 bytes. Whitespace/control characters and oversized values are rejected
with `400 invalid_request`.

The key is not an operation ID and is never exposed in the public operation
representation.

The durable ADR-0042 scope remains:

```text
actor
+ backend
+ resourceType=TimerAssignment
+ timerAssignmentId
+ actionFamily=timer.create
+ Idempotency-Key
```

The normalized admission fingerprint also binds the submitted backend,
TimerAssignment identity and original pre-mutation assignment revision.

After the current authentication, authorization and backend mutation-policy
gates still permit the request, same scope + same fingerprint returns the
existing operation. Idempotent replay does not bypass a permission revocation or
a backend that has since become read-only, matching the ADR-0042 decision
ordering.

Same scope + different fingerprint returns:

```text
409 idempotency_conflict
```

## Backend write policy

Authorization and backend write policy are separate gates.

After `timers.create` authorization, the daemon callback evaluates the existing
`BackendAccessPolicy::canWriteToBackend()`.

Public mapping:

- read-only backend -> `403 read_only_backend`;
- unavailable/disabled/not-resolvable writable backend ->
  `503 backend_unavailable`.

Only an allowed backend reaches `NativeTimerCreateAdmissionService::admit()`.

## Durable admission response

A new or exact-replayed admission returns:

```http
HTTP/1.1 202 Accepted
Location: /api/v1/operations/{operationId}
ETag: "<operation revision ETag>"
Cache-Control: no-store
```

The body is the existing minimal public operation representation:

```json
{
  "operationId": "op_...",
  "state": "accepted",
  "backendId": "backend_...",
  "links": {
    "self": "/api/v1/operations/op_..."
  }
}
```

The operation representation does not expose the idempotency key, request
fingerprint, immutable native payload, NativeTimerBinding identity or internal
revision field.

The client reconciles through:

```text
GET /api/v1/operations/{operationId}
```

It does not repeatedly submit a new command.

## Public error mapping

The admission surface maps immediate failures as follows:

| Condition | HTTP | code |
| --- | ---: | --- |
| malformed JSON | 400 | `invalid_json` |
| malformed/missing Idempotency-Key | 400 | `invalid_request` |
| malformed/non-canonical If-Match | 400 | `invalid_request` |
| unauthenticated | 401 | `unauthorized` |
| permission denied | 403 | security decision code |
| read-only backend | 403 | `read_only_backend` |
| hidden/missing assignment | 404 | `not_found` |
| unsupported method | 405 | `method_not_allowed` |
| state/operation conflict | 409 | `operation_conflict` |
| backend generation conflict | 409 | `generation_conflict` |
| idempotency conflict | 409 | `idempotency_conflict` |
| stale submitted revision | 412 | `revision_conflict` |
| unsupported Content-Type | 415 | `unsupported_media_type` |
| non-empty/invalid public action object | 422 | `validation_error` |
| missing If-Match | 428 | `precondition_required` |
| backend unavailable | 503 | `backend_unavailable` |
| required Suite runtime unavailable | 503 | `service_unavailable` |

No failure in this slice triggers a fresh mutation automatically.

## Accountability boundary

The public CREATE is a protected Phase-62 mutation, so the existing security
gate persists authorization and HTTP-outcome evidence.

The operation identity is Suite-issued only after authorization. Therefore the
SecurityGate decision cannot contain a caller-provided operation ID.

This is consistent with the accepted Phase-62 outcome contract, which carries
an operation ID **when present**, and with ADR-0049, where operation correlation
is optional context.

The focused security regression explicitly proves that a successful public
CREATE authorization/outcome pair may have an empty outer HTTP
`AccountabilityEvent.operationId`.

No response-body parsing or synthetic operation identity is added to the
security layer. The durable MutationOperation remains the authoritative
operation lifecycle and later operation/Agent evidence can use its real ID.

A Phase-62 `operation.succeeded` event for HTTP 202 means the protected HTTP
admission succeeded. It does not claim native Timer execution or readback
verification succeeded.

## Still closed: native dispatch

This slice ends after durable atomic admission.

It does not call:

```text
backendAgentNativeTimerCreateReservationService_->reserve(...)
nativeTimerCreateDispatchService_->claimAfterReservation(...)
backendAgentNativeTimerCreateActivationService_->activateDispatching(...)
```

Consequently:

- no Agent command becomes pollable;
- no SuiteBridge CREATE is sent;
- no VDR Timer is created;
- no native outcome is claimed;
- no speculative retry/fallback exists.

The already composed dispatch owners remain dormant.

## Capability

`GET /api/v1/capabilities` advertises:

```text
public-api.timer-create-admission / version 1
```

It is `available` only while the admission callback is installed.

## Acceptance

CI/build/architecture validation is sufficient because this slice opens durable
Control-Plane admission but not native execution.

The focused tests and guards must prove:

- strong canonical If-Match decoding without exposing raw revisions;
- exact replay retains the original submitted revision;
- closed JSON and Content-Type handling;
- bounded Idempotency-Key handling;
- backend-scoped `timers.create` authorization and browser CSRF;
- backend write policy before atomic admission;
- exact 202/Location/operation representation;
- 409/412/415/422/428 mappings;
- optional outer HTTP operation correlation for server-issued operation IDs;
- old pre-v1 Timer route remains separate;
- public layers do not own NativeTimerSpecification, identity issuance,
  fulfillment, binding repositories or preparation;
- no Agent reserve/claim/activate invocation exists.

No real yaVDR acceptance is required for this candidate because the VDR-native
path is still unreachable.

## Next bounded slice

Live review after this public admission contract was accepted proved one bounded
composition prerequisite before Agent activation can safely become reachable:
the existing Phase-64 readback-verification and operation-completion owners were
not yet composed in DaemonRuntime.

The successor
[Native Timer CREATE Reconciliation Runtime Composition](phase-69c-native-timer-create-reconciliation-runtime.md)
closes that runtime gap without making a command pollable.

Only after that prerequisite is accepted should Phase 69.C open the native-effect
handoff:

```text
durable accepted operation/payload
-> exact Agent command reservation
-> dispatch claim
-> activation/pollability
-> native executor
-> outcome application
-> authoritative readback/reconciliation
```

The first candidate that makes a real native Timer CREATE reachable requires
exact-head real yaVDR acceptance before merge.
