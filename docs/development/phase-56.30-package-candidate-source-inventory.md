# Phase 56.30 - Package Candidate Source Inventory

## Navigation

- [Development Index](index.md)
- [Phase 56.21 - Build Boundary Map](phase-56.21-build-boundary-map.md)
- [Phase 56.24 - HTTP Source Group Split](phase-56.24-http-source-group-split.md)
- [Phase 56.29 - Packaging Boundary Guide](phase-56.29-packaging-boundary-guide.md)

---

## Status

Completed.

---

## Purpose

Phase 56.30 records the first concrete source inventory for package candidates.

This phase does not create packages and does not move source files. It maps existing Make source groups and known mixed aggregates to the packaging candidates from Phase 56.29.

The inventory intentionally marks several areas as `split-required` because current source groups still combine public, internal, REST, daemon, tooling and test-support code.

---

## Source Inputs

This inventory is based on the current build source group files:

- `mk/runtime-sources.mk`
- `mk/http-sources.mk`
- `mk/recording-sources.mk`
- `mk/action-job-sources.mk`
- `mk/vdr-sources.mk`
- `mk/rest-sources.mk`
- `mk/daemon-sources.mk`

---

## Inventory Status Labels

| Label | Meaning |
| --- | --- |
| `candidate` | Good initial public package candidate after header audit. |
| `internal` | Shared repository code, not public package surface yet. |
| `daemon-only` | Application/runtime composition only. |
| `rest-implementation` | REST controller/parser implementation, not core library API. |
| `tooling` | Operator/smoke/local validation support. |
| `test-support` | Mock/fake/test server support only. |
| `split-required` | Current source group mixes multiple package classes. |

---

## Runtime Core Candidate

Candidate package:

```text
vdr-suite-runtime-core
```

Current source group:

```make
RUNTIME_SRC := \
        core/runtime/src/NullRuntimeLogger.cpp \
        core/runtime/src/ConsoleRuntimeLogger.cpp \
        core/runtime/src/RuntimeDiagnosticsJsonSerializer.cpp \
        core/runtime/src/RuntimeDiagnosticsSummaryBuilder.cpp
```

Inventory result:

| Source | Initial classification |
| --- | --- |
| `core/runtime/src/NullRuntimeLogger.cpp` | candidate |
| `core/runtime/src/ConsoleRuntimeLogger.cpp` | candidate |
| `core/runtime/src/RuntimeDiagnosticsJsonSerializer.cpp` | candidate |
| `core/runtime/src/RuntimeDiagnosticsSummaryBuilder.cpp` | candidate |

Notes:

- This is a good first package candidate because it is small and already separated.
- Header audit is still required before exposing a public API.

---

## HTTP Package Candidates

Candidate packages:

```text
vdr-suite-http-client
vdr-suite-http-listener-internal
vdr-suite-test-support-http
```

Current source groups:

```make
HTTP_CLIENT_SRC := \
        core/http/src/BasicHttpClient.cpp

HTTP_LISTENER_SRC := \
        core/http/src/SimpleHttpListener.cpp

HTTP_TEST_SUPPORT_SRC := \
        core/http/src/MockHttpClient.cpp \
        core/http/src/TestHttpServer.cpp
```

Inventory result:

| Source | Initial classification |
| --- | --- |
| `core/http/src/BasicHttpClient.cpp` | candidate |
| `core/http/src/SimpleHttpListener.cpp` | internal / daemon-adjacent |
| `core/http/src/MockHttpClient.cpp` | test-support |
| `core/http/src/TestHttpServer.cpp` | test-support |

Notes:

- The HTTP client is a strong package candidate.
- Listener and test server must not be pulled into the public client package.
- Existing compatibility aggregates `HTTP_SRC` and `HTTP_SERVER_SRC` remain transitional only.

---

## Recording Core Candidate

Candidate package:

```text
vdr-suite-recording-core
```

Current source group:

```make
RECORDINGS_SRC := \
        core/recordings/src/RecordingRepository.cpp \
        core/recordings/src/MetadataRepository.cpp \
        core/recordings/src/RecordingService.cpp
```

Inventory result:

| Source | Initial classification |
| --- | --- |
| `core/recordings/src/RecordingRepository.cpp` | candidate |
| `core/recordings/src/MetadataRepository.cpp` | candidate |
| `core/recordings/src/RecordingService.cpp` | candidate |

Notes:

