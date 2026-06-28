# Phase 56.27 - Service API Developer Boundary Guide

## Navigation

- [Development Index](index.md)
- [Phase 56.25 - Public and Internal API Boundaries](phase-56.25-public-internal-api-boundaries.md)
- [Phase 56.26 - REST API Developer Boundary Guide](phase-56.26-rest-api-developer-boundary-guide.md)

---

## Status

Completed.

---

## Purpose

Phase 56.27 defines how VDR-Suite service APIs should be treated during library boundary, packaging and developer documentation work.

Service APIs are the preferred reusable boundary between REST controllers, daemon wiring, tools and core domain behavior.

This phase documents how service APIs should be classified, reviewed and promoted to public library candidates.

---

## Service API Role

A service API owns use-case behavior.

It sits between external entry points and low-level domain/storage/backend code:

```text
REST controller / daemon / tool / test
  -> service API
  -> domain model / repository / adapter / executor
```

Service APIs should isolate callers from implementation details such as repositories, backend payload formats, cache refresh details and mutation verification steps.

---

## Service Boundary Classes

| Class | Meaning |
| --- | --- |
| Public service candidate | Stable or intended-stable service useful to application and external library users. |
| Internal service | Shared inside VDR-Suite but not yet suitable as public package surface. |
| Daemon coordination service | Runtime lifecycle or polling ownership; daemon-adjacent by design. |
| Backend integration service | Concrete backend adapter or executor coordination. |
| Tooling service | Helper workflow for smoke tests or operator validation. |
| Test support service | Mock/fake/test-only workflow support. |

---

## Public Service Candidates

Public service candidates should be dependency-light, deterministic and usable without daemon runtime wiring.

| Area | Candidate services |
| --- | --- |
| Recording read workflows | recording query/list/detail services |
| Recording metadata | metadata lookup/update planning and provider-neutral services |
| Dashboard read model | read-only dashboard facade when dependency boundaries are stable |
| VDR overview | backend-neutral overview/status/channel/event/timer read services |
| VDR domain adapters | adapter interface and backend registry service contracts |
| EPG search | read-only EPG query and result services |
| SearchTimer preview | dry-run, preview and validation services |
| Runtime diagnostics | runtime diagnostics read services |
| Snapshot read | snapshot read services once cache ownership is documented |
| Metadata/person catalog | provider-neutral metadata/person lookup services |

---

## Internal Services

Internal services may be shared across REST, daemon and tools but are not public package surface yet.

| Area | Internal services | Reason |
| --- | --- | --- |
| Recording mutation | delete/move execution services | destructive-operation policy and permissions still need packaging docs |
| SearchTimer mutation | create/update/delete execution and readback services | backend verification and safety gates are runtime-sensitive |
| Timer mutation | timer action execution services | backend verification and permission policy required |
| Polling/change detection | polling and refresh coordination services | daemon lifecycle ownership |
| Snapshot mutation/update | snapshot cache update services | cache lifecycle ownership |
| Native fuzzy operator refresh | operator refresh/restore services | admin/tooling workflow |

---

## Service API Rules

A service API should:

- expose typed request/result models
- keep side effects explicit
- return deterministic results for read-only operations
- isolate REST controllers from repositories and backend payload details
- isolate tools from daemon wiring when possible
- keep backend-specific details behind adapter interfaces
- document whether it is read-only, planning-only, dry-run or mutating

A service API should not:

- depend on REST controllers
- parse HTTP request bodies
- own daemon startup composition
- directly print CLI output
- mix test-only mocks into production service surfaces
- hide destructive operations behind read-looking method names

---

## Request and Result Model Policy

Service request and result models are often stronger public candidates than service implementations.

They should be:

- plain C++ value objects where possible
- serializable without daemon state
- independent from HTTP request/response classes
- independent from CLI formatting
- precise about success, failure and verification status

Mutation result models should include enough information for REST, CLI and UI callers to explain what happened.

---

## Read-Only Service Policy

Read-only services are preferred public API candidates.

They may:

- query repositories
- query backend adapters
- read snapshots
- compose overview results
- run validation or preview calculations
- return diagnostics

They must not:

- mutate backend state
- schedule daemon work implicitly
- enable production mutation gates
- write persistent state unless explicitly documented as a metadata write/update service

---

## Mutation Service Policy

Mutation services are allowed, but they require stronger boundary documentation.

Every mutation service must document:

- mutation type
- required runtime policy or permission gate
- preview/dry-run support
- execution path
- backend verification/readback behavior
- failure result shape
- safe default behavior

Mutation service names should make mutation explicit.

Good examples:

```text
executeDelete
executeMove
createTimer
updateSearchTimer
verifyBackendReadback
```

Avoid ambiguous names such as:

```text
process
sync
refreshAll
apply
```

unless the side effects are documented at the API boundary.

---

## REST Usage Rule

REST controllers should depend on services, not on repositories or backend executors directly.

Preferred pattern:

```text
Controller -> Parser -> Service -> Result -> Serializer
```

The service owns use-case behavior. The controller owns HTTP translation.

---

## Daemon Usage Rule

Daemon code may compose services and own lifecycle.

Daemon code should not be required for a public service candidate unless the service is explicitly daemon-only.

Preferred pattern:

```text
Daemon runtime -> construct services -> inject into REST/router/tools
```

Not preferred:

```text
Service -> daemon singleton/global runtime
```

---

## Tooling Usage Rule

Tools may call service APIs to validate real backends or operator workflows.

Tooling should stay outside public production library surfaces unless explicitly promoted as a developer utility package.

---

## Packaging Rule

Package public services by use-case and dependency boundary, not by historical source directory alone.

Initial service package candidates:

```text
vdr-suite-recording-service
vdr-suite-vdr-service
vdr-suite-epg-service
vdr-suite-searchtimer-readonly-service
vdr-suite-metadata-service
vdr-suite-runtime-diagnostics-service
```

Do not package daemon coordination or destructive mutation execution as default public service APIs until safety and permission documentation is complete.

---

## Service Review Checklist

Before promoting or documenting a service as public API, verify:

1. The service has typed request/result models.
2. The service does not depend on REST controllers.
3. The service does not require daemon startup state unless explicitly daemon-only.
4. Read-only and mutating behavior are clearly separated.
5. Backend-specific details are hidden behind interfaces/adapters.
6. Test support is not included in the production surface.
7. Error and verification results are caller-visible.
8. The service can be used by REST, CLI or future UI without duplicating core logic.
9. Public headers do not drag in application-only dependencies.
10. Examples can be written without booting the daemon unless the service is daemon-only.

---

## Follow-up

Later Phase 56 work should add:

- service API inventory
- public header candidate list
- service usage examples
- backend adapter authoring examples
- mutation safety and permission guide
- packaging map from service API to Debian/library package

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required because this phase documents service boundaries only.

---

## Back

- [Back to Development Index](index.md)
