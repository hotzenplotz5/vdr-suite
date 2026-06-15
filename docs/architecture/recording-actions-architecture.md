# Recording Actions Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document defines the architecture boundary for future recording actions.

Phase 30.0 does not implement destructive recording operations.
It defines the rules required before move, rename, delete, cut, repair or metadata refresh can be exposed through APIs or frontends.

## Core Rule

Recording actions must never be implemented as direct frontend-to-filesystem operations.

All recording actions must pass through explicit backend-aware service, validation, capability, permission and job boundaries.

## Existing Foundation

VDR-Suite already contains an early action and job foundation:

ActionService
-> RecordingAction
-> JobService
-> Job
-> JobRepository
-> Worker or Adapter boundary

This foundation is intentionally minimal.
It is not yet sufficient for real recording operations because it does not carry backend identity, action payloads, dry-run information, capability checks or permission decisions.

## Required Future Boundary

Future recording actions should use this conceptual chain:

REST/API request
-> RecordingActionRequest
-> Recording identity resolution
-> Validation / dry-run planning
-> Backend capability check
-> Permission check
-> RecordingActionPlan
-> Job boundary
-> Backend or adapter execution
-> Result / diagnostics / audit trail

The job boundary is mandatory for destructive or long-running operations.

## Backend Identity

Recording actions must be backend-aware.

Rules:

- a recording action must know which backend owns the target recording
- backend=default compatibility may be preserved for legacy single-backend behavior
- empty backend identity may only be interpreted through an explicit legacy/default rule
- multi-backend actions must not guess ownership from title, path or display labels
- cross-backend operations are out of scope until explicitly designed

## Destructive Actions

Destructive or state-changing actions include:

- move
- rename
- delete
- cut
- repair
- metadata refresh execution

These must require:

- explicit validation
- explicit action request
- explicit backend ownership
- capability check
- permission check
- job creation
- auditable result state

## Dry-Run and Validation

Every destructive action should support dry-run or validation before execution.

Dry-run must not mutate VDR, VDR-Suite metadata, the filesystem or Rectools state.

## Capability Boundary

Recording actions must not assume that every backend can perform every action.

Future capability names may include:

- recordings.action.move
- recordings.action.rename
- recordings.action.delete
- recordings.action.cut
- recordings.action.repair
- recordings.action.metadata.refresh
- recordings.action.dry-run

## Permission Boundary

Recording actions must be permission-ready.

The architecture must allow future policies such as:

- view-only backend
- read-only backend
- admin-only delete
- operator-only repair
- metadata-only permission
- per-backend permissions
- per-client permissions

## Rectools Boundary

VDR-Rectools remains an external execution system.

Rules:

- no controller should call Rectools directly
- no frontend should call Rectools directly
- Rectools execution should remain behind an adapter or job boundary
- Rectools failures must be represented as job/action results
- placeholder execution must not be mistaken for production execution

## Recording Domain Separation

VDR recordings and VDR-Suite metadata recordings are related but not identical.

Rules:

- VDR remains the source of truth for real recordings
- VDR-Suite metadata remains complementary
- recording action target identity must come from the backend-aware VDR recording model
- metadata refresh may update VDR-Suite metadata, but must not redefine VDR ownership
- local metadata IDs must not be used as the only identity for real backend actions

## API Boundary

Future REST APIs should expose action intent, not filesystem operations.

Possible future endpoints may include:

- POST /api/vdr/recordings/actions/validate
- POST /api/vdr/recordings/actions
- GET /api/vdr/recordings/actions/{actionId}
- GET /api/jobs/{jobId}

Exact routes remain future work.

## Phase 30.1 Request Model

Phase 30.1 introduces the first backend-aware recording action request domain model.

The request model is intentionally not an execution model.
It describes the requested action intent and keeps execution behind later validation, capability, permission and job boundaries.

The request model carries:

- backend identity
- backend-owned recording identity
- action type
- dry-run flag
- action parameters

The default request behavior is dry-run oriented.
Real execution remains out of scope until a later phase defines validation results, action plans and job payload boundaries.

## Phase 30.2 Validation Model

Phase 30.2 introduces the first recording action validation result domain model.