- Good first read/query package candidate.
- Repository dependency assumptions need a header/source audit before packaging.

---

## Metadata Core Candidate

Candidate package:

```text
vdr-suite-metadata-core
```

Current source group:

```make
METADATA_SRC := \
        core/recordings/src/MetadataRepository.cpp \
        core/recordings/src/MetadataService.cpp
```

Inventory result:

| Source | Initial classification |
| --- | --- |
| `core/recordings/src/MetadataRepository.cpp` | candidate |
| `core/recordings/src/MetadataService.cpp` | candidate |

Notes:

- Metadata core overlaps with recording core through `MetadataRepository.cpp`.
- Packaging must decide whether metadata remains inside recording core or becomes a separate library with a shared persistence dependency.

---

## Dashboard Read Model Candidate

Candidate package:

```text
vdr-suite-dashboard-readmodel
```

Current source groups:

```make
JOB_DASHBOARD_SRC
RECORDING_DASHBOARD_SRC
DASHBOARD_FACADE_SRC
DASHBOARD_JSON_SRC
```

Inventory result:

| Source area | Initial classification |
| --- | --- |
| `JobRepository.cpp` / `JobDashboardService.cpp` | internal candidate |
| `RecordingDashboardService.cpp` | internal candidate |
| `DashboardFacade.cpp` | internal candidate |
| `DashboardJsonSerializer.cpp` | candidate if JSON contract is reused outside REST |

Notes:

- Dashboard read models are useful, but they currently compose recording and job internals.
- Public packaging should wait until the recording/job boundary is explicit.

---

## Recording Action and Workflow Inventory

Current source group:

```make
ACTIONS_SRC := \
        core/recordings/src/RecordingActionUtils.cpp \
        core/recordings/src/RecordingActionValidationResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionExecutionResultJsonSerializer.cpp \
        core/recordings/src/RecordingActionValidationService.cpp \
        api/rest/src/RecordingActionValidationRequestParser.cpp \
        core/vdr/src/SearchTimerPreviewEpgCache.cpp
```

Inventory result:

| Source | Initial classification |
| --- | --- |
| `RecordingActionUtils.cpp` | internal |
| `RecordingActionValidationResultJsonSerializer.cpp` | candidate/internal |
| `RecordingActionExecutionResultJsonSerializer.cpp` | internal until mutation policy packaging exists |
| `RecordingActionValidationService.cpp` | candidate for validation-only package |
| `api/rest/src/RecordingActionValidationRequestParser.cpp` | rest-implementation |
| `core/vdr/src/SearchTimerPreviewEpgCache.cpp` | split-required / VDR cache dependency |

Notes:

- `ACTIONS_SRC` is explicitly `split-required`.
- REST parsing must move out of any recording core package candidate.
- VDR SearchTimer cache dependency must not be part of a recording-action core package.

Workflow-related source groups:

```make
ACTION_SERVICE_SRC
JOB_SERVICE_SRC
JOB_REPOSITORY_SRC
WORKFLOW_SRC
WORKER_SRC
RECTOOLS_ADAPTER_SRC
```

Initial classification:

| Source group | Initial classification |
| --- | --- |
| `ACTION_SERVICE_SRC` | internal |
| `JOB_SERVICE_SRC` | internal |
| `JOB_REPOSITORY_SRC` | internal |
| `WORKFLOW_SRC` | internal / daemon-adjacent |
| `WORKER_SRC` | tooling/internal |
| `RECTOOLS_ADAPTER_SRC` | backend/tooling integration |

---

## VDR Source Inventory

Current source group:

```make
VDR_SRC := \
        core/vdr/src/VdrConfig.cpp \
        core/vdr/src/BackendRegistry.cpp \
        core/vdr/src/BackendRegistryService.cpp \
        ...
```

`VDR_SRC` remains a broad mixed aggregate and is `split-required`.

### VDR Domain Candidate

Candidate package:

```text
vdr-suite-vdr-domain
```

Initial source/header area:

| Area | Initial classification |
| --- | --- |
| VDR status/channel/event/timer/recording domain objects | candidate |
| plain capability value objects | candidate |
| backend-neutral query/result models | candidate |

Notes:

- Several domain objects may be header-only and therefore are not visible in the current `VDR_SRC` source list.
- A header inventory is required before packaging.

### VDR Adapter API Candidate

Candidate package:

```text
vdr-suite-vdr-adapter-api
```

Initial source/header area:

| Area | Initial classification |
| --- | --- |
| adapter interface headers | candidate |
| backend capability models | candidate |
| backend registry contract | candidate after dependency audit |
| `BackendRegistry.cpp` | candidate/internal pending header audit |
| `BackendRegistryService.cpp` | candidate/internal pending dependency audit |

### Concrete VDR Adapter Packages

Candidate package:

```text
vdr-suite-vdr-restfulapi-adapter
```

Initial source area:

| Source | Initial classification |
| --- | --- |
| `RestfulApiVdrAdapter.cpp` | concrete backend adapter |
| `RestfulApiChannelMapper.cpp` | concrete backend mapper |
| `RestfulApiRecordingMapper.cpp` | concrete backend mapper |
| `RestfulApiTimerMapper.cpp` | concrete backend mapper |
| `RestfulApiEventMapper.cpp` | concrete backend mapper |
| `RestfulApiStatusMapper.cpp` | concrete backend mapper |
| `ExternalVdrAdapter.cpp` | internal/tooling adapter |
| `MockVdrAdapter.cpp` | test-support unless promoted |
| `VdrAdapterFactory.cpp` | internal composition until package boundary is audited |

### VDR Read Service Candidate

Candidate package:

```text
vdr-suite-vdr-service
```

Initial source area:

| Source | Initial classification |
| --- | --- |
| `VdrService.cpp` | candidate |
| `VdrOverviewService.cpp` | candidate |
| `VdrOverviewJsonSerializer.cpp` | candidate if JSON is public contract |
| `VdrRecordingQueryService.cpp` | candidate |
| `VdrRecordingQueryMatcher.cpp` | candidate |
| `VdrRecordingQueryResultJsonSerializer.cpp` | candidate/internal |

### EPG Core Candidate

Candidate package:

```text
vdr-suite-epg-core
```

Initial source area:

| Source | Initial classification |
| --- | --- |
| `EpgQueryService.cpp` | candidate |
| `EpgSearchMatcher.cpp` | candidate |
| `EpgSearchRequestMapper.cpp` | candidate/internal |
| `EpgSearchService.cpp` | candidate |
| `EpgSearchResultJsonSerializer.cpp` | candidate/internal |

### SearchTimer Read-Only Candidate

Candidate package:

```text
vdr-suite-searchtimer-readonly
```

Initial source area:

| Source | Initial classification |
| --- | --- |
| `SearchTimerPreviewService.cpp` | candidate |
| `SearchTimerPreviewResultJsonSerializer.cpp` | candidate/internal |
| `SearchTimerAutomationReadOnlyService.cpp` | candidate |
| `SearchTimerAutomationDryRunResultJsonSerializer.cpp` | candidate/internal |
| `SearchTimerAutomationSafetyReview.cpp` | candidate/internal |
| `SearchTimerDiscoveryService.cpp` | candidate/internal |
| `SearchTimerDiscoveryStaticProvider.cpp` | candidate/internal |
| `RestfulApiSearchTimerDiscoveryProvider.cpp` | concrete backend integration |
| `SearchTimerDiscoveryJsonSerializer.cpp` | candidate/internal |

### SearchTimer Mutation/Internal Inventory

Initial source area:

| Source area | Initial classification |
| --- | --- |
| create/update/delete services | internal |
| workflow execution services | internal |
| production policy gates | internal |
| backend write allowlist/switches | internal |
| readback verification services | internal until mutation package policy exists |
| `RestfulApiSearchTimerCommandExecutor.cpp` | concrete backend mutation executor |
| `SearchTimerRuntimeMutationPolicyExecutor.cpp` | internal runtime policy |

### Snapshot and Live Inventory

Initial classification:

| Source area | Initial classification |
| --- | --- |
| snapshot read services and serializers | candidate after cache ownership audit |
| snapshot cache/update/planner services | internal / daemon-adjacent |
| change-feed services and serializers | candidate/internal |
| live transport service/factory/SSE transport | internal / daemon-adjacent |
| `TestLiveTransport.cpp` | test-support |
| `BackendPollingCoordinator.cpp` | daemon-adjacent internal |

---

## REST Source Inventory

REST implementation package candidate:

```text
vdr-suite-rest-implementation
```

Contract/documentation candidate:

```text
vdr-suite-rest-api-contracts
```

Current source groups:

```make
REST_DASHBOARD_SRC
REST_VDR_SRC
REST_EPG_SRC
REST_RUNTIME_SRC
REST_SNAPSHOT_CHANGE_FEED_SRC
REST_LIVE_TRANSPORT_SRC
REST_ROUTER_SRC
```

Inventory result:

| Source area | Initial classification |
| --- | --- |
| `api/rest/src/*Controller.cpp` | rest-implementation |
| `api/rest/src/*RequestParser.cpp` | rest-implementation |
| `api/rest/src/RestQueryParameters.cpp` | rest-implementation / utility |
| `api/rest/src/ApiRouter.cpp` | rest-implementation / application-adjacent |
| endpoint schemas/docs | API contract package, not C++ implementation |

Notes:

- `REST_ROUTER_SRC` is `split-required` because it includes controllers and core service dependencies.
- REST contract packaging must be documentation/schema oriented, not a direct dump of C++ controllers.

---

## Daemon Source Inventory

Application package candidate:

```text
vdr-suite-daemon
```

Current source group:

```make
DAEMON_SRC := \
        core/daemon/src/RuntimeConfig.cpp \
        ... \
        core/daemon/src/DaemonRuntime.cpp \
        core/daemon/src/DaemonApp.cpp \
        ...
```

Inventory result:

| Source area | Initial classification |
| --- | --- |
| `core/daemon/src/RuntimeConfig.cpp` | daemon-only |
| `core/daemon/src/DaemonRuntime.cpp` | daemon-only |
| `core/daemon/src/DaemonApp.cpp` | daemon-only |
| REST controllers included by `DAEMON_SRC` | rest-implementation |
| VDR/recording services included by `DAEMON_SRC` | service/internal/candidate depending on narrower inventory |
| `core/http/src/BasicHttpClient.cpp` | public client candidate dependency |
| `core/http/src/SimpleHttpListener.cpp` | daemon listener dependency |
| `core/http/src/TestHttpServer.cpp` | test-support currently included; split-required |

Notes:

- `DAEMON_SRC` is an application composition aggregate, not a public library boundary.
- `TestHttpServer.cpp` appearing in daemon source composition remains a cleanup target before packaging.

---

## Package Candidate Summary

| Package candidate | Current status | Blocking audit |
| --- | --- | --- |
| `vdr-suite-runtime-core` | candidate | header audit |
| `vdr-suite-http-client` | candidate | header audit, listener/test exclusion |
| `vdr-suite-recording-core` | candidate | persistence/header audit |
| `vdr-suite-metadata-core` | candidate/split decision | overlap with recording core |
| `vdr-suite-dashboard-readmodel` | internal candidate | job/recording dependency split |
| `vdr-suite-recording-action-validation` | candidate/internal | remove REST parser and VDR cache dependency |
| `vdr-suite-vdr-domain` | candidate | header inventory |
| `vdr-suite-vdr-adapter-api` | candidate | interface/header audit |
| `vdr-suite-vdr-restfulapi-adapter` | concrete adapter candidate | transport and mapper dependency audit |
| `vdr-suite-vdr-service` | candidate | dependency audit |
| `vdr-suite-epg-core` | candidate | dependency audit |
| `vdr-suite-searchtimer-readonly` | candidate | separate mutation dependencies |
| `vdr-suite-snapshot-read` | candidate/internal | cache ownership audit |
| `vdr-suite-rest-api-contracts` | docs/schema candidate | endpoint inventory |
| `vdr-suite-rest-implementation` | internal/application API layer | route/controller split |
| `vdr-suite-daemon` | application package | test-support exclusion |
| `vdr-suite-test-support-http` | test support | separate from production deps |
| `vdr-suite-test-support-vdr` | test support | separate from production deps |
| `vdr-suite-tools` | tooling | destructive helper labelling |

---

## Immediate Split Targets

The first concrete packaging blockers are:

1. Split `ACTIONS_SRC` into recording action core, REST parser and VDR cache dependency groups.
2. Split `VDR_SRC` into the groups planned in Phase 56.22.
3. Keep HTTP client/listener/test-support groups narrow and migrate targets away from compatibility aggregates.
4. Split `REST_ROUTER_SRC` into controller groups and service dependencies.
5. Remove `TestHttpServer.cpp` from production daemon composition before package promotion.
6. Add header inventory for public package candidates.

---

## Verification

Required local verification:

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

No compile target is required because this phase documents source inventory only.

---

## Back

- [Back to Development Index](index.md)
