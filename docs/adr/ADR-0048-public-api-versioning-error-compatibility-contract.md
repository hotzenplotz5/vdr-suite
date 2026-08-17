# ADR-0048: Public API Versioning, Error and Compatibility Contract

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0003: REST API as External Interface](ADR-0003-rest-api.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](ADR-0037-packaging-install-api-boundary.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](ADR-0047-legacy-osd-compatibility-bridge.md)

---

## Status

Accepted

Date: 2026-07-16

---

## Context

VDR-Suite is intended to serve several independent client families:

- the bundled Web frontend;
- Windows and Linux desktop clients;
- Android and iOS applications;
- television clients;
- automation and monitoring tools;
- selected third-party integrations.

These clients must consume one stable VDR-Suite platform contract rather than depending on:

- VDR-native structures;
- RESTfulAPI response quirks;
- epgsearch command syntax;
- SVDRP reply codes;
- plugin-specific JSON;
- direct Streamdev URLs;
- Backend Agent transport frames;
- local filesystem paths;
- current frontend implementation details.

The repository already contains substantial API foundations:

- `api/rest` controllers and serializers;
- an `ApiRouter` with many read and mutation routes;
- an HTTP abstraction whose lower-level response supports headers;
- backend-neutral domain and service boundaries;
- a DOM-free Web Client API wrapper;
- backend capability and read-only enforcement foundations;
- snapshot, change-feed and live-transport concepts;
- accepted mutation, operation, job, Timer, event, media-session and Legacy OSD contracts.

The current public surface is not yet a stable platform API.

Observed limitations include:

- routes are exposed below unversioned `/api/...` paths;
- equivalent functions sometimes have several aliases, for example `/api/searchtimers` and `/api/vdr/searchtimers`;
- the frontend wrapper probes fallback paths dynamically;
- fallback logic retries after any error rather than only after a proven unsupported route;
- mutation fallback can therefore risk duplicate or ambiguous dispatch;
- `ApiResponse` currently exposes only status, content type and body;
- ETag, Location, Retry-After, request ID, correlation ID and deprecation headers cannot pass through that type;
- many errors are ad-hoc JSON objects such as `{"error":"not found"}`;
- error strings are used as machine and user information simultaneously;
- some controllers return HTTP `200` even when the serialized domain result represents rejection or failure;
- current frontend errors are reduced to a JavaScript `Error` message and lose status, stable code, request ID and retry metadata;
- pagination uses several incompatible limit/offset conventions;
- backend aggregation and partial-source failure do not yet have one public representation;
- no compatibility policy defines additive versus breaking JSON changes;
- no deprecation policy distinguishes supported aliases from permanent contract paths;
- server build version, public API version, Agent protocol version and plugin schema version are not yet explicitly separated in the public contract.

Without a binding decision, the first client that happens to use an endpoint could accidentally freeze:

- an internal route name;
- a backend-specific field;
- an error message string;
- an unstable identifier;
- an implicit default backend;
- a response ordering accident;
- a temporary fallback path;
- a test-only Basic Authentication behavior;
- a plugin or Agent schema version.

This ADR completes the target decision for architecture gaps G-03, G-06, G-25 and G-26 where they concern the public client contract. Runtime implementation remains Phase 68 work, with prerequisite identity, authorization, Agent, Timer, streaming, Broadcast Companion and Legacy OSD phases remaining authoritative.

---

## Decision

VDR-Suite introduces a versioned, client-independent public HTTP API below:

```text
/api/v1
```

The public API is owned by the VDR-Suite Control Plane and its public API layer.

It is distinct from:

```text
Public Client API
  /api/v1/...

Backend Agent Protocol
  separately versioned authenticated machine protocol

Media Plane
  Streaming Gateway connections and media access grants

Legacy OSD Data Plane
  sequenced frame/delta transport and fenced input commands

Plugin Local Contract
  plugin capability, snapshot and command schemas

Internal C++ Services
  no public ABI promise
```

A public API version never implicitly versions the Agent protocol, media transport, OSD frame protocol or plugin-local schema.

The API contract includes:

- path and schema versioning;
- request and response metadata;
- stable error representation;
- HTTP status semantics;
- revisions and conditional requests;
- idempotent mutation submission;
- collection and pagination rules;
- partial multi-backend result rules;
- capability negotiation;
- deprecation and compatibility rules;
- client-wrapper behavior;
- security and information-disclosure constraints;
- machine-readable documentation and compatibility tests.

---

## API Version Model

### Major version in the path

The public major version is part of the path:

```text
/api/v1/...
/api/v2/...
```

A minor version is not placed in every route.

Within one major version, VDR-Suite may make compatible additive changes. Breaking changes require a new major API version or a separately negotiated feature contract.

### Public API version is not server version

The following values are separate:

| Value | Meaning |
| --- | --- |
| `apiVersion` | Public client contract major, for example `v1`. |
| `serverVersion` | VDR-Suite software release. |
| `agentProtocolVersion` | Control Plane to Backend Agent protocol contract. |
| `pluginContractSchema` | Backend-local plugin contract schema. |
| `resourceRevision` | Opaque version of one mutable resource. |
| `snapshotGeneration` | Snapshot/cache generation. |
| `backendGeneration` | Active backend runtime generation. |
| `eventSequence` | Ordered event/change-feed position. |