The validation model is intentionally not an execution model.
It represents validation and dry-run results before a destructive action can create a job or reach a backend adapter.

The validation result carries:

- validity state
- dry-run state
- whether a job would be created
- backend identity
- backend-owned recording identity
- required backend capabilities
- required permissions
- warnings
- errors

The model is designed for later validation services, capability checks, permission checks and job planning.
It does not execute actions and does not mutate recordings, metadata, filesystems or Rectools state.

## Phase 30.3 Action Plan Model

Phase 30.3 introduces the first recording action plan domain model.

The action plan model is intentionally not an execution model.
It represents the planned result after request modeling and validation, but before job payload creation or backend adapter execution.

The action plan carries:

- backend identity
- backend-owned recording identity
- action type
- dry-run state
- whether execution is allowed
- whether a later job should be created
- planned job type
- required backend capabilities
- required permissions
- warnings

The model prepares the later job payload boundary without mutating recordings, metadata, filesystems or Rectools state.

## Phase 30.4 Job Payload Model

Phase 30.4 introduces the first recording action job payload domain model.

The job payload model is intentionally not an execution model and not a persisted job.
It represents the data that a later job boundary can receive after request modeling, validation and action planning.

The job payload carries:

- backend identity
- backend-owned recording identity
- action type
- job type
- dry-run state
- action parameters
- required backend capabilities
- required permissions
- warnings

The model prepares the later job creation boundary without mutating recordings, metadata, filesystems or Rectools state.

## Phase 30.5 Capability Requirements Model

Phase 30.5 introduces the first recording action capability requirements domain model.

The capability requirements model is intentionally not an evaluator and not an execution model.
It describes what a backend must support before an action can be considered executable.

The capability requirements model carries:

- action type
- required backend capabilities
- whether write access is required
- whether dry-run support is required

The model prepares later capability evaluation without querying backends, mutating recordings, changing metadata, touching filesystems or calling Rectools.

## Phase 32.7 RestfulAPI Backend Endpoint Configuration

Phase 32.7 introduces a small recording-action-specific RESTfulAPI backend endpoint configuration.

The configuration is intentionally separate from `VdrConfig`.
It carries only the backend execution endpoint identity needed by the recording-action RESTfulAPI backend executor adapter:

- backend identity
- backend host
- backend port
- backend base path

The configuration must describe the VDR RESTfulAPI backend endpoint, not the VDR-Suite daemon endpoint.
It must not hardcode either the VDR-Suite daemon port or a specific RESTfulAPI backend port.

The adapter remains a foundation boundary in this phase.
It does not execute move, rename or delete HTTP requests yet.
Those request mappings remain later phases.


## Phase 32.8 RestfulAPI Move Request Mapping

Phase 32.8 introduces the first HTTP request mapping boundary for recording move actions.

The mapping converts a `RecordingActionJobPayload` into an `HttpRequest` without executing the request.
It remains a unit-test-only boundary and does not contact a real VDR backend.

The move request mapping carries:

- backend-owned recording identity
- dry-run state
- target path from action parameters
- JSON request body
- JSON request headers
- a RESTfulAPI action path below the configured backend base path

The mapping must not hardcode the VDR-Suite daemon port.
It must also not hardcode a concrete RESTfulAPI backend port.
Host and port remain part of `RestfulApiRecordingActionBackendConfig` and are not embedded into the relative `HttpRequest` path.

This phase does not perform a real move.
It only defines the request object that later adapter execution phases can send through `IHttpClient`.
\n
## Phase 32.9 RestfulAPI Rename Request Mapping

Phase 32.9 adds the recording rename request mapping to the existing RESTfulAPI recording action request builder.

The mapping converts a `RecordingActionJobPayload` into an `HttpRequest` without executing the request.
It follows the same boundary as the move mapping and remains safe for unit tests.

The rename request mapping carries:

- backend-owned recording identity
- dry-run state
- new recording name from action parameters
- JSON request body
- JSON request headers
- a RESTfulAPI action path below the configured backend base path

The mapping must not hardcode the VDR-Suite daemon port or any concrete RESTfulAPI backend port.
The request remains a relative backend path and is not sent to a real VDR in this phase.

