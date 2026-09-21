# Live Parity Discovery REST Controller Contract

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

---

## Purpose

Phase 51.3 adds the read-only REST controller contract for the Live parity discovery catalog.

The controller accepts an already-built SearchTimerDiscoveryCatalog and returns an ApiResponse with the stable discovery JSON contract.

---

## Contract

The controller response contract is:

- statusCode = 200
- contentType = application/json
- body = SearchTimerDiscoveryJsonSerializer output

---

## Boundary

This phase intentionally does not add:

- ApiRouter route wiring
- daemon endpoint exposure
- RESTfulAPI HTTP transport
- live backend discovery fetching
- service/provider abstraction
- SearchTimer writes
- epgsearch writes

The controller remains a pure response boundary over a supplied catalog.

---

## Verification

Targeted verification:

- make test-search-timer-discovery-controller
- make test-search-timer-discovery-json-serializer
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.4 - Live parity discovery service contract

The next phase should add a read-only service/provider boundary that can later feed the controller and router without introducing backend mutation.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