Clients must not infer API compatibility from the server package version or VDR version.

### Contract root

`GET /api/v1` is the public contract root.

It returns a small representation containing at least:

- `apiVersion`;
- `serverVersion`;
- supported public API majors;
- platform capability link;
- documentation or schema link where enabled;
- authentication state appropriate for the caller;
- canonical links to top-level resources the caller may discover.

The contract root is not a full backend snapshot and does not leak unauthorized resources.

---

## Current Unversioned Routes

Existing `/api/...` routes are classified as pre-v1 compatibility routes.

They are not automatically declared to be the stable `v1` contract merely because they exist today.

Migration rules:

1. A canonical `/api/v1/...` route is implemented and documented.
2. Existing unversioned behavior is mapped explicitly to the new service or retained behind a compatibility adapter.
3. A legacy alias receives deprecation metadata once the canonical replacement is available.
4. The bundled Web Client API moves to the canonical v1 route.
5. Compatibility tests prove whether the alias is equivalent, intentionally reduced or unsupported.
6. Removal occurs only under the published deprecation policy.

Server-side aliases are preferred over client-side speculative route retries.

### No speculative mutation fallback

A client must never submit a second mutation merely because the first route returned:

- a network error;
- timeout;
- `500`;
- `502`;
- `503`;
- `504`;
- an invalid response body;
- `outcome_unknown`;
- any other result that does not prove that backend dispatch did not begin.

Mutation compatibility is handled by:

- one negotiated canonical route;
- a server-side alias;
- the same durable operation identity and idempotency key;
- or an explicit route-not-supported response that proves no dispatch occurred.

The preferred v1 behavior is no mutation path fallback at all.

Safe `GET` fallback may exist only during migration and only for explicitly classified unsupported-route responses. It must not hide authentication, authorization, backend, parser or server failures.

---

## Public Resource Model

The API exposes Suite-owned resources and operations.

Examples include:

```text
/api/v1/backends
/api/v1/channels
/api/v1/program-events
/api/v1/recordings
/api/v1/timer-intents
/api/v1/timer-assignments
/api/v1/search-timers
/api/v1/operations
/api/v1/jobs
/api/v1/media-sessions
/api/v1/legacy-osd-sessions
/api/v1/capabilities
```

The final implementation may introduce additional subresources, but every public resource uses stable Suite identity rather than:

- VDR pointers;
- native list positions;
- mutable Recording paths;
- raw RESTfulAPI IDs without backend scope;
- hostnames or IP addresses;
- plugin service pointers;
- Streamdev URLs;
- local file descriptors;
- user-visible titles as identity.

### Backend scope

A mutation always identifies its backend scope explicitly when a backend owns the native state.

There is no implicit default backend for a production mutation.

Read endpoints may support an authorized aggregate scope. Such an endpoint must state whether omitted `backendId` means:

- all authorized backends;
- one configured default;
- or invalid request.

The behavior must not vary silently by client type.

### HTTP methods

Target public semantics are:

| Method | Use |
| --- | --- |
| `GET` | Safe resource or collection read. |
| `POST` | Create a resource, submit a command or create an operation/session. |
| `PATCH` | Partial update when the domain supports a stable patch contract. |
| `PUT` | Full replacement only when the complete representation is authoritative. |
| `DELETE` | Request deletion/cancellation when the domain contract permits it. |

No `GET` route performs mutation.

Domain commands remain acceptable where a simple CRUD model would be misleading. A command endpoint must still produce or reference a durable operation when ADR-0042 or ADR-0043 requires one.

---

## Request Context

Every public request has a server-generated request identity.

Canonical request metadata includes:

| Header or value | Meaning |
| --- | --- |
| `X-Request-ID` | Unique identity for one HTTP request. |
| `X-Correlation-ID` | Optional identity linking related requests, operations, jobs and Agent activity. |
| `Idempotency-Key` | Caller key for one logical mutation submission. |
| `If-Match` | Required current resource representation for protected mutation. |
| `If-None-Match` | Conditional safe read. |
| `Authorization` | Authenticated client credential as defined by the identity phase. |
| `Accept` | Requested representation type. |
| `Content-Type` | Submitted representation type. |

### Request ID rules

- The server creates `X-Request-ID` when the client does not provide an acceptable value.
- The final value is returned in every response, including errors.
- Request IDs are opaque and are not authorization credentials.
- A duplicate client-supplied request ID does not create idempotency.
- Untrusted values are length-limited and normalized or replaced.

### Correlation ID rules

- A correlation ID may group a user workflow, operation, job, Agent dispatch or reconciliation chain.
- It does not replace `operationId`, `jobId`, `attemptId`, `dispatchId` or audit-event identity.
- Public clients may propose a correlation ID, but the server controls the accepted value.
- Trusted internal propagation rules are separate from public authorization.

### Request body rules

