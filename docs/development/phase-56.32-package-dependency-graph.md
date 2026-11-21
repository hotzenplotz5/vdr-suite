# Phase 56.32 - Package Dependency Graph

## Navigation

- [Development Index](index.md)
- [Phase 56.29 - Packaging Boundary Guide](phase-56.29-packaging-boundary-guide.md)
- [Phase 56.30 - Package Candidate Source Inventory](phase-56.30-package-candidate-source-inventory.md)
- [Phase 56.31 - Public Header Inventory](phase-56.31-public-header-inventory.md)

---

## Status

Completed.

---

## Purpose

Phase 56.32 defines the intended dependency graph between the package candidates identified in Phase 56.29 through Phase 56.31.

The goal is to prevent package candidates from becoming arbitrary source bundles. A package may only become public when its dependency direction is clear and it does not pull daemon, REST implementation, tooling or test-support code into public library surfaces.

This phase does not change build files and does not create packages.

---

## Dependency Labels

| Label | Meaning |
| --- | --- |
| `public-core` | Public library candidate with low-level dependencies. |
| `public-service` | Public service package candidate built on public core packages. |
| `concrete-adapter` | Backend-specific implementation package. |
| `rest-contract` | Public HTTP client-facing contract documentation/schema package. |
| `rest-implementation` | Internal/application REST controller/parser package. |
| `application` | Daemon or API server composition package. |
| `tooling` | Operator or smoke-test helper package. |
| `test-support` | Mock/fake/test server package. |
| `internal` | Not a public package surface yet. |

---

## Top-Level Dependency Direction

Allowed top-level direction:

```text
application
  -> rest-implementation
  -> public-service / internal-service
  -> concrete-adapter
  -> adapter-api / domain / core utility
```

Public library direction:

```text
public-service
  -> public-core
  -> domain / utility
```

Concrete adapter direction:

```text
concrete-adapter
  -> adapter-api
  -> domain
  -> transport/client
```

Forbidden direction:

```text
public-core -> public-service
public-core -> rest-implementation
public-core -> daemon/application
public-service -> REST controller/parser
adapter-api -> concrete-adapter
production package -> test-support
REST contract docs -> daemon runtime
```

---

## Public Core Layer

Public core candidates are the base layer.

```text
vdr-suite-runtime-core
vdr-suite-http-client
vdr-suite-recording-core
vdr-suite-metadata-core
vdr-suite-vdr-domain
vdr-suite-vdr-adapter-api
```

Dependency expectations:

| Package | May depend on | Must not depend on |
| --- | --- | --- |
| `vdr-suite-runtime-core` | standard library only or very small utilities | daemon, REST, VDR adapters |
| `vdr-suite-http-client` | runtime core only if needed | listener, test server, daemon, REST controllers |
| `vdr-suite-recording-core` | runtime core, persistence utility if split | REST parsers, daemon, VDR cache |
| `vdr-suite-metadata-core` | recording persistence or shared metadata utility | daemon, REST controllers |
| `vdr-suite-vdr-domain` | standard library only | concrete adapters, REST, daemon |
| `vdr-suite-vdr-adapter-api` | VDR domain, capability value objects | concrete adapters, daemon, REST controllers |

---

## Public Service Layer

Public service candidates should depend on public core packages only.

```text
vdr-suite-vdr-service
vdr-suite-epg-core
vdr-suite-searchtimer-readonly
vdr-suite-snapshot-read
vdr-suite-dashboard-readmodel
vdr-suite-recording-action-validation
```

Dependency expectations:

| Package | May depend on | Must not depend on |
| --- | --- | --- |
| `vdr-suite-vdr-service` | VDR domain, adapter API, runtime core | REST controllers, daemon runtime |
| `vdr-suite-epg-core` | VDR domain, VDR service, runtime core | SearchTimer mutation, daemon |
| `vdr-suite-searchtimer-readonly` | VDR domain, EPG core, runtime core | create/update/delete execution, production policy gates |
| `vdr-suite-snapshot-read` | VDR domain, runtime core | snapshot cache mutation lifecycle unless package is internal |
| `vdr-suite-dashboard-readmodel` | recording core, metadata core, runtime core | daemon composition, REST controllers |
| `vdr-suite-recording-action-validation` | recording core, runtime core | REST request parsers, destructive execution, VDR cache |

---

## Concrete Adapter Layer

Concrete adapter packages implement backend-specific behavior.

```text
vdr-suite-vdr-restfulapi-adapter
vdr-suite-vdr-svdrp-adapter
vdr-suite-vdr-external-adapter
```

Dependency expectations:

| Package | May depend on | Must not depend on |
| --- | --- | --- |
| `vdr-suite-vdr-restfulapi-adapter` | VDR domain, adapter API, HTTP client | daemon runtime, REST API controllers |
| `vdr-suite-vdr-svdrp-adapter` | VDR domain, adapter API, transport utility | daemon runtime, REST API controllers |
| `vdr-suite-vdr-external-adapter` | VDR domain, adapter API, process utility | REST controllers, daemon app |

