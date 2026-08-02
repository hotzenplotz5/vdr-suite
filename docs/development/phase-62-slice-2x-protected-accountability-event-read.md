# Phase 62 Slice 2X — Protected Accountability Event Read

Status: **selected and documented; implementation has not started**

## Selection result

The fresh post-Slice-2W gap analysis selects exactly one next bounded slice:

```text
Phase 62 Slice 2X
Protected Accountability Event Read
```

Slice 2X adds one authenticated, centrally authorized and strictly bounded
server-owned read path for the newest append-only accountability events. It does
not implement the broader audit product, generic security administration,
operation replay, Outbox delivery or credential lifecycle.

## Why this is the next smallest coherent slice

The accepted runtime already writes append-only, secret-free accountability
records, but production code has no protected operator read path. The only
existing read helper is the unbounded repository `listAll()` used by focused
tests. Exposing that helper directly would create both an authorization gap and
an unbounded database/memory boundary.

The other remaining candidates are larger or dependency-bound:

- generic mutation outcomes cannot be made unambiguously fail closed after
  domain dispatch without stronger transaction coupling or Outbox semantics;
- common revisions, idempotency and durable operation lifecycle cross several
  unrelated mutable resources and route owners;
- actor, identity, credential, grant and role administration requires multiple
  dangerous mutation contracts rather than one read-only boundary;
- native/service credential lifecycle depends on explicit enrollment and
  administration ownership and would pull Phase-63-facing concerns forward;
- audit export, configurable redaction and audit retention are separate products
  with storage, privacy and operational-policy implications;
- compatibility retirement is blocked until the remaining Phase-62 contracts are
  complete.

A bounded accountability read is independently useful, consumes the accepted
identity/authorization/accountability foundation, changes no VDR domain state
and can withhold all data if either authorization or audit-of-audit persistence
fails.

## Problem and security invariant

Problem:

- accountability evidence exists only inside SQLite;
- no Suite-owned protected HTTP path can inspect it;
- ordinary GET handling is not sufficient for sensitive security evidence;
- the current test helper is unbounded and cannot become a product API.

Security invariant:

> No accountability row leaves the server unless the request has an active
> resolved identity with the exact global `security.audit.read@*` permission,
> the query is within the fixed bound, and both the authorization decision and
> the successful read outcome are durably recorded. Any authentication,
> authorization, query, serialization-contract or accountability failure returns
> no accountability data.

## In scope

### Exact HTTP surface

```text
GET /api/security/accountability/events
GET /api/security/accountability/events?limit=<1..100>
```

Contract:

- default limit: `50`;
- minimum limit: `1`;
- maximum limit: `100`;
- deterministic newest-first order by `recorded_at DESC, event_id DESC`;
- duplicate `limit`, non-decimal values, zero, values above 100 and unknown query
  parameters fail with HTTP 400 before repository access;
- the response uses `Cache-Control: no-store`;
- browser-session GET access does not require CSRF because the route is
  read-only;
- the route is handled by the Security Runtime before the general `ApiRouter`.

The response is one bounded snapshot. Slice 2X deliberately does not add cursor
pagination or arbitrary history traversal.

### Exact authorization surface

```text
permission=security.audit.read
backend_id=*
action=security.audit.read
```

Rules:

- anonymous requests fail with HTTP 401;
- invalid, expired or revoked identity state fails with the existing exact
  lifecycle error;
- authenticated requests without the exact permission fail with HTTP 403;
- a direct exact `security.audit.read@*` grant permits the read;
- fixed `role.admin@*` explicitly includes `security.audit.read`;
- `role.admin` on any non-`*` backend does not grant the global read;
- `role.read-only` does not grant the read;
- Legacy Basic compatibility does not bypass this sensitive-read authorization;
- Managed Basic and browser sessions use their existing resolved grants and do
  not inherit rights from another credential.

### Fixed response projection

The serializer may emit only the existing accountability fields plus the stored
recording timestamp needed to describe ordering:

```text
eventId
schemaVersion
classes
eventType
severity
occurredAt
recordedAt
actorId
actorType
deviceId
sessionId
authenticationState
permission
backendId
operationId
requestId
correlationId
action
decision
reasonCode
outcome
```

The response never includes HTTP headers, request or response bodies, cookies,
Authorization values, passwords, verifier hashes, session secrets, CSRF tokens,
raw configuration or process environments.

## Explicit exclusions

Slice 2X does not add:

- audit export, download or streaming;
- cursor pagination, offsets, arbitrary time ranges, search or filters;
- configurable redaction or field-selection policy;
- audit retention, deletion, compaction or archival;
- frontend audit viewer, navigation or client-side persistence;
- actor, identity, session, credential, grant or role administration;
- generic mutation outcomes or post-dispatch instrumentation;
- transactional Outbox or cross-domain transaction coupling;
- revisions, `If-Match`, idempotency keys or durable operation replay;
- native/service credential enrollment, rotation or revocation;
- compatibility retirement;
- Android, Android TV or Phase 63–67 runtime.

The fixed allowlist above is a response-safety contract, not the configurable
redaction product deferred beyond Slice 2X.

## Data model and migration boundary

Slice 2X keeps `accountability_events` append-only.

Allowed additive changes:

- expose `recorded_at` through the repository result model;
- add an idempotent index matching the bounded query order:

```text
(recorded_at DESC, event_id DESC)
```

No existing accountability row is rewritten or deleted. No new lifecycle,
operation, export or retention table is introduced.

The existing test-only `listAll()` may remain for focused repository tests, but
production HTTP/service code must use a new bounded query method that can
distinguish:

```text
successful empty result
successful bounded result
repository unavailable/failure
```

## Service, repository, HTTP and frontend boundary

### Repository

`AccountabilityEventRepository` owns all SQLite schema and SELECT statements for
the bounded query. No public header, HTTP owner or serializer may call
`sqlite3_*` directly.

### Service

A dedicated accountability-read service owns:

- the fixed bound;
- the bounded repository call;
- one atomic read/outcome transaction;
- exact success/failure result typing;
- rollback and fail-closed behavior.

It does not own authentication, permission parsing or JSON formatting.

### Security gate

`SecurityHttpGate` owns exact route classification, authentication and the
`security.audit.read@*` authorization decision. The route must not fall through
to anonymous ordinary-GET behavior or the Legacy Basic compatibility bypass.

### HTTP service/server

The HTTP owner parses the exact `limit` grammar, invokes the service and
serializes only the fixed field allowlist. The general `ApiRouter` does not own
or proxy the security database query.

### Frontend

No frontend files change in Slice 2X. A future audit viewer must consume this
server-owned contract and may not broaden it.

## Fail-closed behavior

No accountability data is returned when any of the following occurs:

- Security Runtime unavailable;
- authentication or persistent lifecycle failure;
- permission-grant persistence unavailable;
- missing or wrong exact permission/scope;
- malformed query parameters;
- accountability repository prepare, bind, step or finalize failure;
- success-outcome append failure;
- transaction commit failure;
- response projection detects a field outside the fixed allowlist.

Security and repository availability failures return HTTP 503 with no event
array. Authorization and validation errors retain the existing stable 401, 403
or 400 error families.

## Accountability contract

The existing pre-dispatch authorization event remains mandatory:

```text
event_type=authorization.allowed|authorization.denied
permission=security.audit.read
backend_id=*
action=security.audit.read
outcome=dispatch_authorized|dispatch_denied
```

For an allowed query, the bounded SELECT and one read outcome are coupled in one
`BEGIN IMMEDIATE` transaction. The success event is:

```text
event_type=operation.succeeded
classes=audit,security
severity=info
permission=security.audit.read
backend_id=*
action=security.audit.read
decision=completed
reason_code=accountability_events_returned
outcome=returned
```

The outcome carries the resolved actor/device/session/request/correlation
context and contains no query response body or event contents.

If the SELECT fails, the read transaction is rolled back and no data is
returned. A separate exact failure event may be appended only after rollback:

```text
event_type=operation.failed
classes=audit,security
severity=error
permission=security.audit.read
backend_id=*
action=security.audit.read
decision=failed
reason_code=accountability_query_failed
outcome=failed
```

Failure to persist either mandatory authorization or success accountability
keeps the route fail closed. The returned snapshot does not include its own
success outcome because that outcome is appended after the bounded rows are
selected inside the same transaction.

## Transaction boundary

The route has two explicit accountability boundaries:

1. the existing pre-dispatch authorization append completes before the query;
2. the bounded SELECT and mandatory `operation.succeeded` append execute in one
   `BEGIN IMMEDIATE` transaction and commit together.

Any SELECT, append or commit failure rolls back the read/outcome transaction and
returns no rows. This coupling is local to the read-only security operation and
does not claim a generic Outbox or cross-domain mutation solution.

## Configuration surface

Slice 2X adds no environment variable, daemon default or runtime configuration.
The default and maximum limits are compile-time contract constants covered by
source and architecture tests.