- JSON requests use UTF-8 and `application/json` unless another media type is explicitly documented.
- Request field names use lower camel case.
- Request objects are closed by default: unknown fields are rejected as validation errors.
- Explicit extensibility uses a documented namespaced `extensions` object.
- Unknown request enum values are rejected unless the endpoint explicitly defines pass-through behavior.
- Credentials, session secrets and bearer tokens are never accepted in query parameters.

Strict request parsing prevents a misspelled mutation field from being silently ignored.

---

## Success Representation Rules

VDR-Suite does not require a universal `data` wrapper around every successful response.

Instead:

- a single-resource read returns that resource representation;
- a collection returns the standard collection shape;
- a mutation submission returns the created resource, result or durable operation representation;
- an asynchronous submission returns the operation representation and a `Location` header;
- an empty successful response may use `204`.

Stable metadata belongs in documented fields or response headers, not in unrelated domain objects.

### Standard collection shape

A v1 collection uses:

```json
{
  "items": [],
  "page": {
    "limit": 50,
    "nextCursor": null,
    "hasMore": false
  },
  "meta": {
    "partial": false
  },
  "links": {
    "self": "/api/v1/recordings"
  }
}
```

`items` and `page` are required for paginated collections.

`meta` and `links` are additive extension points with documented semantics.

A total count is optional. Clients must not assume that every federated or expensive collection can provide an exact total.

---

## Error Representation

All public v1 errors use a Problem Details-compatible JSON representation and content type:

```text
application/problem+json
```

Canonical shape:

```json
{
  "type": "urn:vdr-suite:error:revision-conflict",
  "title": "Resource revision conflict",
  "status": 412,
  "detail": "The Recording changed after it was read.",
  "instance": "/api/v1/recordings/rec_123",
  "code": "revision_conflict",
  "requestId": "req_...",
  "correlationId": "corr_...",
  "operationId": "op_...",
  "retryable": false,
  "errors": [
    {
      "field": "ifMatch",
      "code": "stale_revision",
      "message": "Refresh the resource before retrying."
    }
  ],
  "meta": {
    "currentRevision": "opaque-revision-token"
  }
}
```

Required fields:

- `type`;
- `title`;
- `status`;
- `code`;
- `requestId`.

Optional fields:

- `detail`;
- `instance`;
- `correlationId`;
- `operationId`;
- `jobId`;
- `backendId`;
- `retryable`;
- `errors`;
- `meta`.

### Stable versus human-readable fields

Clients branch on:

- HTTP status;
- stable `code`;
- structured fields.

Clients do not branch on:

- `title`;
- `detail`;
- field `message`;
- translated display text.

Human-readable messages may improve without an API-major change.

### Error code rules

- Public codes are stable lowercase snake case.
- Adapter-native codes may be retained in protected diagnostic metadata, but do not become the primary public code.
- Unknown codes must be handled by clients using the HTTP status and a generic fallback.
- A code is not reused for a different meaning.
- Security-sensitive details are omitted even when internal logs contain richer evidence.

### Field errors

Validation errors may contain an `errors` array.

Each field error contains at least:

- a stable field path;
- a stable code;
- an optional human-readable message.

Field paths refer to the public request representation, not C++ member names, SQL columns or plugin fields.

---

## HTTP Status Mapping

The public API uses HTTP status as transport and immediate decision semantics.

| Status | Public meaning |
| ---: | --- |
| `200` | Successful synchronous read or command result, or retrieval of an operation in any stored state. |
| `201` | Resource created synchronously. |
| `202` | Durable operation or session request accepted but not finally completed. |
| `204` | Successful request with no response body. |
| `304` | Conditional read not modified. |
| `400` | Malformed JSON, invalid query syntax or structurally invalid request. |
| `401` | Authentication required or invalid. |
| `403` | Authenticated actor is forbidden, including server-enforced read-only denial. |
| `404` | Resource or route not found, including existence-hiding policy where required. |
| `405` | Method not allowed; response includes `Allow`. |
| `409` | State conflict not represented by an HTTP precondition, including generation, idempotency or controller-lease conflict. |
| `410` | Resource, grant or compatibility route is permanently gone where disclosure is safe. |
| `412` | `If-Match` or another submitted precondition failed. |
| `415` | Unsupported request media type. |
| `422` | Syntactically valid request with invalid domain fields or action semantics. |
| `428` | Required precondition such as `If-Match` is missing. |
| `429` | Rate or concurrency limit exceeded. |
| `500` | Unexpected server failure with no internal detail leakage. |
| `502` | Invalid or failed upstream/provider response where the gateway role is applicable. |
| `503` | Required backend, capability, Agent or service is unavailable. |
| `504` | Upstream timeout where no more precise durable-operation representation is available. |

### Important operation distinction

HTTP success and domain completion are different facts.

A durable operation resource may be retrieved with `200` while its state is:

- `queued`;
- `dispatching`;
- `failed_verified`;
- `outcome_unknown`;
- `cancelled`.

Submission may return `202` with an operation in `outcome_unknown` when the operation exists and reconciliation is required.

The server must not convert an uncertain backend outcome into a generic `500` or blindly dispatch a replacement mutation.

Immediate validation, authorization, capability and precondition rejection use non-2xx responses and do not create misleading success responses.