This phase does not perform a real rename.
It only defines the request object that later adapter execution phases can send through `IHttpClient`.
\n
## Phase 33.0 RestfulAPI Delete Request Mapping

Phase 33.0 adds the recording delete request mapping to the existing RESTfulAPI recording action request builder.

The mapping converts a `RecordingActionJobPayload` into an `HttpRequest` without executing the request.
It follows the same request-object boundary as move and rename.

The delete request mapping carries:

- backend-owned recording identity
- dry-run state
- JSON request body
- JSON request headers
- a RESTfulAPI action path below the configured backend base path

The mapping must not hardcode the VDR-Suite daemon port or any concrete RESTfulAPI backend port.
The request remains a relative backend path and is not sent to a real VDR in this phase.

This phase does not perform a real delete.
It only defines the request object that later adapter execution phases can send through `IHttpClient`.
\n
## Phase 33.1 RestfulAPI HTTP Execution Boundary

Phase 33.1 connects the RESTfulAPI recording action backend executor adapter to the request builder and `IHttpClient`.

The adapter can now map supported recording action payloads to `HttpRequest` objects and execute them through the injected HTTP client boundary.

Supported action mappings in this phase:

- move
- rename
- delete

The execution boundary remains unit-test-only.
Tests use a fake `IHttpClient` and do not contact a real VDR backend.

A successful 2xx HTTP response is mapped to a successful `RecordingActionExecutionResult`.
Unsupported action types are rejected before any HTTP request is sent.

This phase does not validate real RESTfulAPI endpoint semantics and does not mutate recordings.
Real VDR validation remains future work.
\n
## Phase 33.2 RestfulAPI HTTP Failure Mapping

Phase 33.2 refines the RESTfulAPI recording action HTTP execution boundary by mapping non-2xx HTTP responses into explicit action execution errors.

The adapter still uses an injected fake `IHttpClient` in tests and does not contact a real VDR backend.

Failure mapping rules:

- non-2xx HTTP responses produce `success=false`
- the result message identifies the request as failed
- the first error records the HTTP status code
- a non-empty backend response body is preserved as an additional error detail
- empty response bodies do not create an extra error entry

This phase does not interpret real RESTfulAPI error schemas.
It only preserves the backend status and response body at the action execution boundary.
\n
## Phase 33.3 RestfulAPI Unsupported Action Guard

Phase 33.3 makes the RESTfulAPI backend executor adapter unsupported-action guard explicit.

The adapter only supports recording action types that have request mappings:

- move
- rename
- delete

Unsupported actions are rejected before any HTTP request is built or sent.

Guard behavior:

- no `IHttpClient` call occurs
- the result keeps the original action type
- the result keeps backend identity
- the result keeps recording identity
- the result reports `success=false`
- the result contains a clear unsupported-action message and error

This preserves the backend boundary and prevents unmapped actions from accidentally reaching a RESTfulAPI backend.
\n
## Phase 33.4 RestfulAPI Payload Parameter Validation

Phase 33.4 adds payload parameter validation to the RESTfulAPI recording action backend executor adapter.

Validation happens after unsupported-action guarding and before request building or HTTP execution.

Validation rules:

- every supported action requires a backend-owned recording identity
- move requires `targetPath`
- rename requires `newName`
- delete requires only the recording identity

Invalid payloads are rejected before any `HttpRequest` is built or sent through `IHttpClient`.

This keeps malformed move, rename and delete actions from reaching the RESTfulAPI backend.
\n
## Phase 33.5 RestfulAPI Dry Run Enforcement

Phase 33.5 adds a dry-run enforcement guard to the RESTfulAPI recording action backend executor adapter.

The guard runs after payload validation and before request building or HTTP execution.

Rules:

- dry-run payloads may reach the request builder and fake HTTP client boundary
- non-dry-run payloads are rejected before any `HttpRequest` is built or sent
- move, rename and delete real execution remain disabled in this phase

This preserves the safety boundary while request mapping and HTTP result handling are still being built.
Real write execution requires a later explicit enablement boundary and backend permission strategy.
\n
## Phase 33.6 RestfulAPI Execution Enablement Flag

Phase 33.6 adds an explicit execution enablement flag to the RESTfulAPI recording action backend configuration.

The default remains safe:

- `allowExecution=false`

Execution policy:

- dry-run payloads may reach the request builder and fake HTTP client boundary
- non-dry-run payloads are blocked while `allowExecution=false`
- non-dry-run payloads may reach the HTTP boundary only when `allowExecution=true`

This phase still uses fake `IHttpClient` tests only.
It does not validate real VDR RESTfulAPI behavior and does not modify real recordings.

The flag makes future write enablement an explicit backend configuration decision instead of an accidental code-path change.
\n
## Phase 33.7 RestfulAPI Read-Only Backend Guard

Phase 33.7 adds a read-only backend guard to the RESTfulAPI recording action backend executor adapter.

The guard is controlled through the backend action configuration:

- `readOnly=false` keeps the normal execution policy
- `readOnly=true` blocks move, rename and delete execution before request building or HTTP execution

The guard runs after payload validation and before execution enablement.

Read-only backend behavior:

- no `HttpRequest` is built
- no `IHttpClient` call occurs
- the result keeps backend identity and recording identity
- the result reports `success=false`
- the result contains a clear read-only backend error

This supports future multi-backend deployments where one VDR backend can be view-only while another backend allows recording actions.
\n
## Phase 33.8 RestfulAPI Backend Config Documentation

Phase 33.8 documents the RESTfulAPI recording action backend configuration boundary.

The configuration is intentionally separate from `VdrConfig`.
It describes the backend action execution endpoint and execution policy for recording actions.

Fields:

- `backendId`
  - stable backend identity used by the recording action pipeline
  - must match the backend owning the recording
- `host`
  - RESTfulAPI backend host
  - must refer to the VDR RESTfulAPI backend, not the VDR-Suite daemon
- `port`
  - RESTfulAPI backend port
  - must not be confused with the VDR-Suite daemon API port
- `basePath`
  - optional RESTfulAPI backend base path
  - request mappings build relative action paths below this base path
- `allowExecution`
  - default: `false`
  - controls whether non-dry-run recording actions may reach the HTTP boundary
- `readOnly`
  - default: `false`
  - blocks move, rename and delete even when execution is otherwise enabled

Safety rules:

- `readOnly=true` always blocks write actions before HTTP execution
- `allowExecution=false` blocks non-dry-run actions before HTTP execution
- dry-run actions may reach the fake HTTP boundary during unit tests
- no real VDR write validation is performed by this configuration phase

Port separation rule:

- VDR-Suite daemon/API port: user/frontend/API access to VDR-Suite
- RESTfulAPI backend port: VDR-Suite access to a VDR backend

The recording action backend config must describe the backend RESTfulAPI endpoint.
It must not hardcode the VDR-Suite daemon port or a single backend port.
\n
## Phase 33.9 Real RESTfulAPI Endpoint Discovery

Phase 33.9 records the first discovery results against a real VDR RESTfulAPI backend.

Observed backend:

- RESTfulAPI reachable on the backend port
- `recordings.json` returns real recording data
- `OPTIONS` responses are generic and must not be used as endpoint proof

Observed action endpoint results:

- `/recordings/delete.json`
  - service exists
  - `GET` is rejected with: `Only POST and DELETE methods are supported by the /recordings/delete service.`
  - this proves the delete service exists, but no destructive request was executed
- `/recordings/actions/move`
  - not proven as a real action endpoint
  - `GET` reports missing format selection
- `/recordings/actions/rename`
  - not proven as a real action endpoint
  - `GET` reports missing format selection
- `/recordings/actions/delete`
  - not proven as the real delete action endpoint
  - `GET` reports missing format selection

Discovery conclusions:

- the current VDR-Suite placeholder action paths are not yet aligned with the real RESTfulAPI delete service
- real delete endpoint alignment must use `/recordings/delete.json`
- real move and rename endpoints are not yet proven
- no real move, rename or delete request was executed
- no real recording was modified

Next implementation implication:

The delete request mapping should be aligned first, because a real delete service has been observed.
Move and rename must remain discovery targets until real services are proven or a backend extension path is selected.

## Non-Goals

Phase 30.0 does not:

- move files
- delete recordings
- rename recordings
- cut recordings
- repair recordings
- call Rectools for real execution
- change /api/vdr/recordings
- implement frontend action buttons
- implement final authentication or authorization

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)


