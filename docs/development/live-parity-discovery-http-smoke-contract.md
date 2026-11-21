# Live Parity Discovery HTTP Smoke Contract

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

---

## Purpose

Phase 51.8 strengthens the HTTP smoke contract for the daemon-exposed Live parity discovery route.

The phase does not introduce new backend behavior. It turns the observable pre-transport response shape into an explicit TestHttpServer contract.

---

## Covered Routes

- GET /api/searchtimers/discovery?backend=http-server
- GET /api/vdr/searchtimers/discovery
- GET /api/vdr/searchtimers/discovery?backend=ferienhaus
- POST /api/searchtimers/discovery

---

## GET Contract

The daemon-safe static provider response is intentionally empty but structurally stable.

The exact JSON contract contains:

- backendId
- counts.extendedEpgInfo = 0
- counts.channelGroups = 0
- counts.blacklists = 0
- counts.recordingDirectories = 0
- extendedEpgInfo = []
- channelGroups = []
- blacklists = []
- recordingDirectories = []

The /api/vdr/searchtimers/discovery alias defaults to backendId default when no backend query parameter is supplied.

---

## POST Boundary

POST /api/searchtimers/discovery returns the generic 404 not-found response.

This keeps discovery explicitly read-only and avoids any accidental mutation surface.

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

- make test-test-http-server
- make test-search-timer-discovery-static-provider
- make test-api-router
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.9 - Live parity discovery RESTfulAPI provider contract

The next phase should define the RESTfulAPI-facing provider contract and mapping boundary without adding live HTTP transport yet.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
