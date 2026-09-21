# Live Parity Discovery RESTfulAPI Provider Contract

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
- [Live Parity Discovery Daemon Wiring](live-parity-discovery-daemon-wiring.md)
- [Live Parity Discovery HTTP Smoke Contract](live-parity-discovery-http-smoke-contract.md)

---

## Purpose

Phase 51.9 defines the RESTfulAPI-facing provider contract for Live parity discovery.

The provider implements ISearchTimerDiscoveryProvider but intentionally does not execute HTTP yet.

---

## Provider

Class:

- RestfulApiSearchTimerDiscoveryProvider

Contract:

- configuredBackendId
- discoveryEndpoint
- discover(backendId)

Default upstream endpoint contract:

- /searchtimers/discovery.json

---

## Pre-Transport Behavior

Until a dedicated transport phase is implemented, discover() returns a safe empty SearchTimerDiscoveryCatalog.

Backend id behavior:

- discover("") uses configuredBackendId
- discover("ferienhaus") uses the requested backend id

This keeps the provider usable behind SearchTimerDiscoveryService without making network calls.

---

## Boundary

This phase intentionally does not add:

- IHttpClient dependency
- HTTP execute
- JSON parsing
- RESTfulAPI response mapping
- epgsearch discovery fetching
- live backend discovery fetching
- SearchTimer writes
- epgsearch writes

---

## Verification

Targeted verification:

- make test-restfulapi-search-timer-discovery-provider-contract
- make test-search-timer-discovery-static-provider
- make test-search-timer-discovery-service
- make test-test-http-server
- make test-api-router
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.10 - Live parity discovery foundation completion

The next phase should close Phase 51 with a completion document and freeze the pre-transport discovery boundary before Phase 52 begins.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