---

## Canonical Error Categories

ADR-0042 categories map into the public contract as follows:

| Public code | Typical status | Meaning |
| --- | ---: | --- |
| `invalid_json` | `400` | Request body is not valid JSON. |
| `invalid_request` | `400` | Request structure or query syntax is invalid. |
| `validation_error` | `422` | Domain fields or action payload are invalid. |
| `unauthorized` | `401` | No valid actor identity. |
| `forbidden` | `403` | Actor lacks permission or policy denies access. |
| `read_only_backend` | `403` | Backend policy prohibits mutation. |
| `not_found` | `404` | Public resource is not visible or does not exist. |
| `method_not_allowed` | `405` | Route does not support the method. |
| `generation_conflict` | `409` | Backend generation is obsolete. |
| `idempotency_conflict` | `409` | Key was reused with a different normalized request. |
| `operation_conflict` | `409` | Existing operation state prevents the requested transition. |
| `controller_lease_conflict` | `409` | Another valid Legacy OSD controller lease owns control. |
| `revision_conflict` | `412` | Submitted representation revision is stale. |
| `preview_conflict` | `412` | Preview binding is stale, expired or mismatched. |
| `precondition_required` | `428` | Required revision/precondition was omitted. |
| `rate_limited` | `429` | Request exceeds rate or concurrency policy. |
| `capability_unavailable` | `503` | Required capability is unavailable, disabled or degraded beyond use. |
| `backend_unavailable` | `503` | Backend or Agent cannot currently serve the request. |
| `service_unavailable` | `503` | Required Suite component is unavailable. |
| `upstream_error` | `502` | Internal provider returned an invalid or failed response. |
| `upstream_timeout` | `504` | Internal provider timed out and no durable result is available. |
| `internal_error` | `500` | Unexpected server failure. |

`operation_in_progress` and `outcome_unknown` are normally represented as operation states with `200` or `202`, not as reasons to resubmit the mutation.

---

## Required Response Headers

The public API response model must support arbitrary safe HTTP headers.

Common headers include:

| Header | Use |
| --- | --- |
| `Content-Type` | Representation media type. |
| `X-Request-ID` | Final request identity. |
| `X-Correlation-ID` | Accepted workflow correlation identity where present. |
| `ETag` | Opaque resource or representation revision. |
| `Location` | Created resource or accepted operation URL. |
| `Retry-After` | Polling, rate-limit or temporary-unavailability guidance. |
| `Allow` | Supported methods after `405`. |
| `WWW-Authenticate` | Authentication challenge after `401`. |
| `Cache-Control` | Cache policy. |
| `Deprecation` | Marks a deprecated route or representation. |
| `Sunset` | Published date after which a compatibility surface may be removed. |
| `Link` | Successor, documentation or deprecation information where useful. |
| `Vary` | Representation or authorization cache variation. |

The API abstraction must not require controllers to serialize header semantics into JSON fields.

---

## Revision and ETag Contract

ADR-0042 requires an opaque revision for every mutable resource.

The public HTTP mapping is:

```text
resourceRevision
  -> ETag response header

expectedRevision
  -> If-Match request header
```

### Read behavior

A mutable single-resource response returns an `ETag` when its current representation can safely support conditional access.

A client may send:

```text
If-None-Match: "opaque-token"
```

An unchanged representation returns `304` without a JSON body.

### Mutation behavior

A protected update or delete requires:

```text
If-Match: "opaque-token"
```

Missing required precondition returns:

```text
428 precondition_required
```

Stale precondition returns:

```text
412 revision_conflict
```

No backend mutation is dispatched after either rejection.

If both `If-Match` and a body-level `expectedRevision` are temporarily accepted during migration, they must identify the same revision. A mismatch is rejected rather than choosing one silently.

### Strong and weak ETags

- Mutable resource ETags used for `If-Match` must be strong enough for the mutation decision.
- Aggregate, cache or presentation-only responses may use weak ETags when documented.
- Clients treat all ETags as opaque strings.
- Snapshot generation, event sequence and backend generation are not substituted for resource revision unless the endpoint contract explicitly represents that exact resource.

---

## Idempotency and Operation Submission

A production mutation uses ADR-0042 idempotency semantics.

The canonical public header is:

```text
Idempotency-Key
```

Rules:

- the key is required for mutation endpoints classified as retryable submissions;
- the server combines it with actor, backend, resource and action scope;
- same key and same normalized request returns the existing operation or result;
- same key and different normalized request returns `409 idempotency_conflict`;
- key retention survives process restart;
- a request ID is never used as an idempotency key;
- a new retry after a revision conflict uses refreshed state and a new key;
- client libraries preserve the same key across transport retry of the same logical request.

### Accepted operation response

A queued or asynchronous mutation returns `202`, an operation representation and:

```text
Location: /api/v1/operations/{operationId}
```

Example:

```json
{
  "operationId": "op_...",
  "state": "queued",
  "resourceType": "recording",
  "resourceId": "rec_...",
  "backendId": "backend_...",
  "submittedAt": "2026-07-16T17:00:00Z",
  "retryAfterMs": 1000
}
```