## Phase 30.6 Capability Evaluation Model

Phase 30.6 introduces the recording action capability evaluation model.

The evaluation result describes whether the capability requirements of a recording action are satisfied.

The model carries:

- action type
- evaluation result
- available capabilities
- missing capabilities

The model does not perform evaluation itself and does not call backends.



## Phase 30.7 Permission Evaluation Model

Phase 30.7 introduces the recording action permission evaluation model.

The permission evaluation result describes whether a caller has the required permissions to execute a recording action.

The model carries:

- action type
- evaluation result
- granted permissions
- missing permissions

The model does not evaluate permissions and does not execute actions.



## Phase 30.8 Execution Readiness Model

Phase 30.8 introduces the recording action execution readiness model.

The readiness result aggregates prior validation, capability and permission decisions.

The model carries:

- action type
- readiness result
- passed readiness checks
- failed readiness checks

The model does not execute actions and does not call backend services.



## Phase 30.9 Execution Boundary Model

Phase 30.9 introduces the first backend-neutral recording action execution boundary model.

The execution boundary separates recording action domain decisions from backend-specific execution.

The boundary carries:

- action type
- backend identity
- backend-owned recording identity
- execution success state
- execution message
- warnings
- errors

The executor interface accepts a recording action job payload and returns an execution result.

The boundary intentionally does not bind recording actions to RESTfulAPI, SVDRP, Rectools, VDR internals or filesystem operations. Backend-specific executors remain future work.



## Phase 31.0 Executor Registry Foundation

Phase 31.0 introduces the recording action executor registry foundation.

The registry maps backend identities to backend-neutral recording action executors.

The registry allows later phases to select an executor by backend id without coupling recording actions to RESTfulAPI, SVDRP, Rectools, VDR internals or filesystem operations.

The foundation supports:

- backend id based executor registration
- backend id based executor lookup
- missing executor handling

The registry does not dispatch actions and does not execute backend operations.



## Phase 31.1 Executor Registration Model

Phase 31.1 introduces a backend-neutral executor registration model.

The registration model separates executor metadata from registry implementation details.

The registration currently contains:

- backend identity
- executor instance

Future phases may extend registrations with:

- backend type
- capability metadata
- read-only state
- supported recording actions

without changing registry interfaces.



## Phase 31.2 Executor Lookup Result Model

Phase 31.2 introduces a backend-neutral executor lookup result model.

The lookup result replaces raw null-pointer based registry lookup with an explicit result object.

The result carries:

- lookup success state
- backend identity
- executor instance
- lookup message

This prepares later backend executor selection and dispatch phases without coupling recording actions to a specific backend implementation.



## Phase 31.3 Backend Executor Selection Model

Phase 31.3 introduces a backend-neutral executor selection result model.

The selection result separates executor lookup from executor selection.

The result carries:

- selection state
- backend identity
- selected executor
- selection reason

This prepares later dispatch phases where executor selection may consider backend type, capability metadata, read-only state and supported recording actions.

The model does not dispatch actions and does not execute backend operations.



## Phase 31.4 Dispatch Foundation

Phase 31.4 introduces a dispatch result model.

The dispatch layer sits between executor selection and executor execution.

Responsibilities:

- receive selected executor
- dispatch action
- collect execution result
- provide dispatch outcome

This remains backend-neutral and prepares future backend specific executors.


## Phase 31.5 Default Executor Resolution

Phase 31.5 introduces a backend-neutral default executor
resolution result model.

Resolution rules:

- explicit backend id -> exact executor resolution
- unknown backend id -> error
- empty backend id -> default executor resolution
- missing default executor -> error

No implicit fallback to another backend is allowed.

This protects multi-backend installations from accidental
cross-backend execution.


## Phase 31.6 Backend Capability Dispatch Rules

Phase 31.6 introduces backend-neutral capability dispatch rules.

Dispatch rules define which capability is required
for a recording action before dispatch may occur.

Examples:

- move -> moveRecording
- delete -> deleteRecording
- rename -> renameRecording

Capability sources remain backend specific.

Examples:

- RestfulAPI capabilities.json
- SVDRP capability provider
- Rectools capability provider

