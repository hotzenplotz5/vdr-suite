# Phase 56.22 - VDR Source Boundary Plan

## Navigation

- [Development Index](index.md)
- [Phase 56.20 - Library, Daemon and API Boundary Audit](phase-56.20-library-daemon-api-boundary-audit.md)
- [Phase 56.21 - Build Boundary Map](phase-56.21-build-boundary-map.md)
- [Roadmap](../planning/roadmap.md)

---

## Status

Completed.

---

## Purpose

`VDR_SRC` is currently the largest source group in the GNU Make build. It mixes domain objects, backend registries, adapters, RESTfulAPI mapping, EPG services, SearchTimer services, snapshot runtime, change feed, live transport and polling coordination.

This plan defines the target VDR source sub-boundaries before any source movement or Makefile refactor is attempted.

---

## Boundary Rule

The current `VDR_SRC` group remains valid until a later refactor proves smaller source groups through existing tests.

Future changes must split by dependency direction, not by feature enthusiasm:

```text
VDR domain -> VDR services -> adapter/integration -> REST/API -> daemon application
```

No lower-level VDR source group may depend on REST controllers or daemon runtime.

---

## Target Source Groups

| Target group | Classification | Responsibility |
| --- | --- | --- |
| `VDR_DOMAIN_SRC` | Core library | Backend-neutral VDR objects, query/result models and lightweight contract types. |
| `VDR_BACKEND_SRC` | Core library | Backend registry, backend capabilities, adapter interfaces and adapter factory. |
| `VDR_RESTFULAPI_ADAPTER_SRC` | Adapter integration | RESTfulAPI mappers, RESTfulAPI adapters and RESTfulAPI command executors. |
| `VDR_EPG_SRC` | Core library | EPG query, EPG search, matchers, request mapping and result serialization. |
| `VDR_SEARCHTIMER_SRC` | Core library / guarded workflow | SearchTimer domain, validation, preview, planning, workflow execution and readback services. |
| `VDR_SNAPSHOT_SRC` | Core runtime library | Snapshot builder, snapshot cache, snapshot access, snapshot read and change-feed services. |
| `VDR_LIVE_SRC` | Runtime integration | Live update events, live transport service, live transport factory and SSE transport. |
| `VDR_RUNTIME_COORDINATION_SRC` | Daemon-adjacent core | Polling, change detection, refresh planning and backend polling coordination. |
| `VDR_TEST_SUPPORT_SRC` | Test support | Mock adapters, test live transport and other test doubles. |

---

## Proposed File Classification

### `VDR_DOMAIN_SRC`

Examples:

- VDR status, channel, event, timer and recording domain objects
- query request/result models
- capability state/report value types
- SearchTimer and EPG value models only when they are dependency-light

Target rule:

- This group should have no HTTP, REST, daemon, polling or backend-specific command executor dependencies.

### `VDR_BACKEND_SRC`

Examples:

- `BackendRegistry.cpp`
- `BackendRegistryService.cpp`
- `BackendRegistryJsonSerializer.cpp`
- `CapabilityStateJsonSerializer.cpp`
- `CapabilityReportJsonSerializer.cpp`
- `CapabilityReportService.cpp`
- `ExternalVdrAdapter.cpp`
- `MockVdrAdapter.cpp`
- `VdrAdapterFactory.cpp`
- `VdrConfig.cpp`
- `VdrService.cpp`
- `VdrOverviewService.cpp`
- `VdrOverviewJsonSerializer.cpp`

Target rule:

- Backend-neutral services may depend on adapter interfaces and domain models.
- Backend-specific RESTfulAPI implementation should move to `VDR_RESTFULAPI_ADAPTER_SRC`.

### `VDR_RESTFULAPI_ADAPTER_SRC`

Examples:

- `RestfulApiChannelMapper.cpp`
- `RestfulApiRecordingMapper.cpp`
- `RestfulApiTimerMapper.cpp`
- `RestfulApiEventMapper.cpp`
- `RestfulApiStatusMapper.cpp`
- `RestfulApiVdrAdapter.cpp`
- `RestfulApiSearchTimerCommandExecutor.cpp`
- RESTfulAPI SearchTimer discovery/provider/mapper/adapter sources currently compiled through daemon-specific groups
- RESTfulAPI timer and recording action executors where applicable

Target rule:

- This group may depend on HTTP client and core VDR domain/services.
- This group must not depend on REST controllers or daemon runtime.

### `VDR_EPG_SRC`

Examples:

- `EpgQueryService.cpp`
- `EpgSearchMatcher.cpp`
- `EpgSearchResultJsonSerializer.cpp`
- `EpgSearchRequestMapper.cpp`
- `EpgSearchService.cpp`
- native fuzzy capability/freshness/restore/probe services currently compiled by daemon groups

Target rule:

- Query and matcher logic is core library surface.
- Operator refresh and stale-probe administration may require a separate API integration group if they become transport-facing.

### `VDR_SEARCHTIMER_SRC`

Examples:

- create/update/delete services and serializers
- preview service, preview result serializer and preview EPG cache
- discovery service and discovery serializers
- workflow validation and planning services
- guarded executor invocation, production policy gates and readback verification services
- workflow command dispatch and execution services
- workflow execution/plan/validation/result serializers

Target rule:

- Read-only validation, preview and planning are core library candidates.
- Real execution and mutation policy remain guarded and must not be packaged as safe public APIs without permission policy documentation.
- REST request parsers remain in `api/rest`.

### `VDR_SNAPSHOT_SRC`

Examples:

- `VdrSnapshotBuilder.cpp`
- `SnapshotCache.cpp`
- `SnapshotCacheService.cpp`
- `SnapshotAccessService.cpp`
- `VdrSnapshotReadService.cpp`
- `VdrSnapshotReadJsonSerializer.cpp`
- `SnapshotChangeFeedEntry.cpp`
- `SnapshotChangeFeed.cpp`
- `SnapshotChangeFeedService.cpp`
- `SnapshotChangeFeedJsonSerializer.cpp`

Target rule:

- Snapshot read and cache services are reusable runtime libraries.
- Startup polling or daemon refresh scheduling belongs outside this group.

### `VDR_LIVE_SRC`

Examples:

- `LiveUpdateEvent.cpp`
- `LiveUpdateEventJsonSerializer.cpp`
- `LiveTransportService.cpp`
- `LiveTransportFactory.cpp`
- `SseLiveTransport.cpp`

Target rule:

- Live event models and serializers may be core runtime library surface.
- SSE transport is runtime integration and may remain internal until transport packaging is defined.

### `VDR_RUNTIME_COORDINATION_SRC`

Examples:

- `PollingService.cpp`
- `VdrChangeState.cpp`
- `VdrChangeEvent.cpp`
- `ChangeDetectionService.cpp`
- `SnapshotRefreshDecision.cpp`
- `SnapshotRefreshDecisionService.cpp`
- `SnapshotUpdatePlan.cpp`
- `SnapshotRefreshPlanner.cpp`
- `BackendPollingCoordinator.cpp`

Target rule:

- This group coordinates runtime behavior and may be used by daemon composition.
- It should remain separate from pure snapshot read/cache library surface.

### `VDR_TEST_SUPPORT_SRC`

Examples:

- `MockVdrAdapter.cpp` when used as a test double
- `TestLiveTransport.cpp`

Target rule:

- Test doubles may be available for developers, but they must not be confused with production library code.

---

## Refactor Order

A safe future refactor should proceed in this order:

1. Add new Make source variables while keeping `VDR_SRC` as an aggregate.
2. Make `VDR_SRC` compose the new subgroups instead of listing every file directly.
3. Run the full existing VDR, REST, daemon and documentation tests.
4. Only then allow selected tests or tools to depend on smaller groups directly.
5. Do not move files between directories until the source-variable split is stable.

---

## Initial Target Aggregate

The eventual structure should look like this:

```make
VDR_SRC := \
        $(VDR_DOMAIN_SRC) \
        $(VDR_BACKEND_SRC) \
        $(VDR_RESTFULAPI_ADAPTER_SRC) \
        $(VDR_EPG_SRC) \
        $(VDR_SEARCHTIMER_SRC) \
        $(VDR_SNAPSHOT_SRC) \
        $(VDR_LIVE_SRC) \
        $(VDR_RUNTIME_COORDINATION_SRC) \
        $(VDR_TEST_SUPPORT_SRC)
```

This preserves existing build behavior while making boundary intent visible.

---

## Risks

- SearchTimer code touches validation, preview, workflow planning, execution, policy gates and RESTfulAPI command execution; it should not be split casually.
- Snapshot and polling code are related but not the same boundary.
- Live transport has both model and transport concerns.
- RESTfulAPI adapter code is backend integration, not REST API controller code.
- Mock/test support must stay obvious so package consumers do not treat it as production surface.

---

## Verification

Required local verification after this documentation-only plan:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required yet because this phase does not change Makefile behavior.

---

## Back

- [Back to Development Index](index.md)