The operation representation, not repeated command submission, is the reconciliation surface.

---

## Pagination, Filtering and Ordering

### Cursor-first collections

New v1 collections that can change during traversal use opaque cursor pagination.

Canonical query fields:

```text
limit
cursor
sort
order
```

Domain filters use documented lower-camel-case names such as:

```text
backendId
channelId
from
until
state
query
```

### Cursor rules

A cursor is bound to:

- endpoint and API major;
- actor authorization scope where required;
- normalized filters;
- normalized ordering;
- backend scope;
- relevant snapshot or query consistency context.

Clients must not parse a cursor.

An invalid cursor returns `400 invalid_request`.

An expired or no-longer-reconcilable cursor returns a stable code such as `cursor_expired`, normally with `409` when the original traversal cannot continue consistently.

### Ordering rules

- Default ordering is documented per endpoint.
- Stable pagination requires a deterministic tie-breaker.
- Array order is not a compatibility guarantee unless the endpoint documents it.
- Changing a documented default order within the same API major is a breaking change unless the client requested an explicit order.

### Offset migration

Existing `limit` and `offset` routes may remain as legacy compatibility surfaces.

New federated or high-change v1 collections use cursors. An endpoint may support offset pagination only when the data and performance model make it deterministic and documented.

---

## Multi-Backend Partial Results

VDR-Suite may aggregate data from several authorized backends.

A partial result must never look fully authoritative.

When an endpoint permits partial data and at least one useful source succeeds, it may return `200` with:

```json
{
  "items": [],
  "page": {
    "limit": 50,
    "nextCursor": null,
    "hasMore": false
  },
  "meta": {
    "partial": true,
    "sources": [
      {
        "backendId": "backend_remote",
        "state": "unavailable",
        "code": "backend_unavailable"
      }
    ]
  }
}
```

Rules:

- source status contains no unauthorized backend detail;
- partial success is endpoint-specific and documented;
- a mutation never uses partial target resolution;
- an authoritative single-resource request does not silently substitute another backend resource;
- if no useful source can satisfy the endpoint, return `503` or the more precise error;
- `206 Partial Content` is reserved for HTTP range semantics and is not used as a generic federation warning.

---

## Capability Negotiation

Public API capabilities are separate from backend capabilities.

### Platform capability

Platform capabilities describe whether the current server/API supports features such as:

- cursor pagination;
- conditional mutation;
- durable operations;
- media sessions;
- Legacy OSD viewing;
- specific representation schema versions.

They are discoverable below the v1 contract root or a canonical capability resource.

### Backend capability

Backend capabilities describe whether one backend can currently provide or mutate a domain resource.

They retain:

- backend identity;
- capability ID;
- capability schema version;
- available, degraded, disabled or unsupported state;
- reason and freshness where safe;
- read versus write semantics.

A platform endpoint existing does not mean every backend supports the operation.

### Client behavior

- Clients use contract discovery and capability data rather than User-Agent-specific server behavior.
- The server does not return different undocumented JSON shapes based on client brand.
- A client may report its product and version for diagnostics and compatibility policy, but it does not select hidden contracts.
- Unsupported optional features degrade explicitly.
- Unknown capability IDs are ignored by clients unless they are required for the requested workflow.

---

## JSON Compatibility Rules

### Compatible additive changes within v1

The following are normally compatible:

- adding an optional response field;
- adding a new link relation;
- adding optional metadata;
- adding a new endpoint;
- adding a new optional request field with a safe default;
- adding a new capability ID;
- adding a new error code under an existing appropriate HTTP status when clients already handle unknown codes generically.

### Breaking changes

The following require v2, a separately negotiated representation or a documented compatibility layer:

- removing or renaming a public field;
- changing a field type;
- changing a stable field meaning;
- making an optional request field required;
- changing nullability in a way that invalidates existing clients;
- changing a resource identity format in a way clients must interpret;
- changing a documented default ordering;
- reusing an error code for a different meaning;
- changing a mutation from safe/read-only to state-changing;
- changing an endpoint from synchronous final result to asynchronous operation without compatible representation and documented transition;
- exposing a backend-native identifier where a Suite identity was promised.

### Response openness

Clients must ignore unknown response fields.

Response enums are open unless explicitly marked closed. A client must preserve or display an unknown value safely rather than crash or silently map it to a different known state.

### Null and absence

- Omitted field means unavailable, not selected or not applicable according to the documented resource schema.
- `null` is used only when the schema gives it an explicit meaning.
- Empty string is not used as a generic replacement for missing data.
- A server does not switch between omitted, `null` and empty value arbitrarily within one major version.

### Identity and time representation

- Public identities are opaque strings.
- Clients do not assume numeric IDs or embedded backend meaning.
- New v1 absolute timestamps use UTC date-time strings.
- Legacy epoch fields are normalized deliberately rather than copied into v1 by accident.
- Durations and byte counts declare units in field names or schema documentation.

---

## Caching and Conditional Reads

Public cache behavior is explicit.

