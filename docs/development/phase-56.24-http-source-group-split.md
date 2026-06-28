# Phase 56.24 - HTTP Source Group Split

## Navigation

- [Development Index](index.md)
- [Phase 56.21 - Build Boundary Map](phase-56.21-build-boundary-map.md)
- [Phase 56.23 - HTTP Boundary Plan](phase-56.23-http-boundary-plan.md)

---

## Status

Completed.

---

## Purpose

Phase 56.24 introduces explicit HTTP source groups in `mk/http-sources.mk` while preserving the old aggregate variables for existing build targets.

This is the first small build-boundary refactor in Phase 56.

---

## Change

The HTTP build groups now expose explicit narrow boundaries:

```make
HTTP_CLIENT_SRC := \
        core/http/src/BasicHttpClient.cpp

HTTP_LISTENER_SRC := \
        core/http/src/SimpleHttpListener.cpp

HTTP_TEST_SUPPORT_SRC := \
        core/http/src/MockHttpClient.cpp \
        core/http/src/TestHttpServer.cpp
```

Existing compatibility aggregates remain available:

```make
HTTP_SRC := \
        $(HTTP_TEST_SUPPORT_SRC)

HTTP_SERVER_SRC := \
        $(HTTP_CLIENT_SRC) \
        $(HTTP_LISTENER_SRC) \
        core/http/src/TestHttpServer.cpp
```

---

## Boundary Result

| Source group | Boundary |
| --- | --- |
| `HTTP_CLIENT_SRC` | reusable HTTP client implementation |
| `HTTP_LISTENER_SRC` | daemon/API harness listener infrastructure |
| `HTTP_TEST_SUPPORT_SRC` | mock client and local test server support |
| `HTTP_SRC` | transitional compatibility aggregate |
| `HTTP_SERVER_SRC` | transitional server/harness aggregate |

---

## Compatibility

The old aggregate variables remain available so current Make targets do not need to change in this phase.

No source file moved directories.
No public header contract changed.
No daemon behavior changed.
No REST API behavior changed.

---

## Follow-up

A later phase may update individual Make targets to use the narrowest correct source group directly.

That should happen target-by-target with compile tests.

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

Recommended compile verification:

```bash
make test-mock-http-client
make test-restful-api-vdr-adapter
make test-vdr-controller
```

If a target name differs locally, list available HTTP-related targets with:

```bash
make -qp | grep -E "^test-.*http|^test-.*vdr-controller|^test-.*restful" | cut -d: -f1 | sort -u
```

---

## Back

- [Back to Development Index](index.md)
