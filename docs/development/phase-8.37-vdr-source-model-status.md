# Phase 8.37 – VDR Source Model Architecture Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

## Status

Completed as architecture documentation.

## Commit Context

This status file documents the Phase 8.37 architecture step without modifying source code, database schema, REST API or frontend navigation.

## Added Documentation

* `docs/adr/ADR-0011-vdr-source-model-architecture.md`

## Architecture Decision

Phase 8.37 introduces `Source` as a future core domain concept above the backend adapter boundary.

Target conceptual model:

```text
Actor
        ↓
Permission
        ↓
Library
        ↓
Content
        ↓
Source
        ↓
Capability
        ↓
Backend Adapter
        ↓
VDR
```

Current technical VDR adapter relationship remains:

```text
RuntimeConfig
        ↓
VdrConfig
        ↓
VdrAdapterFactory
        ↓
IVdrAdapter
        ↓
VDR backend
```

Future Source relationship:

```text
Library / Content Layer
        ↓
Source
        ↓
VdrConfig
        ↓
VdrAdapterFactory
        ↓
IVdrAdapter
        ↓
VDR backend
```

## Source Responsibility

A Source represents the origin and policy context of content and capabilities.

A Source answers:

* where content comes from
* which backend owns backend-local identity
* which capabilities are available for that origin
* which permissions apply to actions against that origin

Example future sources:

* local-vdr
* remote-vdr-house-a
* remote-vdr-house-b
* archive
* nas-import
* test-mock

## Explicit Non-Goals

Phase 8.37 does not introduce:

* Source C++ classes
* Source repositories
* Source services
* Source REST controllers
* Source JSON serializers
* database schema changes
* frontend navigation changes
* permission enforcement
* federation protocol
* remote authentication
* source discovery
* synchronization

## Architectural Rules Established

1. Source sits above backend adapters.
2. Source does not replace `VdrConfig`.
3. Source does not replace `IVdrAdapter`.
4. Source must not leak transport details.
5. Content may later reference Source.
6. Backend-local IDs must not be treated as globally unique.
7. Destructive operations must later require source context.
8. Permissions must be evaluated above the adapter layer.
9. Libraries aggregate content; Sources provide origin.
10. VDR remains the primary backend domain.

## Future Identity Rule

Backend-local IDs are not globally safe.

Future global content identity must become source-aware:

```text
globalContentIdentity = sourceId + backendLocalId
```

## Follow-Up Candidate

Phase 8.38 may introduce a minimal Source domain object, but only after ADR-0011 is accepted as the architecture foundation.

Expected Phase 8.38 boundaries:

* minimal backend-neutral Source domain object
* no persistence
* no REST API
* no frontend
* no permission enforcement
* no federation implementation

## Current-Status Integration Note

`docs/development/current-status.md` still needs to be updated to reference Phase 8.37 and ADR-0011.

Because GitHub connector file replacement requires complete-file writes and large file reads may be truncated by the connector response limit, this separate status file preserves the Phase 8.37 status without risking accidental loss of existing `current-status.md` content.
---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
