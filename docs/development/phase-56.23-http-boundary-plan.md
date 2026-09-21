# Phase 56.23 - HTTP Boundary Plan

## Navigation

- [Development Index](index.md)
- [Phase 56.20 - Library, Daemon and API Boundary Audit](phase-56.20-library-daemon-api-boundary-audit.md)
- [Phase 56.21 - Build Boundary Map](phase-56.21-build-boundary-map.md)
- [Phase 56.22 - VDR Source Boundary Plan](phase-56.22-vdr-source-boundary-plan.md)

---

## Status

Completed.

---

## Purpose

The current HTTP source grouping mixes reusable client behavior, daemon/server listener behavior and test support.

This plan defines the target HTTP source boundaries before changing `mk/http-sources.mk` or package surfaces.

---

## Current State

The current `mk/http-sources.mk` source groups are:

```make
HTTP_SRC := \
        core/http/src/MockHttpClient.cpp

HTTP_SERVER_SRC := \
        core/http/src/BasicHttpClient.cpp \
        core/http/src/SimpleHttpListener.cpp \
        core/http/src/TestHttpServer.cpp
```

This is functional for the current tests and daemon build, but the group names do not express the real boundary intent:

- `MockHttpClient` is test support, not production HTTP library code.
- `BasicHttpClient` is reusable client implementation.
- `SimpleHttpListener` is server/listener runtime infrastructure.
- `TestHttpServer` is test infrastructure and must not become a public server library by accident.

---

## Target HTTP Source Groups

| Target group | Classification | Intended content |
| --- | --- | --- |
| `HTTP_CLIENT_SRC` | Core library | Production HTTP client implementation and reusable request/response support. |
| `HTTP_SERVER_SRC` | Runtime integration | Production/simple listener used by daemon and API harnesses. |
| `HTTP_TEST_SUPPORT_SRC` | Test support | Mock HTTP client, test HTTP server and future fake transport helpers. |
| `HTTP_SRC` | Transitional aggregate | Optional compatibility aggregate during refactor only. |

---

## Proposed File Classification

| Source file | Target group | Reason |
| --- | --- | --- |
| `core/http/src/BasicHttpClient.cpp` | `HTTP_CLIENT_SRC` | Production HTTP client used by RESTfulAPI adapters and real-backend helpers. |
| `core/http/src/SimpleHttpListener.cpp` | `HTTP_SERVER_SRC` | Listener used by daemon and local API harness runtime. |
| `core/http/src/MockHttpClient.cpp` | `HTTP_TEST_SUPPORT_SRC` | Mock implementation for tests and developer examples. |
| `core/http/src/TestHttpServer.cpp` | `HTTP_TEST_SUPPORT_SRC` | Test server for local integration and regression tests. |

Header-only request/response/interface types should be documented as part of the public HTTP client contract when packaging documentation is written.

---

## Dependency Direction

Allowed direction:

```text
core HTTP request/response/interface
  -> HTTP client implementation
  -> VDR RESTfulAPI adapters / tools

core HTTP listener
  -> REST router / daemon application

HTTP test support
  -> tests and smoke harnesses only
```

Forbidden direction:

```text
HTTP client -> daemon runtime
HTTP client -> REST controllers
HTTP test support -> production daemon source group
TestHttpServer -> public HTTP package by default
```

---

## Safe Refactor Order

A later code/build phase should refactor in this order:

1. Introduce `HTTP_CLIENT_SRC`, `HTTP_SERVER_SRC` and `HTTP_TEST_SUPPORT_SRC` in `mk/http-sources.mk`.
2. Keep a transitional `HTTP_SRC` aggregate only if existing tests still reference it.
3. Update compile targets one by one to use the narrowest correct group.
4. Run all existing HTTP, RESTfulAPI adapter, REST controller, daemon and real-backend smoke helper build tests.
5. Remove transitional aggregate only after every target is explicit.

---

## Expected Build Intent

The eventual source map should look like:

```make
HTTP_CLIENT_SRC := \
        core/http/src/BasicHttpClient.cpp

HTTP_SERVER_SRC := \
        core/http/src/SimpleHttpListener.cpp

HTTP_TEST_SUPPORT_SRC := \
        core/http/src/MockHttpClient.cpp \
        core/http/src/TestHttpServer.cpp
```

This makes packaging safer because the public client library can be documented independently from daemon listener and test infrastructure.

---

## Packaging Rules

Public package candidate:

- HTTP request/response/interface headers
- `BasicHttpClient` implementation when dependency and ABI expectations are documented

Internal/runtime package candidate:

- `SimpleHttpListener`, if daemon or API harness packages require it

Test/developer support only:

- `MockHttpClient`
- `TestHttpServer`

---

## Verification

Required local verification after this documentation-only plan:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required yet because this phase does not change Makefile behavior.

---

## Back

- [Back to Development Index](index.md)
