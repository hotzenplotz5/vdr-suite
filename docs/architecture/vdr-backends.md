# VDR Backend Architecture

## Overview

```text
RuntimeConfig
        │
        ▼
VdrConfig
        │
        ▼
VdrAdapterFactory
        │
        ▼
IVdrAdapter
    ├── MockVdrAdapter
    ├── ExternalVdrAdapter
    ├── RestfulApiVdrAdapter
    └── SvdrpVdrAdapter (future)
```

## Design Goals

- Backend independence
- Testability
- Future extensibility
- Frontend neutrality

## MockVdrAdapter

Purpose:

- Testing
- Development
- CI

No external VDR required.

## ExternalVdrAdapter

Purpose:

- Generic external VDR integration

Acts as the foundation for future communication mechanisms.

## RestfulApiVdrAdapter

Implemented backend.

Responsibilities:

- Communicate with vdr-plugin-restfulapi through IHttpClient
- Convert RESTfulAPI responses into backend-neutral VDR domain objects
- Hide HTTP implementation details
- Keep RESTfulAPI JSON outside service and frontend layers

Implemented mappings:

- /info.json -> VdrStatus
- /events.json -> VdrEvent
- /channels.json -> VdrChannel
- /recordings.json -> VdrRecording
- /timers.json -> VdrTimer

Implemented mappers:

- RestfulApiStatusMapper
- RestfulApiEventMapper
- RestfulApiChannelMapper
- RestfulApiRecordingMapper
- RestfulApiTimerMapper

The rest of the application must not know whether RESTfulAPI is used.

## SvdrpVdrAdapter

Future backend.

SVDRP is a VDR control and compatibility protocol. It is useful for controlling VDR and for accessing selected VDR state, but it is not the primary internal API of VDR-Suite.

Responsibilities:

- Communicate via SVDRP
- Convert protocol responses into backend-neutral VDR domain objects
- Hide SVDRP command names, response codes and line formats behind the adapter boundary
- Provide control-oriented capabilities only where they are explicitly modeled

Appropriate future uses:

- status and availability checks
- channel switching
- timer read and write operations
- selected recording operations
- remote control compatibility
- OSD or legacy control fallbacks
- plugin command fallbacks where no better adapter exists

SVDRP must not leak into:

- service layer code
- REST API controllers
- frontend code
- dashboard logic
- snapshot domain objects

Destructive SVDRP commands must not be triggered directly from frontend actions. Examples include deleting timers, deleting recordings, moving recordings or simulating remote control sequences that have destructive effects.

Such operations must go through the VDR-Suite action, validation and permission layers before any future SVDRP adapter executes them.

## RESTfulAPI vs SVDRP

RESTfulAPI and SVDRP have different architectural roles.

RESTfulAPI is currently the preferred read-oriented integration path for structured VDR data because it exposes JSON endpoints that can be mapped into VDR-Suite domain objects.

SVDRP is a future control-oriented adapter path. It is especially relevant for operations that are part of VDR's native control model, such as timers, channel switching, remote key input and plugin commands.

Both mechanisms remain transport details below `IVdrAdapter`. Higher layers must depend on domain operations and capabilities, not on whether a backend used RESTfulAPI, SVDRP, a plugin bridge or a mock implementation.

## Recording Operation Strategy

Recording operations are domain operations, not filesystem operations.

Move, delete, rename and other write-oriented recording actions must not be executed directly by frontend code, controller code or service-layer filesystem access.

All recording write operations must pass through:

- capability validation
- permission validation
- operation validation
- action boundaries
- backend adapter boundaries

VDR-native operations are preferred whenever they are available.

Preferred execution order:

1. VDR-native backend functionality
2. RESTfulAPI integration
3. SVDRP integration where appropriate
4. Filesystem or external tool backends

Filesystem-based tools may remain valuable execution backends for operations such as import, repair, shrink, cut, recovery or batch processing. However, such tools are privileged execution mechanisms and must not become the VDR-Suite recording domain model.

Higher layers must not depend on filesystem layouts, shell commands, recording directory structures or backend-specific implementation details.

## Plugin API Boundary

VDR plugins are valuable integration points, but they are not a stable VDR-Suite domain layer.

VDR plugins run as dynamic libraries inside the VDR process. They are loaded by VDR through plugin loading mechanisms such as `dlopen` and `dlsym`, and they execute in the same process context as the VDR core.

The VDR plugin API exposes extension points such as:

- `Initialize()`
- `Start()`
- `Stop()`
- `Housekeeping()`
- `MainThreadHook()`
- `Service()`
- `SVDRPHelpPages()`
- `SVDRPCommand()`

Plugin bridges are therefore privileged integration points. Plugin code is not isolated from VDR. A faulty or blocking plugin can block, destabilize or crash the VDR process.

Plugin commands are not public VDR-Suite APIs. Plugin SVDRP commands may be useful as backend-specific integration mechanisms, but they must not be exposed directly as stable Suite-level operations.

Plugin services must be mapped into backend-neutral VDR-Suite domain objects. `void*` service payloads, plugin ABI details, command names, hook semantics and other plugin-internal contracts must not leak above the adapter boundary.

A future `PluginBridgeVdrAdapter` may exist, but only behind `IVdrAdapter`. Higher layers must continue to depend on backend-neutral domain operations, action state and capabilities instead of raw plugin internals.

The VDR-Suite daemon remains outside the VDR process. Plugin bridges may expose capabilities, but not raw plugin internals.

Any plugin-based write or control operation must still pass through VDR-Suite capability, permission, validation and action boundaries before it reaches a plugin bridge.

## Remote Control and HITK

Remote control integration is not a general application state model.

Future HITK-style key injection may be useful for:

- legacy OSD operation
- compatibility with VDR workflows that only exist as remote key sequences
- diagnostics
- emergency fallback control

It must not become the basis for normal VDR-Suite UI state management.

Modern UI features should prefer explicit domain operations, snapshot reads, action state and backend capabilities over key-sequence simulation.

## Security Boundary

SVDRP access control in VDR is host-oriented. VDR-Suite must not treat SVDRP host allow rules as a user, role or permission model.

Authentication, authorization, scoped permissions and destructive-operation policy belong to VDR-Suite.

A future SVDRP adapter must therefore be treated as a privileged backend transport that is protected by VDR-Suite service boundaries.

## Future Expansion

Additional adapters may be added without changes to:

- Dashboard
- Daemon
- Recording services
- Metadata services
- Frontend applications

Only the factory and adapter implementation need to change.