# Phase 56.29 - Packaging Boundary Guide

## Navigation

- [Development Index](index.md)
- [Phase 56.20 - Library, Daemon and API Boundary Audit](phase-56.20-library-daemon-api-boundary-audit.md)
- [Phase 56.21 - Build Boundary Map](phase-56.21-build-boundary-map.md)
- [Phase 56.25 - Public and Internal API Boundaries](phase-56.25-public-internal-api-boundaries.md)
- [Phase 56.28 - Backend Adapter Developer Boundary Guide](phase-56.28-backend-adapter-developer-boundary-guide.md)

---

## Status

Completed.

---

## Purpose

Phase 56.29 defines initial packaging boundaries for VDR-Suite.

The goal is not to create Debian packages yet. The goal is to document which code surfaces may become reusable libraries, which surfaces are application-only and which surfaces must stay test/tooling-only.

This guide uses the library, REST, service and backend boundary decisions from Phase 56.20 through Phase 56.28.

---

## Packaging Principles

Packages should follow dependency boundaries, not historical source layout alone.

Preferred principles:

1. Public libraries must be dependency-light.
2. Public libraries must not depend on daemon composition.
3. REST contracts and REST implementation are different packaging surfaces.
4. Read-only APIs are safer first public packages than mutation APIs.
5. Mutation execution needs explicit safety and permission packaging docs.
6. Test support must not be pulled into production runtime packages.
7. Tooling packages must be separate from reusable runtime libraries.
8. Multi-site backend identity must be representable without global daemon state.

---

## Initial Package Classes

| Class | Meaning |
| --- | --- |
| Public library package | Reusable C++ API and implementation intended for external consumers. |
| API contract package | Documentation/schema/client-facing contract without daemon internals. |
| Application package | Daemon, process entry points and runtime composition. |
| Backend adapter package | Concrete backend integration implementation. |
| Tooling package | Operator helpers, smoke checks or local validation tools. |
| Test support package | Mock/fake/local test server utilities. |
| Internal-only module | Not packaged as public surface yet. |

---

## Candidate Public Library Packages

| Package | Candidate contents | Notes |
| --- | --- | --- |
| `vdr-suite-runtime-core` | diagnostics values and read services | Small, reusable runtime metadata surface. |
| `vdr-suite-http-client` | HTTP request/response/interface and basic client | Client only; excludes listener and test server. |
| `vdr-suite-recording-core` | recording domain, repositories and read services | Public read/query candidate. |
| `vdr-suite-metadata-core` | metadata and provider-neutral models/services | Public after provider examples are written. |
| `vdr-suite-vdr-domain` | VDR status/channel/event/timer/recording domain objects | Backend-neutral. |
| `vdr-suite-vdr-adapter-api` | adapter interfaces and capability models | Extension point for backend authors. |
| `vdr-suite-vdr-service` | backend-neutral read services | Public candidate when headers are audited. |
| `vdr-suite-epg-core` | EPG query/match/result models and read services | Read-only API candidate. |
| `vdr-suite-searchtimer-readonly` | preview, validation and dry-run models/services | Safe SearchTimer public surface first. |
| `vdr-suite-snapshot-read` | snapshot read models/services | Public after cache ownership docs. |

---

## API Contract Packages

API contract packages describe client-facing interfaces without exposing server implementation as public C++ API.

| Package | Contents |
| --- | --- |
| `vdr-suite-rest-api-contracts` | endpoint inventory, request/response schemas, examples |
| `vdr-suite-client-examples` | minimal HTTP/client usage examples |
| `vdr-suite-developer-docs` | developer guides, boundary docs and adapter guides |

REST controllers, request parsers and daemon route wiring are not automatically part of the contract package.

---

## Application Packages

Application packages own runtime composition.

| Package | Contents |
| --- | --- |
| `vdr-suite-daemon` | daemon startup, runtime wiring, HTTP listener composition |
| `vdr-suite-api-server` | REST router/controller runtime composition if split from daemon later |
| `vdr-suite-config` | deployment configuration templates and examples |

Application packages may depend on public libraries, internal modules and backend adapters.

Public libraries must not depend on application packages.

---

## Backend Adapter Packages

Backend adapter packages connect VDR-Suite to concrete backends.