The dispatch layer consumes capabilities but does not
depend on a specific backend implementation.


## Phase 31.7 Backend Permission Dispatch Rules

Phase 31.7 introduces backend-neutral permission dispatch rules.

Permission rules define which permission is required
for a recording action before dispatch may occur.

Examples:

- move -> moveRecording
- delete -> deleteRecording
- rename -> renameRecording

Permission evaluation is independent from capability evaluation.

A backend may support an action technically while policy,
configuration or backend role prohibits execution.

Examples:

- full-access backend
- read-only backend
- maintenance backend

Dispatch requires both capability and permission approval.


## Phase 31.8 Executor Resolution Service

The executor resolution service combines:

- executor registry
- executor lookup
- backend selection
- default executor resolution

The caller requests an executor and receives
a backend-neutral selection result.

Future implementations may resolve:

- RestfulAPI executors
- SVDRP executors
- Rectools executors
- additional backend implementations

without changing caller behavior.


## Phase 31.9 Dispatch Service

Phase 31.9 introduces the backend-neutral recording action dispatch service.

The dispatch service receives a selected executor and a recording action job payload.

It returns a dispatch result that contains the backend executor execution result.

The service does not know whether the selected executor is backed by RestfulAPI, SVDRP, Rectools, native VDR integration or another backend.


## Phase 32.0 Backend Executor Adapter Foundation

Phase 32.0 introduces the backend-neutral recording action backend executor adapter boundary.

The adapter boundary extends the recording action executor interface with backend identity and backend type metadata.

The boundary prepares backend-specific executor implementations such as:

- RestfulAPI
- SVDRP
- Rectools
- native VDR integration
- mock/test backends

The recording action dispatch service remains backend-neutral.


## Phase 32.1 Mock Backend Executor Adapter

Phase 32.1 introduces a concrete mock backend executor adapter.

The mock adapter implements the backend executor adapter boundary and provides a deterministic backend-neutral test adapter.

The mock adapter prepares later real backend adapters such as RestfulAPI, SVDRP, Rectools and native VDR integration without coupling dispatch logic to those implementations.


## Phase 32.2 Backend Executor Adapter Registry

Phase 32.2 introduces a backend-neutral recording action backend executor adapter registry.

The adapter registry maps backend identities to backend executor adapters.

It allows dispatch infrastructure to locate concrete backend adapters such as mock, RestfulAPI, SVDRP, Rectools or native VDR adapters without depending on a specific backend implementation.


## Phase 32.3 Backend Executor Adapter Lookup

Phase 32.3 introduces a backend-neutral backend executor adapter lookup result model.

The lookup result replaces raw null-pointer adapter lookup with an explicit result object.

The result carries:

- lookup success state
- backend identity
- backend executor adapter
- lookup message

This prepares later backend adapter resolution and dispatch integration without coupling recording actions to a specific backend implementation.


## Phase 32.4 Backend Executor Adapter Resolution Service

Phase 32.4 introduces a backend-neutral backend executor adapter resolution service.

The resolution service consumes backend executor adapter lookup results and returns a resolved backend adapter result.

For Phase 32.4 the service preserves successful lookups and turns missing lookups into explicit resolution failures.

This keeps backend adapter resolution independent from concrete backend implementations such as RestfulAPI, SVDRP, Rectools or native VDR integration.


## Phase 32.5 Backend Executor Adapter Dispatch Integration

Phase 32.5 introduces backend executor adapter dispatch integration.

The adapter dispatch service receives a resolved backend executor adapter and a recording action job payload.

It dispatches the payload to the resolved backend adapter and returns a recording action dispatch result.

The service keeps recording action dispatch independent from concrete backend implementations such as Mock, RestfulAPI, SVDRP, Rectools or native VDR integration.


## Phase 32.6 RestfulAPI Backend Executor Adapter Foundation

Phase 32.6 introduces the RestfulAPI recording action backend executor adapter foundation.

The adapter follows the existing VDR RestfulAPI adapter boundary by accepting VdrConfig and IHttpClient.

This phase does not execute real recording actions yet.

It establishes backend identity, backend type and execution-result boundaries for later RestfulAPI move, rename and delete dispatch phases.