Notes:

- Concrete adapters may be production packages.
- Mock adapters belong to test-support unless explicitly promoted as examples.
- Backend factories and registries need a separate audit because they can become composition points.

---

## REST Contract and REST Implementation Layer

REST contract and implementation are separate package classes.

```text
vdr-suite-rest-api-contracts
vdr-suite-rest-implementation
```

Dependency expectations:

| Package | May depend on | Must not depend on |
| --- | --- | --- |
| `vdr-suite-rest-api-contracts` | docs, schemas, examples | daemon runtime, C++ controller implementation |
| `vdr-suite-rest-implementation` | public services, internal services, serializers | daemon app entry point, test support by default |

Notes:

- REST contracts are public client-facing HTTP surface.
- REST controller and parser C++ headers are not public core-library headers by default.
- `REST_ROUTER_SRC` remains `split-required` because it combines controllers and service dependencies.

---

## Application Layer

Application packages compose the runtime.

```text
vdr-suite-daemon
vdr-suite-api-server
vdr-suite-config
```

Dependency expectations:

| Package | May depend on | Must not depend on |
| --- | --- | --- |
| `vdr-suite-daemon` | all required public/internal services, REST implementation, concrete adapters, listener | test support as production dependency |
| `vdr-suite-api-server` | REST implementation, listener, service packages | test support as production dependency |
| `vdr-suite-config` | docs/templates | compiled library internals |

Notes:

- Application packages are allowed to know broad composition.
- No public library package may depend on an application package.

---

## Tooling and Test-Support Layer

Tooling and test-support packages are terminal dependencies, not production foundations.

```text
vdr-suite-tools
vdr-suite-smoke-tools
vdr-suite-test-support-http
vdr-suite-test-support-vdr
vdr-suite-test-support-live
```

Dependency expectations:

| Package | May depend on | Must not depend on |
| --- | --- | --- |
| `vdr-suite-tools` | public libraries, concrete adapters | daemon internals unless tool is application-specific |
| `vdr-suite-smoke-tools` | public libraries, concrete adapters, selected internal helpers | default production daemon dependency |
| `vdr-suite-test-support-http` | HTTP client interfaces | production public client package as reverse dependency |
| `vdr-suite-test-support-vdr` | VDR domain and adapter API | production adapter packages by default |
| `vdr-suite-test-support-live` | live transport interfaces | production live transport package by default |

---

## Split Order

The safe split order is:

1. Keep `runtime-core` isolated.
2. Keep `http-client` isolated from listener and test support.
3. Split recording core from recording action REST parser and VDR cache dependency.
4. Split VDR domain and adapter API from concrete adapters.
5. Split VDR read services from mutation services and daemon polling.
6. Split EPG and SearchTimer read-only services from SearchTimer mutation execution.
7. Split REST contracts from REST implementation.
8. Split daemon application composition from REST implementation and test support.
9. Split tooling and smoke helpers from production package dependencies.
10. Add examples only after package dependencies are stable enough to explain.

---

## Immediate Dependency Blockers

| Blocker | Impact |
| --- | --- |
| `ACTIONS_SRC` includes REST parser and VDR cache | Blocks clean recording-action validation package. |
| `VDR_SRC` is one broad aggregate | Blocks precise VDR domain/service/adapter package mapping. |
| `REST_ROUTER_SRC` mixes controllers and service dependencies | Blocks clean REST implementation package definition. |
| `DAEMON_SRC` includes `TestHttpServer.cpp` | Blocks production daemon package hygiene. |
| SearchTimer read-only and mutation files share broad VDR aggregate | Blocks safe SearchTimer readonly package. |
| Header manifests are not package-specific yet | Blocks public `-dev` package creation. |

---

## Target Graph

```text
vdr-suite-daemon
  -> vdr-suite-rest-implementation
  -> vdr-suite-vdr-service
  -> vdr-suite-vdr-restfulapi-adapter
  -> vdr-suite-vdr-adapter-api
  -> vdr-suite-vdr-domain

vdr-suite-rest-implementation
  -> vdr-suite-recording-core
  -> vdr-suite-metadata-core
  -> vdr-suite-epg-core
  -> vdr-suite-searchtimer-readonly
  -> vdr-suite-snapshot-read
  -> vdr-suite-runtime-core

vdr-suite-vdr-restfulapi-adapter
  -> vdr-suite-http-client
  -> vdr-suite-vdr-adapter-api
  -> vdr-suite-vdr-domain

vdr-suite-test-support-*
  -> public core / adapter API only
```

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required because this phase documents package dependency direction only.

---

## Back

- [Back to Development Index](index.md)