- User-specific or permission-filtered responses are not shared-cacheable by default.
- Authentication and authorization variation is reflected in `Cache-Control` and `Vary`.
- Stable assets may use long cache lifetimes and content-addressed identities.
- Mutable domain resources use ETag and conditional requests where practical.
- Live or operational state may use `no-store` or short private caching.
- A cached response never bypasses backend authorization or resource revision checks.

SSE, WebSocket and other live update cursors remain separate from HTTP cache validators.

---

## Deprecation and Compatibility Policy

### Lifecycle

A public route, field or behavior moves through:

```text
supported
-> deprecated
-> sunset announced
-> removed in a compatible major transition or after legacy-policy completion
```

### Rules

- A supported v1 field or route is not removed from v1 without an extraordinary security reason.
- Breaking cleanup normally occurs in v2.
- Deprecation includes a documented successor.
- Deprecated responses may include `Deprecation`, `Sunset` and `Link` metadata.
- The API documentation and changelog record the first deprecated release and successor path.
- The compatibility matrix records which API majors each server release supports.
- A server may support multiple API majors concurrently.
- Clients do not infer removal dates from package version numbers.
- Security-critical endpoints may be disabled sooner, but the server returns a structured error and the reason is documented.

### Legacy unversioned surface

The current `/api/...` surface is a migration layer, not an indefinite major-version promise.

It may remain available while the bundled frontend and documented clients migrate to `/api/v1`.

Legacy support must not:

- weaken authentication or authorization;
- omit read-only enforcement;
- bypass revisions or idempotency;
- expose internal backend paths;
- retry mutations through another route;
- return a more privileged representation than v1.

---

## Client API Wrapper Contract

The bundled Web Client API remains DOM-free and becomes the reference public-client behavior, not a privileged private shortcut.

### Structured client errors

The wrapper exposes a structured error object containing at least:

- HTTP status;
- stable public error code;
- title/detail for display;
- request ID;
- correlation ID where present;
- operation ID where present;
- retryable flag;
- field errors;
- safe metadata.

The wrapper does not reduce every failure to only a message string.

### Retry behavior

Automatic client retry is allowed only when:

- the request is safe and idempotent; or
- the mutation has a durable idempotency key and the retry reuses the same logical request; and
- the server contract classifies the failure as retryable.

The wrapper must not retry:

- validation errors;
- authorization denial;
- revision conflict;
- idempotency conflict;
- unknown mutation outcome by creating a new request;
- arbitrary `5xx` against a fallback mutation path.

### Route behavior

The v1 wrapper uses canonical paths discovered from the supported API contract.

Compatibility aliases may be hidden behind a bounded migration adapter. They are not exposed throughout UI modules.

---

## Security and Information Disclosure

Public errors and representations must not leak:

- stack traces;
- SQL errors or schema names;
- filesystem paths;
- internal hostnames, IP addresses or ports;
- Streamdev URLs;
- Agent transport addresses;
- plugin service pointers;
- raw credentials or authorization headers;
- private OSD frame contents;
- another actor's operation or idempotency result;
- hidden backend existence;
- native VDR pointers or lock state.

Authorization is evaluated before returning current revision, capability, source status or conflict metadata that could reveal a protected resource.

A `404` may intentionally hide whether a forbidden resource exists.

Request and correlation IDs are safe diagnostic handles, not secrets, but they do not grant access to logs or operations.

---

## Boundary with Backend Agent Protocol

The public `/api/v1` contract terminates at the Control Plane.

The Backend Agent protocol has its own:

- handshake;
- protocol version;
- capability schema;
- authenticated machine identity;
- backend generation and lease;
- command/result envelopes;
- reconnect and reconciliation behavior.

The Control Plane maps public requests into domain services and, where required, Agent commands.

Public clients never receive raw Agent frames or use Agent protocol versions as API versions.

An Agent error is translated into the stable public error or operation vocabulary while preserving richer internal evidence for diagnostics and audit.

---

## Boundary with Streaming and Legacy OSD

### Streaming

The public API creates and manages MediaSessions and access grants.

The media bytes themselves use the Streaming Gateway media plane defined by ADR-0046.

A public API route does not return a permanent Streamdev, Agent or plugin endpoint.

### Legacy OSD

The public API creates LegacyOsdSessions, viewer bindings and controller-lease requests.

Sequenced OSD frames/deltas and bounded input transport use the separately defined OSD compatibility plane.

The public API version does not turn raw remote keys, SVDRP or plugin service calls into general client commands.

---

## Boundary with `vdr-plugin-suite-bridge`

The plugin-local contract remains private to the Backend Agent boundary.

The plugin may report:

- local capability schema;
- snapshot schema;
- deterministic native result categories;
- native identity and current-state evidence;
- local command/service contract version.

The plugin does not own:

- `/api/v1` routes;
- public error text or HTTP status;
- user authentication;
- request IDs for public HTTP requests;
- public pagination;
- public ETags;
- public deprecation policy;
- client compatibility matrices.

Public API error codes are mapped in the Control Plane or public API layer. Plugin-native codes remain internal evidence unless a stable public meaning is intentionally defined.

ADR-0048 therefore requires no plugin capability or schema bump merely because this decision is accepted.

---

## Existing Implementation Mapping

The current implementation is retained as foundation and migration evidence.

