# Phase 56.20 - Library, Daemon and API Boundary Audit

## Navigation

- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Completed Phases](completed-phases.md)

---

## Status

Completed.

---

## Purpose

Phase 56 is not only a documentation archive phase. The active Phase 56 milestone is Library Boundary, Packaging and Developer Documentation.

This audit establishes the current source-boundary state before changing build structure, public headers, packaging or developer-facing API documentation.

---

## Current Build Boundary Evidence

The current GNU Make build already separates source groups by responsibility:

- `VDR_SRC` collects the large VDR domain, adapter, SearchTimer, snapshot, live transport and backend-polling implementation set.
- `REST_*_SRC` collects REST controllers, router wiring and REST-facing request parsing.
- `DAEMON_SRC` composes daemon runtime, REST controllers, HTTP listener/client, VDR services and recording services into the application runtime.
- `RUNTIME_SRC` is already a compact runtime diagnostics and logging source group.
- `HTTP_SRC` and `HTTP_SERVER_SRC` separate mock/client-only HTTP use from listener/server use.
- `RECORDINGS_SRC`, `METADATA_SRC`, dashboard source groups and action/job source groups already express smaller library-like seams.

This means Phase 56 does not need to invent boundaries from nothing. It needs to make the existing implicit boundaries explicit and packageable.

---

## Current Boundary Map

| Area | Current location | Current role | Boundary classification |
| --- | --- | --- | --- |
| Recording repositories/services | `core/recordings` | Persistent recording, metadata, job and dashboard services | Library candidate |
| Recording actions | `core/recordings` plus REST parser dependency | Validation, dry-run and guarded execution support | Library candidate with REST dependency cleanup needed |
| VDR domain and services | `core/vdr` | Backend-neutral VDR domain, adapters, snapshots, SearchTimer and EPGSearch services | Library candidate, but too broad today |
| REST API | `api/rest` | Controllers, request parsing, router and transport-facing contracts | API surface, not core library |
| Runtime diagnostics | `core/runtime` | Runtime logging and diagnostics helpers | Library candidate |
| HTTP client/server | `core/http` | Basic HTTP client, listener and test server | Split candidate: client library versus application/test server |
| Daemon runtime | `core/daemon` | Application composition, runtime config and process lifecycle | Application-only / internal |
| Tools and smoke helpers | `apps/tools` | Operator/test helpers and real-backend smoke tools | Application/tooling-only |

---

## Public Library Candidates

### Core runtime library

Candidate public surface:

- runtime logging interfaces and implementations
- runtime diagnostics models and serializers
- diagnostics summary builder

Boundary rule:

- Must not depend on daemon runtime, REST controllers or VDR adapters.

### Core HTTP client library

Candidate public surface:

- `IHttpClient`
- HTTP request and response models
- basic HTTP client
- mock HTTP client for tests and examples

Boundary rule:

- HTTP listener and test server remain application/test infrastructure unless deliberately promoted later.

### Recording core library

Candidate public surface:

- recording repositories and services
- metadata repositories and services
- dashboard services and dashboard JSON serialization
- recording action validation models and services

Boundary rule:

- Recording action core must not require REST request parsers as library inputs. REST parsing belongs to `api/rest` and should map into core request models.

### VDR core library

Candidate public surface:

- backend-neutral VDR domain objects
- backend registry and capability services
- VDR service and overview services
- recording query, EPG query and EPGSearch services
- SearchTimer domain, validation, planning, preview and readback services
- snapshot cache and snapshot read services

Boundary rule:

- RESTfulAPI adapters may remain in a backend-adapter sub-boundary.
- Live transport, SSE transport and backend polling require explicit classification before they become public library surface.

### REST API integration library

Candidate public surface:

- REST controllers
- request parsers
- response contracts
- API router

Boundary rule:

- REST API is a transport-facing integration surface. It should depend on core libraries, not become a dependency of core libraries.

---

## Internal-only Areas

These areas should not be treated as public reusable library surface in Phase 56:

- `core/daemon` daemon composition and lifecycle implementation
- daemon runtime configuration wiring
- executable entry points under `apps/`
- real-backend smoke helpers unless documented as operator tools
- test-only HTTP server and mock live transport unless explicitly documented as test utilities
- production mutation gates and real backend executors until permissions and packaging policy are finalized

---

## Boundary Issues Found

### REST parser leakage into action source group

`ACTIONS_SRC` currently includes `api/rest/src/RecordingActionValidationRequestParser.cpp`. That makes the recording action source group depend on a REST parser.

Required follow-up:

- keep REST parsing in `api/rest`
- keep recording action validation input models in `core/recordings`
- document or refactor the build source group so core action tests do not imply a REST dependency unless testing REST behavior

### Cross-domain VDR dependency in recording action source group

`ACTIONS_SRC` currently includes `core/vdr/src/SearchTimerPreviewEpgCache.cpp`.

Required follow-up:

- verify whether this is an intentional shared test dependency, stale linkage or an accidental source-group expansion
- avoid making recording action libraries depend on SearchTimer preview cache unless explicitly justified

### VDR source group is too broad for a single public library

`VDR_SRC` currently contains backend registry, adapters, query services, EPGSearch, SearchTimer workflow, snapshot cache, change feed, live transport, polling and adapter factory sources.

Required follow-up:

- split documentation into sub-boundaries before changing Make targets
- likely sub-boundaries: `vdr-domain`, `vdr-adapters`, `vdr-snapshot`, `vdr-searchtimer`, `vdr-live`, `vdr-runtime-coordination`

### Daemon source group is application composition

`DAEMON_SRC` correctly composes many domains, REST controllers, HTTP server/client and runtime lifecycle code.

Boundary decision:

- `DAEMON_SRC` is not a library boundary.
- It is the application assembly target that consumes the library and API surfaces.

---

## Phase 56 Boundary Rules

1. Core libraries may expose domain models, services, repositories, serializers and backend-neutral interfaces.
2. REST controllers and request parsers belong to the REST API boundary.
3. Daemon runtime belongs to application composition and must not become part of the public library surface.
4. Real-backend mutation executors remain guarded and must not be promoted as safe public API without permission and packaging policy.
5. Build target names should document whether a group is core library, REST/API integration, daemon application or test/tooling support.
6. Documentation must be updated before structural source movement so future refactors have a stable target map.

---

## Proposed Next Phase

Phase 56.21 should define the explicit library/API boundary map for the build system.

Expected deliverables:

- a build-boundary document mapping existing `mk/*.mk` source groups to library/API/application/test classes
- a list of safe source-group renames or splits
- no code movement yet unless the boundary is trivial and covered by existing tests

---

## Verification

Required local verification after this documentation-only audit:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

---

## Back

- [Back to Development Index](index.md)
