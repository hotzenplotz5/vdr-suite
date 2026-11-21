# Phase 55.4c - SearchTimer Discovery Runtime Wiring

## Navigation

- [Development Index](index.md)
- [Phase 55.4 - RESTfulAPI SearchTimer Discovery Provider](phase-55.4-searchtimer-discovery-restfulapi-provider.md)
- [Phase 55.4b - Shared JSON String Decoder](phase-55.4b-shared-json-string-decoder.md)
- [Current Project Status](current-status.md)

---

## Status

Completed. GitHub Actions run 28288004880 passed docs-check, fast-regression-test and daemon build.

## Purpose

Phase 55.4 implemented the HTTP-backed RESTfulAPI SearchTimer discovery provider, but daemon runtime still constructed the static provider.

This phase wires daemon runtime to the HTTP-backed provider when a backend runtime context and HTTP client are available.

## Behaviour

The daemon runtime now uses the RESTfulAPI discovery provider for the active backend context.

The static provider remains available as fallback when no backend context exists.

## Changed files

```text
core/daemon/include/DaemonRuntime.h
core/daemon/src/DaemonRuntime.cpp
mk/daemon-sources.mk
mk/local-test-groups.mk
tools/check_searchtimer_discovery_runtime_wiring.py
```

## Verification coverage

New target:

```text
test-searchtimer-discovery-runtime-wiring
```

The target verifies the runtime wiring contract and is included in the VDR test group.

Verified checks:

```bash
make test-searchtimer-discovery-runtime-wiring
make test-restfulapi-search-timer-discovery-provider-contract
make test-docs
make test-phase
make daemon
```

## Back

- [Back to Development Index](index.md)
- [Back to Phase 55.4](phase-55.4-searchtimer-discovery-restfulapi-provider.md)
- [Back to Phase 55.4b](phase-55.4b-shared-json-string-decoder.md)