| Existing component | Role under ADR-0048 |
| --- | --- |
| `ApiRouter` | Current unversioned route inventory and source for v1 migration. |
| Current `/api` aliases | Explicit pre-v1 compatibility candidates, not automatically stable v1 paths. |
| `ApiResponse` | Must grow safe header support and request context mapping. |
| `HttpResponse` / `HttpServerResponse` | Existing lower-level header-capable transport foundation. |
| Controllers and serializers | Domain-specific representations to review and normalize behind v1. |
| `TestHttpServer` Basic Auth | Test/runtime foundation, not final production identity contract. |
| Ad-hoc `{"error":"..."}` bodies | Migration source to the common Problem Details representation. |
| Controllers returning `200` with domain failure fields | Must be classified into immediate HTTP rejection versus durable operation state. |
| `web/frontend/api/client-api.js` | DOM-free reference wrapper to migrate to v1 and structured errors. |
| Current catch-all route fallback | Safe-read migration evidence; prohibited for ambiguous mutation retry. |
| Backend registry/capabilities | Inputs to public backend and platform capability representations. |
| Snapshot and change-feed models | Inputs to ETag, cursor and live-consistency design, not interchangeable revisions. |
| ADR-0042 operation model | Source for preconditions, idempotency and operation representation. |
| ADR-0043 job model | Source for asynchronous state, Retry-After and correlation behavior. |

No existing controller is declared v1-stable solely by being mapped through the new prefix.

Each endpoint requires contract review, representation tests and compatibility documentation.

---

## Implementation Sequence

Phase 68 should implement this decision in bounded slices.

### 1. Public response and request context foundation

- add safe response headers to `ApiResponse` or its successor;
- introduce request context with request ID and accepted correlation ID;
- preserve headers through every HTTP server implementation;
- centralize content-type and no-body handling.

### 2. Common error writer

- introduce one public Problem Details model and serializer;
- add stable code registry;
- map authentication, routing, validation and internal failures;
- remove stack/path/internal-detail leakage;
- add negative contract tests.

### 3. `/api/v1` contract root and route namespace

- implement the contract root;
- expose supported API major and links;
- add canonical v1 route registration;
- keep legacy routes separate from canonical route definitions.

### 4. Read endpoint migration

- migrate backend, channel, event, Recording and capability reads;
- normalize stable identities and field names;
- add standard collections and cursor pagination where required;
- add partial-source metadata;
- add ETag and conditional GET behavior.

### 5. Mutation endpoint migration

- map ADR-0042 mutation envelopes to HTTP;
- require `If-Match` where appropriate;
- require and persist `Idempotency-Key` where appropriate;
- return durable operation resources;
- map immediate rejection to stable non-2xx errors;
- represent unknown outcomes without duplicate dispatch.

### 6. Job, media and OSD control resources

- expose public operation/job state without Agent internals;
- expose MediaSession control resources separately from media bytes;
- expose LegacyOsdSession/controller resources separately from frame transport;
- preserve authorization and audit correlation.

### 7. Client wrapper migration

- add structured client error type;
- move all bundled frontend paths to `/api/v1`;
- remove catch-all mutation fallback;
- preserve request IDs for support and diagnostics;
- add safe retry classification.

### 8. Legacy compatibility layer

- inventory every unversioned route;
- classify alias, replacement, reduced compatibility or removal;
- add deprecation metadata;
- ensure aliases use the same authorization and mutation service;
- document sunset policy.

### 9. Machine-readable specification and compatibility tests

- maintain a versioned machine-readable API description;
- validate examples and schemas in CI;
- run golden response/error tests;
- run backward-compatible schema-diff checks;
- test unknown response fields and enum values;
- test deprecation headers and legacy aliases;
- publish a client/server compatibility matrix.

### 10. Operational acceptance

- test Web frontend against v1 only;
- test at least one independent client harness;
- test local and remote-backend error mapping;
- test read-only denial;
- test ETag conflict and idempotency replay;
- test partial multi-backend reads;
- test no duplicate mutation after timeout or invalid response;
- prove rollback to the previous runtime package.

---

## Migration Rules for Existing Controllers

Migration does not require one destructive rewrite.

For each route:

1. identify the underlying domain service;
2. classify read, synchronous command, durable operation or live transport;
3. identify stable Suite resources and revisions;
4. define canonical v1 request and response schema;
5. define authorization and capability behavior;
6. map immediate failures to stable HTTP errors;
7. add request/correlation IDs and headers;
8. add contract tests;
9. connect the bundled Client API to the canonical route;
10. retain or deprecate the old alias deliberately.

Controllers remain thin HTTP-facing adapters. Core services do not depend on `api/rest`.

---

## Rejected Alternatives

### Keep unversioned `/api` indefinitely

Rejected because accidental route and field behavior would become an undocumented compatibility promise.

### Put a version only in a custom request header

Rejected as the sole mechanism because links, documentation, proxies, logs and simultaneous major-version support become less explicit.

### Version every small additive field change

Rejected because a new major for every optional field would make evolution impractical. Compatibility rules define safe additive changes.

### Wrap every success in `{ "data": ... }`

