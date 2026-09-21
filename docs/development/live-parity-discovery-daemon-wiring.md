# Live Parity Discovery Daemon Wiring

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
- [Live Parity Discovery Controller Service Integration](live-parity-discovery-controller-service-integration.md)
- [Live Parity Discovery Router Contract](live-parity-discovery-router-contract.md)

---

## Purpose

Phase 51.7 wires Live parity discovery into the daemon runtime with a safe static provider.

The daemon can now inject SearchTimerDiscoveryController into ApiRouter so the discovery route is reachable through the running runtime.

---

## Provider Boundary

SearchTimerDiscoveryStaticProvider returns a stable empty SearchTimerDiscoveryCatalog for the requested backendId.

This is intentional:

- it proves the runtime wiring
- it keeps frontend/API contracts stable
- it avoids fake Live data
- it avoids RESTfulAPI transport
- it avoids epgsearch reads and writes

---

## Runtime Chain

DaemonRuntime now owns:

- SearchTimerDiscoveryStaticProvider
- SearchTimerDiscoveryService
- SearchTimerDiscoveryJsonSerializer
- SearchTimerDiscoveryController

ApiRouter receives the discovery controller pointer during daemon initialization.

---

## HTTP Smoke

test-test-http-server verifies:

- GET /api/searchtimers/discovery?backend=http-server
- statusCode 200
- application/json content type
- backendId propagation
- zero discovery counts
- stable empty arrays

---

## Boundary

This phase intentionally does not add:

- RESTfulAPI discovery transport
- epgsearch discovery fetching
- live backend discovery fetching
- provider registry
- SearchTimer writes
- epgsearch writes

---

## Verification

Targeted verification:

- make test-search-timer-discovery-static-provider
- make test-test-http-server
- make test-api-router
- make test-search-timer-discovery-controller
- make test-search-timer-discovery-service
- make test-search-timer-discovery-json-serializer
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.8 - Live parity discovery HTTP smoke contract

The next phase should strengthen the HTTP route contract and document the observable empty-provider response as the final pre-transport daemon behavior.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
