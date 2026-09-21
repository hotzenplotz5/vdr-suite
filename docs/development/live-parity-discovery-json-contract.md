# Live Parity Discovery JSON Contract

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [Live Plugin Parity Source Audit](live-plugin-parity-source-audit.md)
- [Live Parity Discovery Domain Foundation](live-parity-discovery-domain-foundation.md)

---

## Purpose

Phase 51.2 adds a stable frontend-facing JSON contract for the read-only Live parity discovery catalog.

The serializer turns backend-neutral discovery domain objects into JSON without adding REST routes, backend transport or backend mutation.

---

## JSON Shape

The serialized catalog contains:

- backendId
- counts
- extendedEpgInfo
- channelGroups
- blacklists
- recordingDirectories

The shape is intentionally close to the RESTfulAPI helper-list surfaces while preserving VDR-Suite backend identity.

---

## Safety Boundary

This phase remains read-only.

It does not add:

- REST routes
- ApiRouter wiring
- RESTfulAPI HTTP calls
- daemon runtime fetching
- SearchTimer writes
- epgsearch writes
- timer conflict execution

---

## Verification

Targeted verification:

- make test-search-timer-discovery-json-serializer
- make test-search-timer-discovery
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.3 - Live parity discovery REST controller contract

The next phase should expose the discovery JSON contract behind a read-only controller boundary without adding real backend transport yet.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
