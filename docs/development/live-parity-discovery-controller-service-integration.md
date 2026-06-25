# Live Parity Discovery Controller Service Integration

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [Live Plugin Parity Source Audit](live-plugin-parity-source-audit.md)
- [Live Parity Discovery Domain Foundation](live-parity-discovery-domain-foundation.md)
- [Live Parity Discovery JSON Contract](live-parity-discovery-json-contract.md)
- [Live Parity Discovery REST Controller Contract](live-parity-discovery-rest-controller-contract.md)
- [Live Parity Discovery Service Contract](live-parity-discovery-service-contract.md)

---

## Purpose

Phase 51.5 connects the read-only Live parity discovery controller to the read-only discovery service contract.

The controller can now obtain a SearchTimerDiscoveryCatalog by backendId through SearchTimerDiscoveryService while preserving the existing direct catalog response path.

---

## Contract

The controller now supports two read-only paths:

- getDiscovery(SearchTimerDiscoveryCatalog)
- getDiscovery(backendId)

The backendId path delegates to SearchTimerDiscoveryService::discover(backendId) and then serializes the returned catalog through SearchTimerDiscoveryJsonSerializer.

A controller created without a service returns HTTP-style statusCode 503 for backendId discovery requests instead of dereferencing a missing dependency.

---

## Boundary

This phase intentionally does not add:

- ApiRouter route wiring
- daemon endpoint exposure
- RESTfulAPI HTTP transport
- live backend discovery fetching
- provider registry
- SearchTimer writes
- epgsearch writes

The integration remains a read-only in-process controller/service boundary.

---

## Verification

Targeted verification:

- make test-search-timer-discovery-controller
- make test-search-timer-discovery-service
- make test-search-timer-discovery-json-serializer
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.6 - Live parity discovery router contract

The next phase should add read-only ApiRouter route wiring with a safe in-memory or injected discovery provider boundary and still avoid real RESTfulAPI transport.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
