# Public-v1 TimerAssignments and protected Timer CREATE — developer contract

## Navigation

- [API developer entry point](README.md)
- [Partial OpenAPI 3.1 reference](openapi.json)
- [Operation status and reconciliation](operation-status.md)
- [Accepted TimerAssignment collection contract](../docs/development/phase-69d-timer-assignment-collection.md)
- [Accepted Timer CREATE admission contract](../docs/development/phase-69c-public-timer-create-admission.md)
- [Public API compatibility and mutation semantics (ADR-0048)](../docs/adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)

## Scope and provenance

Source-verified against `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0` (2026-10-11). TimerAssignment item GET and CREATE admission are accepted Phase 69.C contracts; the collection is accepted Phase 69.D (PR #363). This is a description of accepted **source**, not a claim about a deployed daemon or real VDR native execution.

### 1. Enumerate TimerAssignments

```http
GET /api/v1/timer-assignments?backend=backend-one&limit=50&sort=timerAssignmentId&order=asc
```

An authenticated actor must hold `timers.view` for the exact **one** backend specified by the `backend` query key. The JS reference client uses the option name `backendId` but generates `?backend=...`. There is no implicit backend selection, multi-backend aggregation or native-VDR fallback.

Only `backend`, `limit` (1–100, default 50), `cursor` (opaque, at most 2048 characters), `sort=timerAssignmentId`, and `order=asc` are allowed; unknown or repeated fields are rejected. Items are sorted by `timerAssignmentId ASC`. The `ta1_` cursor is bound to the backend. A malformed or wrong-backend cursor returns **400**, not 409. Preserve the same backend while traversing.

```json
{
  "items": [{"timerAssignmentId":"assignment:one","backendId":"backend-one","links":{"self":"/api/v1/timer-assignments/assignment:one?backend=backend-one"}}],
  "page": {"limit":50,"nextCursor":null,"hasMore":false},
  "meta": {"partial":false},
  "links": {"self":"/api/v1/timer-assignments?backend=backend-one&limit=50&sort=timerAssignmentId&order=asc","next":null}
}
```

The collection is read from the durable Suite-owned repository: no live native Timer completeness guarantee, no collection ETag, no offset/total-count guarantee, and `meta.partial=false` for successful reads.

### 1a. Optional read-only planned schedule (candidate extension)

A collection item may additionally have `planned`, derived **only** from
the persisted, valid desired NativeTimerSpecification for that exact
Suite-owned assignment. The existing `timerAssignmentId`, `backendId`,
`links`, cursor/order and `timers.view@backend` contract do not change.
An assignment without a valid desired specification omits `planned`.

```json
{"timerAssignmentId":"assignment:one","backendId":"backend-one","planned":{"title":"Nachrichten","day":"20261017","weekdays":"-------","startTime":"2015","endTime":"2100"},"links":{"self":"/api/v1/timer-assignments/assignment:one?backend=backend-one"}}
```

`startTime` and `endTime` are desired native **HHMM strings**, **not**
Unix seconds, ISO timestamps or evidence that a recording started. `day`
and `weekdays` are the persisted VDR scheduling selectors, not converted
UTC dates or a promise of scheduling completeness. Clients must label these
values as *geplant*, never as live native timer state. Missing `planned`
means "Planungsdetails nicht verfügbar", not that a VDR recording has no
programme. No raw provider channel IDs, recording directories, native
timer IDs, binding IDs, assignment revisions, or decision evidence is
exposed. This collection still **does not enumerate all native VDR timers**.

The change is read-only and additive to Public **v1**; it does not add
Timer mutations, grant elevation, account discovery, native execution, or
a new /v2 route. Device/VIDAA clients must first verify actual deployed
backend compatibility and use only `/api/v1/timer-assignments?backend=...`.
This candidate is not a statement that the daemon is installed.

### 2. Read a specific TimerAssignment

```http
GET /api/v1/timer-assignments/assignment:one?backend=backend-one
```

With `timers.view@backend`, the response exposes only `timerAssignmentId`, `backendId`, and `links.self`. The strong opaque `ETag` is returned in a response **header**, never as raw `assignmentRevision` or as a JSON field. `If-None-Match: <ETag from prior GET>` can return **304 with an empty body** and the current `ETag`. Malformed conditions return `400 invalid_request`; a missing/wrong-backend item returns `404`. Do not infer the native timer ID, assignment state, role or VDR scheduling internals from this minimal resource.

### 3. Submit Timer CREATE safely

```http
POST /api/v1/timer-assignments/assignment:one?backend=backend-one
Content-Type: application/json
If-Match: <opaque strong ETag returned by item GET>
Idempotency-Key: <caller-owned visible-ASCII key>

{}
```

**Do not send a native Timer specification.** The only valid body is the empty JSON object `{}`, at most 4096 bytes. `Content-Type` must be `application/json` (optional media-type parameters allowed). Missing/invalid `If-Match` yields 428/400. The Idempotency-Key is required, 1–160 visible ASCII bytes (no whitespace or control characters), and binds a logical request under the authenticated actor, backend, assignment, action and original revision.

The security gate first verifies the actor and `timers.create@backend`; browser-session CSRF applies where relevant. The backend's write policy is independently enforced. `403 read_only_backend` is possible even with `timers.create` permission. Timer CREATE does not replace `/api/vdr/timers/actions/create` and must not retry on that private endpoint.

**Success is HTTP 202, not native Timer success.** The response is a durable Operation representation with `Location: /api/v1/operations/{operationId}` and an opaque Operation `ETag`. An exact accepted replay may return the same Operation, but it is still subject to current authentication and backend write policy.

If the original response is lost, do **not** blindly submit another POST or generate a new idempotency key. Any explicit same-logical-request replay must use the same scope, original `If-Match`, key and payload. Different fingerprints under the same key cause `409 idempotency_conflict`. See [Operation read/reconciliation](operation-status.md). The reference client never automatically retries or polls.

## HTTP results to handle

| Code | Meaning |
| --- | --- |
| `200` / `304` (GET item) | Visible resource / unchanged conditional read; 304 has no body |
| `200` (GET collection) | Suite-owned backend-scoped page; not a live native Timer list |
| `202` (POST item) | Accepted or replayed durable Operation, not completed native Timer |
| `400` | Invalid query/cursor/ETag/Idempotency-Key or malformed JSON (`invalid_request`, `invalid_json`, `invalid_backend_scope`) |
| `401` / `403` | Authentication / `timers.view` or `timers.create` authorization; backend may be read-only |
| `404` | Assignment absent or not visible under backend scope |
| `405` | Collection allows GET; item allows GET and POST (`Allow` headers) |
| `409` | `idempotency_conflict`, `generation_conflict` or `operation_conflict` |
| `412` | `revision_conflict` for outdated `If-Match` |
| `415` / `422` / `428` | Unsupported content type / nonempty JSON request or invalid submission / missing precondition |
| `503` | Runtime, repository, backend availability or accountability service unavailable |

Errors from the Public-v1 runtime use `application/problem+json` with stable `code`. An authorization rejection can occur earlier in the Security gate.

## Implementation and regression anchors

- [Public runtime parser, serializer and POST mapping](rest/src/PublicApiRuntime.cpp)
- [Public runtime typed resources](rest/include/PublicApiRuntime.h)
- [Authorization and browser CSRF gate](../core/security/include/SecurityHttpGate.h)
- [Timer collection tests](rest/tests/test_public_timer_assignment_collection.cpp)
- [Timer item/ETag tests](rest/tests/test_public_timer_assignment_resource.cpp)
- [Timer mutation admission test](rest/tests/test_public_timer_create_admission.cpp)
- [JavaScript reference client](../clients/reference-js/public-v1-client.js)
- [Timer CREATE reference client acceptance](../docs/development/phase-69f-public-v1-timer-create-reference-client.md)

This documentation-only change does not execute a native Timer mutation, install anything, or add a CI route/schema checker.

## Back

- [API entry point](README.md)
- [Documentation index](../docs/index.md)
