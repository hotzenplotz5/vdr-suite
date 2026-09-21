# Live Parity Discovery Service Contract

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

---

## Purpose

Phase 51.4 adds the read-only service and provider contract for Live parity discovery data.

The service delegates discovery to an injected provider and returns a SearchTimerDiscoveryCatalog for a requested backendId.

---

## Contract

The service contract is:

- ISearchTimerDiscoveryProvider::discover(backendId)
- SearchTimerDiscoveryService::discover(backendId)
- return type: SearchTimerDiscoveryCatalog

The provider boundary keeps future RESTfulAPI, epgsearch or mock discovery implementations replaceable.

---

## Boundary

This phase intentionally does not add:

- controller rewiring to the service
- ApiRouter route wiring
- daemon endpoint exposure
- RESTfulAPI HTTP transport
- live backend discovery fetching
- SearchTimer writes
- epgsearch writes

The service remains a read-only domain boundary.

---

## Verification

Targeted verification:

- make test-search-timer-discovery-service
- make test-search-timer-discovery-controller
- make test-search-timer-discovery-json-serializer
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.5 - Live parity discovery controller service integration

The next phase should connect the controller to the service contract without adding ApiRouter route wiring or real backend transport yet.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