| Package | Contents |
| --- | --- |
| `vdr-suite-vdr-restfulapi-adapter` | RESTfulAPI concrete adapter, mappers and HTTP client integration |
| `vdr-suite-vdr-mock-adapter` | mock adapter only if packaged as developer/test support |
| `vdr-suite-vdr-svdrp-adapter` | future SVDRP adapter |
| `vdr-suite-backend-registry` | registry/capability composition if promoted from core |

Concrete backend adapters should depend on adapter API packages, domain packages and transport/client packages, not daemon packages.

---

## Tooling Packages

Tooling packages support maintainers and operators.

| Package | Contents |
| --- | --- |
| `vdr-suite-tools` | non-destructive local validation helpers |
| `vdr-suite-smoke-tools` | real-backend smoke harnesses |
| `vdr-suite-operator-tools` | future explicit operator workflows |

Destructive tooling must be clearly labelled and must not be installed as a default dependency of the daemon or public libraries.

---

## Test Support Packages

Test support packages are optional developer utilities.

| Package | Contents |
| --- | --- |
| `vdr-suite-test-support-http` | `MockHttpClient`, `TestHttpServer` |
| `vdr-suite-test-support-vdr` | mock/fake VDR adapters and fixtures |
| `vdr-suite-test-support-live` | live transport fakes and regression helpers |

Production packages should not depend on test support packages.

---

## Internal-Only for Now

These areas must remain internal until later phases explicitly promote them:

- daemon runtime composition
- production mutation execution gates
- recording destructive action executors
- SearchTimer create/update/delete execution services
- timer mutation executors
- backend polling coordinator
- snapshot cache mutation/update lifecycle
- smoke harness internals
- local real-backend destructive test helpers
- test servers and mocks as production dependencies

---

## Dependency Direction

Preferred package dependency direction:

```text
application package
  -> REST implementation
  -> service packages
  -> domain / adapter API packages
  -> utility packages
```

Backend adapter direction:

```text
concrete backend adapter
  -> adapter API
  -> VDR domain
  -> HTTP client or backend transport
```

Forbidden package dependency direction:

```text
public domain package -> daemon package
public service package -> REST controller package
public HTTP client package -> HTTP listener/test server package
production package -> test support package
adapter API package -> concrete backend package
```

---

## Header and Source Audit Requirements

Before creating real packages, each candidate package needs a header/source audit.

For every package candidate, document:

- public headers
- private headers
- source files
- direct dependencies
- transitive dependencies
- test-only dependencies
- daemon/application dependencies
- compile targets proving isolation
- examples that use the public surface

No public package should be created before this audit is complete.

---

## Debian Packaging Implication

When Debian packaging is added, prefer a conservative split:

```text
vdr-suite-daemon
vdr-suite-tools
vdr-suite-dev
vdr-suite-doc
vdr-suite-test-support
```

Then split finer library packages only after stable public headers and dependency audits exist.

Early Debian packages should avoid pretending unstable internal APIs are public ABI.

---

## Versioning and Stability Policy

Initial package docs should label APIs as one of:

| Stability | Meaning |
| --- | --- |
| experimental | May change without compatibility promise. |
| candidate | Intended public API, still under audit. |
| stable | Compatibility promise exists. |
| internal | No external compatibility promise. |
| test-support | Developer utility only. |

Phase 56 package candidates are mostly `candidate` or `internal`, not `stable`.

---

## Packaging Review Checklist

Before promoting a package candidate, verify:

1. Public headers are identified.
2. Internal headers are excluded.
3. Daemon dependencies are absent unless the package is application-only.
4. REST implementation is separated from REST contract docs.
5. Test support is not a production dependency.
6. Mutation APIs are clearly labelled and gated.
7. Multi-site backend identity is not blocked by global state.
8. Examples compile or are documented as pseudocode until compile examples exist.
9. The package has an explicit stability label.
10. Documentation states whether the package is public, internal, tooling or test support.

---

## Follow-up

Later Phase 56 work should add:

- concrete header/source inventory per package candidate
- package dependency graph
- developer package naming decision
- Debian packaging skeleton plan
- sample build/install layout
- public API example programs

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required because this phase documents packaging boundaries only.

---

## Back

- [Back to Development Index](index.md)
