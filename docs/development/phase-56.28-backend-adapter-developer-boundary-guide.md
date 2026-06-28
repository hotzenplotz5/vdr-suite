# Phase 56.28 - Backend Adapter Developer Boundary Guide

## Navigation

- [Development Index](index.md)
- [Phase 56.22 - VDR Source Boundary Plan](phase-56.22-vdr-source-boundary-plan.md)
- [Phase 56.25 - Public and Internal API Boundaries](phase-56.25-public-internal-api-boundaries.md)
- [Phase 56.27 - Service API Developer Boundary Guide](phase-56.27-service-api-developer-boundary-guide.md)

---

## Status

Completed.

---

## Purpose

Phase 56.28 defines the developer boundary for VDR backend adapters.

Backend adapters are the extension point for connecting VDR-Suite services to concrete data sources such as RESTfulAPI, mock backends, future SVDRP support or multi-site backend providers.

This phase documents which adapter surfaces are public candidates and which details must remain backend-internal, daemon-internal or test-only.

---

## Adapter Role

A backend adapter translates backend-specific protocols and payloads into VDR-Suite domain and service contracts.

Preferred direction:

```text
VDR service
  -> adapter interface
  -> concrete backend adapter
  -> transport/client/backend protocol
```

The adapter owns protocol translation. It must not own REST controller behavior, daemon lifecycle or user-interface decisions.

---

## Public Adapter Surface Candidates

| Surface | Public candidate? | Notes |
| --- | --- | --- |
| backend-neutral VDR domain objects | yes | status, channel, event, timer, recording models |
| adapter interface | yes | extension point for new backend providers |
| backend capability model | yes | allows UI/API to reason about read/write/support features |
| backend registry contract | yes, after dependency audit | needed for multi-backend and multi-site composition |
| read-only adapter operations | yes | safer first public adapter surface |
| mutation adapter operations | not yet by default | requires permission, policy and verification docs |
| concrete RESTfulAPI adapter internals | internal | protocol-specific implementation |
| mock/test adapters | test support unless explicitly promoted | developer utilities only |

---

## Adapter Boundary Classes

| Boundary | Meaning |
| --- | --- |
| Adapter interface | Backend-neutral contract used by VDR services. |
| Concrete adapter | Backend-specific implementation of the interface. |
| Mapper | Converts backend payload fields into domain objects. |
| Transport | HTTP/SVDRP/local process transport used by a concrete adapter. |
| Capability provider | Reports supported operations and policy constraints. |
| Test adapter | Mock/fake backend implementation for tests and examples. |

---

## Dependency Direction Rules

Allowed:

```text
VDR service -> adapter interface
concrete adapter -> mapper
concrete adapter -> transport/client
mapper -> domain object
```

Forbidden:

```text
adapter interface -> REST controller
adapter interface -> daemon runtime
mapper -> HTTP listener
transport -> REST controller
concrete adapter -> UI/frontend code
core domain -> concrete adapter
```

The core domain must stay backend-neutral.

---

## Concrete Backend Adapter Policy

A concrete backend adapter may:

- call a backend protocol
- map backend payloads into domain objects
- expose backend capability information
- normalize backend errors into typed results
- perform backend-specific field fallback where documented

A concrete backend adapter should not:

- parse VDR-Suite REST API requests
- return backend-native JSON to callers
- depend on daemon startup wiring
- directly decide frontend permissions
- silently execute destructive operations without service-level policy

---

## Mapper Policy

Mappers are protocol adapters, not business services.

They may:

- translate field names
- normalize optional fields
- preserve backend IDs
- apply documented compatibility fallback
- reject malformed backend payloads

They should not:

- query repositories
- call other backends
- schedule daemon refreshes
- execute mutations
- encode REST response policy

Mapper behavior must be deterministic and side-effect free.

---

## Capability Policy

Every backend adapter should make capabilities explicit.

Capabilities should distinguish:

- read status
- read channels
- read events
- read timers
- read recordings
- create/update/delete timers
- create/update/delete SearchTimers
- recording move/delete support
- snapshot support
- live/change event support
- read-only mode
- permission-controlled mutations

Capability reporting must not imply execution permission. A backend may support a feature while the current runtime policy still disables it.

---

## Read-Only Adapter Policy

Read-only adapter operations are preferred public candidates.

They may:

- fetch status
- fetch channels
- fetch events
- fetch timers
- fetch recordings
- fetch SearchTimer previews or lists when backend supports them
- fetch capabilities

They must not:

- write backend state
- refresh daemon-owned caches implicitly unless documented
- bypass service-level permissions

---

## Mutation Adapter Policy

Mutation operations require service-level policy before they become public.

Mutation adapter methods should be reached through guarded services, not directly from REST controllers or tools.

Required documentation:

- operation name
- backend capability requirement
- runtime permission requirement
- request model
- result model
- backend readback/verification rule
- failure mapping
- idempotency expectations

---

## Multi-Site Implication

Backend adapters must prepare for multiple configured backend instances.

Adapter APIs should avoid hidden global state.

Preferred:

```text
BackendId + AdapterConfig + AdapterInstance
```

Not preferred:

```text
global default VDR backend singleton
```

Multi-site policy must support:

- named backend instances
- read-only remote backends
- per-backend capability reporting
- per-backend permission policy
- future frontend selection of backend scope

---

## Test Adapter Policy

Mock and fake adapters are important developer utilities, but they are not production backend packages by default.

They may be documented as:

```text
vdr-suite-backend-test-support
```

only if clearly separated from production packages.

Test adapters must not be required by production runtime packages.

---

## Adapter Developer Checklist

Before adding or promoting a backend adapter, verify:

1. The adapter implements a backend-neutral interface.
2. Domain objects stay backend-neutral.
3. Protocol mapping is isolated in mapper code.
4. Transport is isolated from service and domain logic.
5. Capabilities are explicit and tested.
6. Read-only and mutating operations are separated.
7. Mutation methods require service-level safety policy.
8. No REST controller code is required by the adapter.
9. No daemon lifecycle singleton is required by the adapter interface.
10. Multi-site backend identity can be represented without global state.

---

## Packaging Rule

Initial backend-related package candidates:

```text
vdr-suite-vdr-domain
vdr-suite-vdr-adapter-api
vdr-suite-vdr-backend-registry
vdr-suite-vdr-restfulapi-adapter
vdr-suite-vdr-backend-test-support
```

Production packages should not depend on test support packages.

Concrete backend adapter packages should depend on adapter API and transport/client packages, not on daemon application packages.

---

## Follow-up

Later Phase 56 work should add:

- backend adapter inventory
- public adapter header candidate list
- RESTfulAPI adapter usage example
- mock adapter example for tests
- multi-site backend configuration guide
- permission and read-only backend guide

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required because this phase documents adapter boundaries only.

---

## Back

- [Back to Development Index](index.md)
