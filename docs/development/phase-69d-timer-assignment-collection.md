# Phase 69.D — First Public Collection: TimerAssignment

## Status

**CANDIDATE — first bounded 69.D collection slice.**

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69 Public API Kickoff](phase-69-public-api-kickoff.md)
- [Phase 69.C Closeout](phase-69c-closeout.md)

This slice opens one stable read collection only. It does not reopen Phase 64
Timer mutation design, ADR-0064, Home, LiveTV, Playback, Broadcast Companion or
Legacy OSD ownership.

## Live inventory and selection proof

The write gate for this slice was taken from live
`main=de8bd1ba189f3e7e014adb041a0fcc8b13d8f73e`, the merge of PR #362.
There were no open pull requests at that gate.

The live stable `/api/v1` inventory before this slice contained:

- `GET /api/v1` contract root;
- `GET /api/v1/capabilities`;
- actor-owned `GET /api/v1/operations/{operationId}`;
- backend-scoped `GET /api/v1/timer-assignments/{timerAssignmentId}?backend=...`;
- backend-scoped Timer CREATE on that TimerAssignment item.

It contained no stable collection.

The live pre-v1 inventory contains collection-shaped surfaces for Backends,
Channels, Events/EPG, Recordings, Timers, SearchTimers, Jobs, people/search and
metadata/history. ADR-0048 forbids promoting those shapes merely because they
already exist.

The first collection candidates were compared against the existing owners:

| Candidate | Existing owner/read model | Why it is not the first 69.D collection |
| --- | --- | --- |
| Operations | MutationOperationRepository | The accepted public item read is actor-owned, but there is no existing actor-list read facade. Opening a collection would first require a new owner-scoped query contract. |
| Backends | BackendRegistryService | `/api/backends` is pre-v1 and its serializer contains legacy/frontend-oriented shape. A stable public collection would also need a new actor-to-backend collection authorization/filter contract. |
| Jobs | JobRepository | The current global legacy job list is not an actor-owned stable public read model. |
| Channels / Events | VdrSnapshotReadService | Existing native/provider identities and future multi-backend ordering need a separate stable identity audit before public-v1 freezing. |
| Recordings | VdrSnapshotReadService / recording caches | This is Home/metadata/fanout sensitive and would unnecessarily couple the first 69.D slice to completed Phase-66 behavior. |
| SearchTimers | SearchTimer services | Existing collection conventions are pre-v1 and include legacy pagination/provider semantics. |
| TimerAssignments | TimerAssignmentRepository + TimerAssignmentReadService | The public item identity, item representation and exact `timers.view@backend` authorization are already accepted. The missing piece is only a bounded read query over the existing Suite-owned repository. |

Therefore the first stable public collection is:

```http
GET /api/v1/timer-assignments?backend={backendId}
```

The collection is deliberately **single-backend**. This is a read-only extension
of the already accepted TimerAssignment public resource, not new Timer
scheduling, mutation, fulfillment or native-VDR authority.

## Stable ordering

The only supported ordering in this slice is:

```text
timerAssignmentId ASC
```

`timerAssignmentId` is the immutable Suite-owned canonical identity and the
repository primary key. The repository performs the order and keyset boundary in
SQLite; the read facade verifies strict monotonic order and exact backend scope.

Clients may omit `sort` and `order`. If supplied, the only accepted values are:

```text
sort=timerAssignmentId
order=asc
```

Changing this default order in v1 would be a breaking collection-contract change.

## Limits

The contract is bounded:

```text
default limit = 50
maximum limit = 100
minimum limit = 1
```

The domain read facade asks the repository for at most `limit + 1` rows. The
extra row is only a bounded look-ahead to derive `hasMore`; it is never returned
to the client.

Non-decimal, zero or over-maximum limits return `400 invalid_request`.
Offset pagination is not accepted by this v1 collection.

## Cursor contract

Pagination is keyset-based. The cursor is **opaque** to clients and must be
round-tripped unchanged.

The internal cursor scope is deliberately independent of actor identity because authorization is re-evaluated on every request. It is bound to:

- this TimerAssignment v1 collection and cursor-codec version;
- the exact authorized backend;
- the fixed `timerAssignmentId ASC` ordering;
- the last returned immutable TimerAssignment identity.

There are no additional collection filters in this first slice.

A cursor from another backend, a malformed cursor, an unsupported cursor
version or otherwise invalid cursor returns:

```text
400 invalid_request
```

The cursor is deliberately stateless: it does not create a second cursor
repository or lifecycle.

### Cursor stability