## Unit and integration tests

Focused tests must cover at least:

### Repository

- additive/idempotent schema and ordered-index creation;
- deterministic newest-first ordering with equal `recorded_at` tie-broken by
  `event_id`;
- limits 1, 50 and 100;
- successful empty result distinct from unavailable persistence;
- no unbounded production query path;
- append-only update/delete triggers remain effective.

### Authorization

- anonymous denial;
- invalid/revoked lifecycle denial;
- direct exact `security.audit.read@*` allow;
- missing permission denial;
- wrong backend scope denial;
- `role.admin@*` allow;
- non-global `role.admin` denial;
- `role.read-only` does not grant;
- Legacy Basic compatibility cannot bypass the exact permission.

### Service and transaction

- bounded read plus exact success event commit atomically;
- forced success-event append failure rolls back and returns no rows;
- forced SELECT failure returns unavailable and no rows;
- exact actor, request and correlation context on the success event;
- no response data appears in outcome accountability.

### HTTP

- exact route and method ownership;
- default and explicit valid limit;
- malformed, duplicate, zero, over-limit and unknown parameters return 400;
- newest-first JSON projection and fixed field set;
- `Cache-Control: no-store` and request/correlation response headers;
- browser GET succeeds without CSRF only when authorization succeeds;
- grant-store, query-store and accountability failure return 503 with no events;
- route never reaches the general `ApiRouter`.

## Architecture guard

A dedicated Slice-2X guard must fail when:

- the exact path is not classified as a sensitive protected GET;
- production code calls `AccountabilityEventRepository::listAll()`;
- accountability SELECT SQL appears outside the approved repository
  implementation unit;
- the maximum limit exceeds 100 or can be supplied unbounded;
- `role.admin` gains `security.audit.read` for a non-global backend scope;
- Legacy Basic compatibility can bypass exact authorization;
- the route is registered in the general business `ApiRouter`;
- frontend code, export code, retention code or generic administration is added
  to the slice;
- the serializer references forbidden secret-bearing names or request bodies.

The guard is wired into the existing architecture/test graph and is itself a
fingerprint that can justify rerunning its focused validation.

## Packaging and installation impact

Expected packaging impact is limited to registering new daemon/security source
and focused test files in the existing Make fragments.

Slice 2X requires no:

- new Debian package;
- new systemd unit or override;
- new Nginx location;
- new configuration file or environment default;
- frontend asset change;
- database maintenance command.

The existing `/api/` reverse-proxy path carries the new endpoint.

## Real-runtime acceptance boundary

Runtime acceptance is required only after implementation and a fully green
five-job source CI on the implementation head. It must use a guarded,
rollback-safe isolated security database and prove:

- anonymous request returns 401;
- authenticated identity without the grant returns 403;
- wrong-scope grant returns 403;
- exact direct grant returns 200;
- exact `role.admin@*` returns 200;
- `limit=2` returns exactly the two newest deterministic rows;
- malformed and over-limit requests return 400;
- the response is no-store and contains only the fixed secret-free projection;
- exact authorization and read-success accountability are present;
- forced outcome-accountability failure returns 503 and exposes no rows;
- forced query failure returns 503 and exposes no rows;
- SQLite quick and foreign-key checks pass;
- production database, daemon configuration and deferred loader remain
  unchanged;
- temporary grants, rows and systemd overrides are removed;
- final daemon is active;
- zero VDR domain mutations occur.

No Slice-2W runtime scenario is repeated unless one of its own accepted
fingerprints changes.

## Runtime repetition fingerprints

A later Slice-2X runtime acceptance is repeated only when at least one of these
changes materially:

- production daemon or Slice-2X source implementation;
- accountability event model, schema, ordered index or bounded SELECT SQL;
- read service transaction or failure handling;
- SecurityHttpGate protected-GET classification, permission or fixed-role
  mapping;
- HTTP limit parser or response serializer;
- accountability success/failure event contract;
- architecture guard or focused tests;
- daemon source/build/packaging registration;
- Slice-2X runtime-acceptance harness;
- systemd execution path used by the acceptance.

Documentation-only changes, a new chat or unchanged Slice-2W fingerprints do not
justify repeating runtime acceptance.

## Selection gate

No production implementation may begin until this contract and the canonical
status, gap and handoff documents are mutually consistent and all five GitHub
Actions jobs pass on the final selection head:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`;
- `packaging-regression-test`.

After that green selection gate, the next action is only the bounded Slice-2X
implementation described here. PR #117 remains open, Draft and unmerged.