# Live Parity Discovery Router Contract

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

---

## Purpose

Phase 51.6 adds the read-only ApiRouter route contract for Live parity discovery.

The route exposes the existing controller/service/serializer chain through ApiRouter when a SearchTimerDiscoveryController is injected.

---

## Routes

- GET /api/searchtimers/discovery
- GET /api/vdr/searchtimers/discovery

Supported query parameter:

- backend: optional backend id, defaults to default

---

## Contract

When the discovery controller is configured, the route returns:

- statusCode = 200
- contentType = application/json
- body = SearchTimerDiscoveryJsonSerializer output

When the discovery controller is not configured, the route returns:

- statusCode = 503
- contentType = application/json
- body error: searchtimer discovery unavailable

---

## Boundary

This phase intentionally does not add:

- daemon provider wiring
- RESTfulAPI HTTP transport
- live backend discovery fetching
- provider registry
- SearchTimer writes
- epgsearch writes

The route remains a read-only router contract over an injected controller.

---

## Verification

Targeted verification:

- make test-api-router
- make test-search-timer-discovery-controller
- make test-search-timer-discovery-service
- make test-search-timer-discovery-json-serializer
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.7 - Live parity discovery daemon wiring

The next phase should wire a daemon-safe discovery provider/controller so the daemon can expose the route without real RESTfulAPI transport yet.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
