# Phase 56.25 - Public and Internal API Boundaries

## Navigation

- [Development Index](index.md)
- [Phase 56.20 - Library, Daemon and API Boundary Audit](phase-56.20-library-daemon-api-boundary-audit.md)
- [Phase 56.21 - Build Boundary Map](phase-56.21-build-boundary-map.md)
- [Phase 56.22 - VDR Source Boundary Plan](phase-56.22-vdr-source-boundary-plan.md)
- [Phase 56.23 - HTTP Boundary Plan](phase-56.23-http-boundary-plan.md)
- [Phase 56.24 - HTTP Source Group Split](phase-56.24-http-source-group-split.md)

---

## Status

Completed.

---

## Purpose

Phase 56.25 defines which APIs are intended to become public library surfaces and which APIs must remain internal to the daemon, REST layer, tooling or tests.

This phase does not change code. It documents the API ownership model that later packaging and developer documentation must follow.

---

## Boundary Classes

| Boundary | Meaning |
| --- | --- |
| Public library API | Stable or intended-stable C++ surface suitable for reuse outside the daemon. |
| Internal core API | Shared inside the repository, but not promised as external package surface. |
| REST API surface | HTTP endpoint contract and request/response serialization layer. |
| Daemon-only API | Runtime composition, process lifecycle, polling and operational wiring. |
| Tooling API | Operator helper, smoke harness or command-line workflow support. |
| Test/support API | Mock, fake, local server or regression-only support code. |

---

## Public Library API Candidates

These surfaces are candidates for public documentation and packaging once dependency and header expectations are finalized.

| Area | Candidate public API | Notes |
| --- | --- | --- |
| Runtime diagnostics | runtime log and diagnostics value objects/services | Small dependency footprint and useful for application integration. |
| HTTP client | request/response/interface headers and `BasicHttpClient` | Public only as client surface, not server/test support. |
| Recording core | recording repository/service models and metadata services | Public for read/query and metadata workflows. |
| Recording actions | validation and result models | Execution remains guarded until mutation policy and permissions are finalized. |
| VDR domain | status, channel, event, timer, recording and backend-neutral models | Public only for backend-neutral domain and service contracts. |
| VDR backend | adapter interfaces, backend registry and capability models | Public candidate for external adapter authors. |
| VDR snapshot read | snapshot read models, serializers and read services | Public candidate when cache ownership is documented. |
| EPG search | EPG query models, matchers and result serializers | Public candidate for read-only discovery and preview flows. |
| SearchTimer read/preview | preview, validation and dry-run planning models | Public candidate for safe read-only planning. |
| Metadata/person/catalog | content classification, people and metadata provider boundaries | Public candidate after provider contract examples are written. |

---

## Internal Core APIs

Internal core APIs may be shared by modules in this repository but should not be promised as stable package surface yet.

| Area | Internal API | Reason |
| --- | --- | --- |
| Recording action execution | real delete/move execution services and backend executor registries | Mutation-sensitive and permission-dependent. |
| SearchTimer mutation | create/update/delete execution, readback verification and policy gates | Requires explicit runtime safety and permission story. |
| Backend polling | polling service, change detection and refresh planner | Runtime coordination is daemon-adjacent. |
| Snapshot cache mutation | snapshot builder/cache update services | Cache lifecycle belongs to runtime ownership. |
| RESTfulAPI concrete executors | backend-specific mutation executors | Backend integration detail, not generic API. |
| Native fuzzy operator refresh | operator refresh and persisted restore helpers | Operational/admin workflow, not public library surface yet. |

---

## REST API Surface

REST API surface is public from the perspective of HTTP clients, but it is not the same thing as a reusable C++ library API.

| REST area | Boundary |
| --- | --- |
| Dashboard REST controllers | API surface over dashboard read models. |
| Recordings REST controllers | API surface over recording services and action workflows. |
| Metadata REST controllers | API surface over metadata services. |
| VDR REST controllers | API surface over VDR overview/status/timer/event/channel services. |
| EPG REST controllers | API surface over EPG query and search services. |
| SearchTimer REST controllers | API surface over preview, workflow validation and guarded execution. |
| Runtime diagnostics REST controllers | API surface over runtime state. |
| Snapshot/change-feed REST controllers | API surface over snapshot read and change-feed services. |
| Live transport REST/SSE controllers | API surface over live transport events. |

Rule:

```text
REST request parsers and controllers belong to api/rest.
Core library APIs must not depend on REST controllers.
```

---

## Daemon-Only APIs

Daemon-only APIs are application composition and runtime lifecycle code. They should not be packaged as public libraries.

| Area | Daemon-only ownership |
| --- | --- |
| daemon application startup | process entry and application wiring |
| runtime configuration loading | final runtime composition and environment binding |
| backend polling coordinator wiring | scheduling and lifecycle ownership |
| HTTP listener composition | daemon/API server ownership |
| production mutation enablement | runtime safety policy and operator-controlled gates |

Rule:

```text
The daemon may depend on public and internal APIs.
Public library APIs must not depend on daemon composition.
```

---

## Tooling APIs

Tooling APIs support local validation, real-backend smoke tests and operator checks.

| Tooling area | Classification |
| --- | --- |
| real-backend read-only regression helpers | tooling |
| SearchTimer yaVDR API smoke harness | tooling / local API harness |
| RESTfulAPI connectivity smoke | tooling |
| recording action real delete/move smoke helpers | tooling with destructive-operation caution |
| native fuzzy validation helpers | tooling / operator validation |

Tooling code may use public and internal APIs, but public APIs must not depend on tooling.

---

## Test and Support APIs

Test/support APIs are useful for maintainers and examples but must be labelled as such.

| Support area | Classification |
| --- | --- |
| `MockHttpClient` | test support |
| `TestHttpServer` | test support |
| `MockVdrAdapter` when used as a test double | test support |
| `TestLiveTransport` | test support |
| local regression fakes | test support |

Test/support APIs may be packaged later as developer-test utilities, but not as production runtime libraries.

---

## Packaging Implications

Initial package documentation should prefer small, dependency-light surfaces:

1. `vdr-suite-runtime-core`
2. `vdr-suite-http-client`
3. `vdr-suite-recording-core`
4. `vdr-suite-vdr-domain`
5. `vdr-suite-vdr-backend`
6. `vdr-suite-epg-core`
7. `vdr-suite-searchtimer-readonly`
8. `vdr-suite-rest-api-contracts`

Do not expose daemon composition, destructive real-backend executors, smoke helpers or test servers as production library packages.

---

## Developer Documentation Implications

Phase 56 documentation should eventually include:

- public C++ library API guide
- REST API usage guide
- backend adapter authoring guide
- metadata provider authoring guide
- SearchTimer read-only planning examples
- safe mutation policy and permission guide
- daemon integration and deployment guide
- test utility guide

---

## Decision Summary

| Surface | Public package? | Notes |
| --- | --- | --- |
| domain models | yes, after header audit | public library candidate |
| read-only services | yes, after dependency audit | public library candidate |
| REST controllers | no as C++ library; yes as HTTP contract | API surface, not core library |
| REST request parsers | no | API implementation detail |
| daemon wiring | no | application-only |
| mutation executors | not yet | requires permission and safety docs |
| polling coordinator | not yet | daemon-adjacent runtime behavior |
| mock/test server | no production package | test/developer support only |
| smoke helpers | no | tooling only |

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required because this phase documents API ownership only.

---

## Back

- [Back to Development Index](index.md)
