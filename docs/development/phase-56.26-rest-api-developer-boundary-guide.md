# Phase 56.26 - REST API Developer Boundary Guide

## Navigation

- [Development Index](index.md)
- [Phase 56.20 - Library, Daemon and API Boundary Audit](phase-56.20-library-daemon-api-boundary-audit.md)
- [Phase 56.25 - Public and Internal API Boundaries](phase-56.25-public-internal-api-boundaries.md)

---

## Status

Completed.

---

## Purpose

Phase 56.26 defines how developers should think about REST API code in VDR-Suite.

The REST API is a public HTTP contract, but its C++ controllers, request parsers and daemon wiring are not automatically public library APIs.

This phase documents the boundary rules for adding, reviewing and packaging REST API features.

---

## REST Layer Ownership

REST API code owns:

- HTTP route registration
- request parsing
- HTTP status mapping
- response body serialization for endpoint contracts
- controller-level input validation
- API-specific error messages
- translation between HTTP request data and service calls

REST API code must not own:

- core domain rules
- backend-specific business logic
- daemon lifecycle decisions
- persistent storage schema ownership
- destructive action safety policy
- polling or scheduling lifecycle

---

## Dependency Direction

Allowed direction:

```text
REST controller/request parser
  -> service boundary
  -> core/domain model
  -> storage/backend adapter boundary
```

Forbidden direction:

```text
core/domain model
  -> REST controller

core/service
  -> HTTP request parser

backend adapter
  -> REST controller

daemon runtime
  -> REST request parser as business logic owner
```

REST may call core services. Core services must not call REST.

---

## Controller Boundary

A REST controller should be thin.

It may:

- inspect HTTP method and path-owned parameters
- parse request body into a request object
- call a service
- serialize the service result
- map expected service outcomes to HTTP status codes

It should not:

- perform backend mutation directly
- duplicate service rules
- know daemon startup state beyond injected runtime services
- own cross-backend permission decisions
- directly read or write database tables unless the feature is explicitly a REST-only diagnostic endpoint

---

## Request Parser Boundary

A REST request parser converts HTTP request data into typed request objects.

It may:

- decode JSON or form bodies
- validate required request fields
- normalize API-specific input syntax
- reject malformed HTTP/API payloads

It should not:

- execute domain workflows
- query VDR backends
- query repositories
- mutate runtime state
- depend on daemon composition

Parser output should be service-ready, but not service-executed.

---

## Serializer Boundary

Serializers may format domain/service results as API JSON.

They may:

- expose stable endpoint fields
- preserve compatibility aliases when required
- hide internal implementation details
- include diagnostics that belong to the endpoint contract

They should not:

- trigger refreshes
- query backends
- apply mutation decisions
- modify service results

Serialization must stay deterministic and side-effect free.

---

## Service Boundary Used by REST

REST endpoints should call service APIs instead of composing domain behavior directly.

Preferred pattern:

```text
Controller -> RequestParser -> Service -> Result -> Serializer
```

Examples:

| Feature | REST should call |
| --- | --- |
| dashboard overview | dashboard facade/service |
| recording list/detail | recording service/repository boundary |
| recording action validation | validation service |
| recording action execution | guarded execution service |
| VDR overview | VDR overview service |
| timer actions | timer action service/execution service |
| EPG search | EPG search service |
| SearchTimer preview | preview/workflow validation service |
| runtime diagnostics | runtime diagnostics service |
| snapshot read | snapshot read service |

---

## Public HTTP Contract vs Public C++ API

The REST endpoint is public as an HTTP contract when documented and tested.

The C++ controller implementation remains an internal API unless explicitly promoted.

| Surface | Public? | Notes |
| --- | --- | --- |
| HTTP route and response schema | yes, when documented | client-facing contract |
| request parser class | no by default | implementation detail |
| controller class | no by default | API layer implementation |
| JSON serializer class | maybe | public only if used by non-REST clients |
| service result model | candidate | reusable core/service API |
| daemon route wiring | no | application composition |

---

## REST Feature Review Checklist

Before adding or changing a REST endpoint, verify:

1. The endpoint has a clear service boundary.
2. Core logic is not implemented inside the controller.
3. Request parsing is deterministic and side-effect free.
4. Serialization is deterministic and side-effect free.
5. Mutation endpoints use explicit safety and permission gates.
6. Backend-specific behavior is behind adapter/service boundaries.
7. The route is covered by controller/router tests.
8. Documentation states whether the endpoint is read-only or mutating.
9. HTTP status mapping is intentional and stable.
10. The endpoint does not expose daemon-only internals accidentally.

---

## Read-Only Endpoint Policy

Read-only endpoints are preferred public API candidates.

They may expose:

- status
- overview
- recordings
- metadata
- VDR channels/events/timers
- EPG search results
- SearchTimer previews and dry-run information
- runtime diagnostics
- snapshot read state

Read-only endpoints should not require mutation gates.

---

## Mutation Endpoint Policy

Mutation endpoints require stronger boundaries.

They must route through guarded services for:

- recording delete
- recording move
- timer create/update/delete
- SearchTimer create/update/delete
- persisted runtime changes
- backend operator refresh

Mutation endpoints must document:

- whether the endpoint is enabled by default
- what permission or runtime policy is required
- whether the operation is preview-only, dry-run or executing
- what verification/readback is performed
- what failure result is returned when backend verification fails

---

## REST Packaging Rule

REST API contracts may be documented as public developer surface.

REST implementation classes should remain in API-layer packages unless a later phase explicitly promotes a reusable component.

Initial package split should treat REST as:

```text
vdr-suite-rest-api-contracts     public docs/client contract
vdr-suite-rest-implementation    internal/application API layer
vdr-suite-daemon                 application wiring and listener runtime
```

---

## Example Boundary Template

For every future endpoint document, prefer this structure:

```text
Endpoint
Method and path
Read-only or mutating
Request body
Response body
Service dependency
Safety/permission rule
Error mapping
Tests
Internal dependencies
```

---

## Follow-up

Later Phase 56 work should add:

- REST API endpoint inventory
- REST API usage examples
- safe mutation endpoint guide
- daemon deployment route guide
- client compatibility notes

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required because this phase documents API boundaries only.

---

## Back

- [Back to Development Index](index.md)