This is a live keyset collection, **not a snapshot-isolation contract**.
TimerAssignment revision/state changes do not move an item because ordering is
only by immutable identity. The TimerAssignment repository has no delete path in
this collection contract, so an existing continuation boundary remains
reconcilable.

A TimerAssignment created concurrently with an identity that sorts before the
current boundary is visible on a fresh traversal, not retroactively inserted into
an already traversed page sequence. That behavior is explicit rather than
pretending the live collection is a snapshot.

There is no current `cursor_expired` state because this owner has no compaction
or deletion behavior that can make the keyset boundary unreconcilable. If that
owner contract changes, 69.D must define the stable expiration semantics before
silently changing cursor behavior.

## Response envelope

Successful responses use the ADR-0048 collection envelope:

```json
{
  "items": [
    {
      "timerAssignmentId": "assignment:...",
      "backendId": "backend-one",
      "links": {
        "self": "/api/v1/timer-assignments/assignment:...?backend=backend-one"
      }
    }
  ],
  "page": {
    "limit": 50,
    "nextCursor": null,
    "hasMore": false
  },
  "meta": {
    "partial": false
  },
  "links": {
    "self": "/api/v1/timer-assignments?backend=backend-one&limit=50&sort=timerAssignmentId&order=asc",
    "next": null
  }
}
```

There is no `total` guarantee.

The collection deliberately exposes only the already reviewed stable public
identity/scope fields. Assignment revisions remain item ETags; Timer intent
internals, Agent IDs, SuiteBridge IDs, provider IDs, backend generations,
native Timer IDs/bindings and scheduling evidence remain private.

## Empty collection and cache semantics

A successful backend-scoped read with no assignments is a normal complete
result:

```text
200
items=[]
page.hasMore=false
page.nextCursor=null
meta.partial=false
```

The collection remains `Cache-Control: no-store` under the existing public API
success headers. There is **no collection ETag** and no collection-level 304
contract in this slice: membership is a live mutable set and the repository does
not own a stable collection revision/snapshot token. Existing strong item ETags
are unchanged.

## Source failure and partial results

The chosen collection does not fan out to VDR backends or providers. The source
is one Suite-owned SQLite `TimerAssignmentRepository` query, scoped by
`backend_id`.

Therefore partial source success is structurally unreachable in this
**single-backend** slice and every successful response states:

```text
meta.partial=false
```

An offline backend is not a partial-result event for this read because no backend
I/O is performed. A repository/storage failure returns `503 service_unavailable`;
it must never be converted to an empty collection and must never fall back to a
pre-v1 route.

If a future public TimerAssignment collection aggregates multiple backends, that
is a separate 69.D boundary. It must first define safe per-source status,
cross-backend canonical ordering, duplicate resolution and all-sources-failed
semantics. This slice does not manufacture that federation.

## Duplicate identity and cross-backend ordering

`timer_assignment_id` is the durable repository primary key. Within the
mandatory backend scope, duplicate public identities are therefore not a normal
result. The read/runtime layers additionally verify strict ascending identities
and exact backend equality; an impossible inconsistent result is treated as
service failure rather than silently de-duplicated.

Cross-backend ordering is intentionally not applicable because `backend` is
mandatory and authorization is checked before the collection read.

## Authorization and public errors

The existing Phase-62 SecurityHttpGate path is reused unchanged in meaning:

```text
permission = timers.view
backendId  = the authorized backend query scope
action     = timers.view
```

Public behavior:

- unauthenticated -> `401`;
- missing/invalid backend scope -> `400 invalid_backend_scope`;
- backend outside the actor grant -> `403`;
- invalid limit/cursor/sort/order/unknown query member -> `400 invalid_request`;
- repository/read runtime unavailable -> `503 service_unavailable`;
- `POST /api/v1/timer-assignments` -> `405 Allow: GET`;
- the existing item `POST /api/v1/timer-assignments/{id}` Timer CREATE contract remains unchanged.

Human-readable problem text is not used as machine semantics; stable problem
codes remain the contract.

## Regression boundary

This slice changes only:

- the existing TimerAssignment repository/read facade;
- PublicApiRuntime collection handling;
- the existing TimerAssignment SecurityHttpGate authorization route family;
- DaemonRuntime composition/reset;
- Phase-69 inventory/docs/tests/guards.

It does not change Home rails, Home lifecycle, Live Preview/LiveTV, recording
metadata fanout, private first-party frontend routes, Timer CREATE dispatch,
SuiteBridge transport, provider selection or VDR mutation behavior.

No real-yaVDR acceptance is required for this slice because it adds a
Suite-owned SQLite/public HTTP read and no native runtime effect.