Rejected because it adds little value, obscures natural resource and operation representations and is not required for consistent errors or metadata.

### Continue plain `{"error":"message"}` responses

Rejected because clients cannot distinguish stable causes, preserve request IDs, render field errors or make safe retry decisions.

### Return `200` for every handled request

Rejected because immediate validation, authorization and precondition failures require standard HTTP semantics.

### Retry every failed request automatically

Rejected because a timed-out or invalid mutation response may already have changed VDR state.

### Let clients probe all known route aliases

Rejected because it hides compatibility state and can duplicate or misclassify mutations.

### Expose Agent or plugin protocol directly to clients

Rejected because those are trusted internal machine boundaries with different identity, lifecycle and compatibility requirements.

### Use server package version as API compatibility

Rejected because software release cadence and public contract evolution are different concerns.

### Make response messages stable machine identifiers

Rejected because user-facing wording must be improvable and localizable without breaking clients.

---

## Consequences

Positive:

- Web, desktop, mobile and TV clients share one deliberate contract;
- internal backend and plugin quirks stay behind adapters;
- clients can make safe conflict and retry decisions;
- mutations cannot be duplicated by speculative fallback;
- request and correlation IDs improve diagnosis across Control Plane, Agent and jobs;
- ETags connect HTTP concurrency to ADR-0042 revisions;
- common errors make UI and SDK behavior consistent;
- cursor collections support changing and federated data safely;
- partial backend failure becomes visible rather than silently incomplete;
- legacy routes can be migrated without pretending they are permanent;
- plugin, Agent, media and OSD protocol versions remain independently evolvable;
- compatibility can be tested in CI rather than inferred after release.

Trade-offs:

- existing controllers and serializers require explicit review;
- header propagation must be added to the current API response abstraction;
- the bundled frontend must migrate away from generic `Error` and broad fallback behavior;
- supporting more than one API major increases maintenance cost;
- strict request parsing requires explicit extension points;
- cursor pagination is more complex than offset pagination;
- stable public error codes require governance;
- deprecation and schema documentation become ongoing release work;
- current test-only authentication and route behavior cannot be treated as final production contract.

---

## Non-Goals

This ADR does not:

- implement Phase 68;
- migrate every current endpoint;
- define the final endpoint list for all future domains;
- implement production authentication or RBAC;
- define the ADR-0049 audit-event schema;
- define the Agent wire protocol;
- define media byte transport;
- define OSD frame encoding or transport;
- create mobile, desktop or TV SDKs;
- require GraphQL, gRPC or another additional public protocol;
- promise a public C++ ABI;
- mark G-03, G-06, G-25 or G-26 as implemented;
- make existing unversioned routes stable by declaration;
- enable any new VDR or plugin mutation.

---

## Acceptance Criteria

ADR-0048 is implemented only when:

- `/api/v1` is the canonical documented public API namespace;
- the contract root reports the public API major and discoverable links;
- public API, Agent protocol, media plane, OSD plane and plugin schema versions are distinct;
- `ApiResponse` or its replacement supports safe arbitrary response headers;
- every response carries a server-controlled request ID;
- errors use the common Problem Details-compatible representation;
- stable public error codes are documented and tested;
- authentication, authorization, validation, capability and conflict errors use consistent statuses;
- `ETag`, `If-Match`, `If-None-Match` and `428` behavior are implemented for selected revisioned resources;
- production mutation submissions use persistent idempotency and durable operation identity;
- ambiguous mutation outcomes never trigger speculative path fallback or a fresh mutation automatically;
- asynchronous submissions return `202`, an operation representation and `Location`;
- v1 collections use the standard collection envelope;
- changing/federated collections use deterministic opaque cursors where required;
- partial multi-backend responses are explicitly marked and source-scoped safely;
- platform and backend capabilities are distinct and versioned;
- response clients tolerate unknown fields and enum values;
- request objects reject unknown fields unless an extension point is documented;
- deprecation and sunset metadata are supported;
- every retained unversioned route is inventoried and classified;
- the bundled Web Client API uses structured errors and canonical v1 paths;
- no mutating client fallback retries another path after an ambiguous result;
- a machine-readable specification, examples and compatibility tests run in CI;
- security tests prove that paths, credentials, Agent endpoints and unauthorized resource details do not leak;
- read-only, revision-conflict, idempotency-replay, backend-unavailable and partial-read cases are covered;
- controlled runtime acceptance and rollback pass.

Acceptance of this ADR is not runtime completion.

---

## Related Decisions

- [ADR-0003: REST API as External Interface](ADR-0003-rest-api.md)
- [ADR-0007: RESTfulAPI Adapter Boundary](ADR-0007-restfulapi-adapter-boundary.md)
- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0018: Incremental Snapshot Synchronization](ADR-0018-incremental-snapshot-synchronization.md)
- [ADR-0019: SSE Event Stream Transport Strategy](ADR-0019-sse-event-stream-transport-strategy.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](ADR-0037-packaging-install-api-boundary.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0045: Canonical EPG Event Identity and Provenance](ADR-0045-canonical-epg-event-identity-provenance.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](ADR-0047-legacy-osd-compatibility-bridge.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
