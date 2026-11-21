# Live Parity Discovery Domain Foundation

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)
- [Live Plugin Parity Source Audit](live-plugin-parity-source-audit.md)

---

## Purpose

Phase 51.1 adds the first read-only Live parity discovery domain foundation.

The domain models describe helper lists exposed by the Live / epgsearch / RESTfulAPI SearchTimer surface without creating REST routes, adapter mappings or backend mutations.

---

## Scope

Implemented read-only discovery domain objects:

- SearchTimerDiscoveryExtendedEpgInfo
- SearchTimerDiscoveryChannelGroup
- SearchTimerDiscoveryBlacklist
- SearchTimerDiscoveryRecordingDirectory
- SearchTimerDiscoveryCatalog

The catalog keeps backend identity so later phases can expose discovery data per VDR backend.

---

## Source Alignment

The model follows the existing RESTfulAPI / epgsearch helper surfaces:

- Extended EPG info uses id, name, values and config.
- Channel groups are represented by group names.
- Blacklists are represented by id and search expression.
- Recording directories are represented by directory path strings.

---

## Build System Finding

During Phase 51.1 verification, the daemon build exposed that SearchTimerController uses SearchTimerWorkflowValidationRequestParser while mk/daemon-sources.mk did not link the parser source.

This phase fixes the daemon source list by adding api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp to DAEMON_SRC.

---

## Non-Goals

Phase 51.1 intentionally does not add:

- JSON serialization
- REST controller routes
- RESTfulAPI adapter mapping
- real yaVDR HTTP calls
- production SearchTimer mutation
- timer conflict modeling
- SearchTimer preview parity

---

## Safety Result

This phase is read-only and domain-only.

It cannot mutate VDR, epgsearch or RESTfulAPI state.

---

## Verification

Targeted verification:

- make test-search-timer-discovery
- make test-docs
- make test-phase
- make daemon

---

## Next Phase

Phase 51.2 - Live parity discovery JSON contract

The next phase should serialize the discovery catalog into a stable frontend-ready JSON contract without adding backend transport yet.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
