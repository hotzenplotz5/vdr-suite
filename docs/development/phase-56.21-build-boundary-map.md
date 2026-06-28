# Phase 56.21 - Build Boundary Map

## Navigation

- [Development Index](index.md)
- [Phase 56.20 - Library, Daemon and API Boundary Audit](phase-56.20-library-daemon-api-boundary-audit.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Status

Completed.

---

## Purpose

This document maps the existing GNU Make source groups to their intended Phase 56 library, API, daemon, tool and test boundaries.

It does not move code. It defines the target classification that later Makefile or source-layout changes must preserve.

---

## Boundary Classes

| Class | Meaning |
| --- | --- |
| Core library | Reusable domain/service source group with no daemon ownership. |
| API integration | REST or transport-facing controller/parser/router source group. |
| Daemon application | Runtime composition and process lifecycle source group. |
| Tooling / smoke support | Operator helper, smoke harness or real-backend validation support. |
| Test support | Mock, fake, test server or dedicated test-only implementation. |
| Needs cleanup | Current source group crosses a boundary and should be split before public packaging. |

---

## Current Source Group Map

| Source group | File | Current classification | Phase 56 target classification | Notes |
| --- | --- | --- | --- | --- |
| `RECORDINGS_SRC` | `mk/recording-sources.mk` | Core library | Core library | Recording repository, metadata repository and recording service. |
| `METADATA_SRC` | `mk/recording-sources.mk` | Core library | Core library | Metadata repository and metadata service. |
| `JOB_DASHBOARD_SRC` | `mk/recording-sources.mk` | Core library | Core library | Job repository and dashboard service. |
| `RECORDING_DASHBOARD_SRC` | `mk/recording-sources.mk` | Core library | Core library | Recording dashboard read model. |
| `DASHBOARD_FACADE_SRC` | `mk/recording-sources.mk` | Core library | Core library | Aggregates dashboard-facing recording and job services. |
| `DASHBOARD_JSON_SRC` | `mk/recording-sources.mk` | Core library | Core library | Serialization helper; acceptable as library surface when contract-stable. |
| `DASHBOARD_CLI_SRC` | `mk/recording-sources.mk` | Tool/application mix | Tooling / smoke support | Should remain a CLI composition group, not a library boundary. |
| `ACTION_SERVICE_SRC` | `mk/action-job-sources.mk` | Core library | Core library | Recording action service with utility dependency. |
| `JOB_SERVICE_SRC` | `mk/action-job-sources.mk` | Core library | Core library | Job service with utility dependency. |
| `JOB_REPOSITORY_SRC` | `mk/action-job-sources.mk` | Core library | Core library | Repository plus service grouping for tests. |
| `WORKFLOW_SRC` | `mk/action-job-sources.mk` | Core library | Core library | Recording workflow service boundary. |
| `WORKER_SRC` | `mk/action-job-sources.mk` | Tool/test support | Tooling / smoke support | Worker simulator is not public library surface. |
| `RECTOOLS_ADAPTER_SRC` | `mk/action-job-sources.mk` | Adapter support | Core library candidate | Candidate only after external tool boundary is documented. |
| `ACTIONS_SRC` | `mk/action-job-sources.mk` | Mixed core/API/VDR | Needs cleanup | Contains REST request parser and VDR SearchTimer cache source. Split before public packaging. |
| `VDR_SRC` | `mk/vdr-sources.mk` | Broad core domain | Needs subdivision | Too broad for one public library. Split documentation before build refactor. |
| `REST_DASHBOARD_SRC` | `mk/rest-sources.mk` | API integration | API integration | REST dashboard controller plus required core dependencies. |
| `REST_VDR_SRC` | `mk/rest-sources.mk` | API integration | API integration | VDR REST controllers. |
| `REST_EPG_SRC` | `mk/rest-sources.mk` | API integration | API integration | EPG REST controller and query parameter parsing. |
| `REST_RUNTIME_SRC` | `mk/rest-sources.mk` | API integration | API integration | Runtime diagnostics controller. |
| `REST_SNAPSHOT_CHANGE_FEED_SRC` | `mk/rest-sources.mk` | API integration | API integration | Snapshot change-feed controller. |
| `REST_LIVE_TRANSPORT_SRC` | `mk/rest-sources.mk` | API integration | API integration | Live transport controller. |
| `REST_ROUTER_SRC` | `mk/rest-sources.mk` | Full API composition | API integration / needs cleanup | Large router group composes many core dependencies. It is API-facing, not a core library. |
| `RUNTIME_SRC` | `mk/runtime-sources.mk` | Core library | Core library | Compact runtime logging and diagnostics source group. |
| `HTTP_SRC` | `mk/http-sources.mk` | Test/core mix | Test support / core client split | Mock HTTP client belongs to tests; public client interface should be explicit. |
| `HTTP_SERVER_SRC` | `mk/http-sources.mk` | HTTP client/server mix | Needs cleanup | Basic client, listener and test server should become explicit client/server/test groups. |
| `DAEMON_SRC` | `mk/daemon-sources.mk` | Daemon composition | Daemon application | Application assembly target, not a library. |

---

## Required Boundary Splits

### Recording action core versus REST parsing

`ACTIONS_SRC` must not represent a reusable recording action library while it includes REST request parsing from `api/rest`.

Target split:

- `RECORDING_ACTION_CORE_SRC`: core recording action utils, validation service and result serializers
- `RECORDING_ACTION_REST_SRC`: REST request parser and controllers
- `RECORDING_ACTION_TEST_SUPPORT_SRC`: any test-only glue

### Recording action core versus VDR SearchTimer cache

`ACTIONS_SRC` currently includes a SearchTimer preview cache implementation from `core/vdr`.

Target split:

- keep recording action source groups independent from SearchTimer cache
- move the cache dependency into the specific test or API group that needs it
- document the reason if the dependency is intentional

### VDR source group subdivision

`VDR_SRC` is the most important Phase 56 split candidate.

Target sub-boundaries:

| Target group | Intended content |
| --- | --- |
| `VDR_DOMAIN_SRC` | Domain objects, query models, result models and backend-neutral contracts. |
| `VDR_BACKEND_SRC` | Backend registry, adapter interfaces, adapter factory and backend capability services. |
| `VDR_RESTFULAPI_ADAPTER_SRC` | RESTfulAPI mappers, RESTfulAPI adapters and command executors. |
| `VDR_SEARCHTIMER_SRC` | SearchTimer domain, validation, planning, preview, workflow and readback services. |
| `VDR_EPG_SRC` | EPG query and EPGSearch services and matchers. |
| `VDR_SNAPSHOT_SRC` | Snapshot cache, snapshot builder, read services and change-feed services. |
| `VDR_LIVE_SRC` | Live transport events, serializers, factory and SSE transport. |
| `VDR_RUNTIME_COORDINATION_SRC` | Polling, refresh planning and backend polling coordinator. |

No code should move until these groups are validated against current tests.

### HTTP client/server/test split

Target groups:

| Target group | Intended content |
| --- | --- |
| `HTTP_CLIENT_SRC` | Basic HTTP client and reusable request/response implementation if needed. |
| `HTTP_TEST_SUPPORT_SRC` | Mock HTTP client and test HTTP server. |
| `HTTP_SERVER_SRC` | Simple HTTP listener used by daemon or harness applications. |

---

## Public Packaging Candidates

The first packaging layer should expose only stable, dependency-light surfaces:

1. Runtime diagnostics core
2. HTTP client interfaces and basic client
3. Recording repositories/services and metadata services
4. Backend-neutral VDR domain and service interfaces
5. REST API contracts only after request/response examples are documented

Do not package daemon composition, real-backend destructive executors or smoke helpers as public library surface.

---

## Documentation Deliverables After This Map

Phase 56 should still produce:

- public library boundary documentation
- REST API developer guide
- service API developer guide
- build and packaging guide
- example snippets for backend adapters, recording services, SearchTimer services and metadata/provider boundaries

---

## Verification

Required local verification after this documentation-only map:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
