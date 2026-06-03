# Phase 8.38 – Minimal Source Domain Object Gate

## Status

Architecture gate.

No implementation has been performed in this phase gate.

## Decision

Phase 8.38 may introduce a minimal backend-neutral `Source` domain object.

This is now architecturally allowed because the required architecture chain exists:

```text
ADR-0011: Source Model Architecture
ADR-0012: Source Capability Model
ADR-0013: Permission Model
```

ADR-0011 establishes that Source is a future core domain concept above the backend adapter boundary.

ADR-0012 establishes that capabilities belong to the Source architecture and must remain separate from permissions.

ADR-0013 establishes that permissions are Actor-based and must remain separate from Source capabilities.

## Required Boundary

The first Source implementation must be intentionally small.

It must not introduce persistence, REST API changes, frontend navigation, federation, capability enforcement or permission enforcement.

## Recommended First Source Object

The first `Source` domain object should model only stable identity and basic classification.

Recommended fields:

```text
id
name
type
enabled
backendMode
```

Meaning:

```text
id:
        stable source identity

name:
        human-readable source name

type:
        source category, for example local-vdr, remote-vdr-suite, archive, nas-import, test-mock

enabled:
        whether the source is active

backendMode:
        adapter/backend selection hint, aligned with VdrConfig mode but not replacing VdrConfig
```

## Explicitly Excluded Fields

The first Source object must not contain:

```text
host
port
credentials
permissions
capabilities
library membership
federation token
remote trust state
sync state
database id
REST URL
frontend route
```

Reason:

These fields either belong to `VdrConfig`, future capability objects, future permission objects, future federation objects, future persistence or future frontend state.

## Source vs VdrConfig

`Source` must not become a second `VdrConfig`.

Correct relationship:

```text
Source
        ↓ references / selects
VdrConfig or future backend config
        ↓
VdrAdapterFactory
        ↓
IVdrAdapter
```

Incorrect relationship:

```text
Source
        contains host, port, transport details, credentials and adapter internals
```

## Source vs Capability

Capabilities must not be embedded directly into the first Source class.

Reason:

Capability modeling is its own future step.

The first Source object should not predefine the final representation of capability sets.

Future relationship:

```text
Source
        ↓
SourceCapabilitySet
        ↓
Capability checks
```

## Source vs Permission

Permissions must not be embedded directly into the first Source class.

Reason:

Permissions apply to Actor + Source + Operation and may later be scoped.

Future relationship:

```text
Actor
        ↓
Permission
        ↓
Source
        ↓
Capability
```

## Source vs IVdrAdapter

`IVdrAdapter` must remain unaware of Source.

Adapters are technical backend boundaries.

Source belongs above adapters.

Do not add `getSource()` to `IVdrAdapter` in the first implementation.

Do not pass Source into every adapter method.

## Source and Content Identity

The first Source object may establish identity semantics, but it must not yet rewrite content objects.

Future rule:

```text
globalContentIdentity = sourceId + backendLocalId
```

Do not add `sourceId` to `VdrRecording`, `VdrTimer`, `VdrChannel` or `VdrEvent` in the first Source object phase unless a separate architecture step explicitly approves that migration.

## Recommended File Placement

Before implementation, the real repository structure must be checked again.

Based on the current architecture, the most likely location is:

```text
core/vdr/include/Source.h
```

or, if a more general core-domain area is introduced later:

```text
core/domain/include/Source.h
```

However, no new directory should be invented without checking the repository structure first.

Given the current repository has VDR-domain objects in `core/vdr/include`, the minimal first implementation should probably live near existing VDR domain objects unless a broader domain layer already exists.

## Recommended Tests

The first implementation should include minimal tests only:

```text
Source default construction
Source field assignment
Source enabled/disabled state
Source type string handling
Source backendMode string handling
```

No tests for:

```text
REST serialization
DB persistence
capability enforcement
permission enforcement
federation
remote discovery
adapter creation
```

## Recommended Phase 8.38 Scope

In scope:

* minimal Source domain object
* simple unit test
* Makefile test target integration if required by existing structure
* current-status update or separate status document

Out of scope:

* database schema
* REST API
* frontend
* permission checks
* capability checks
* federation
* source registry
* multi-source runtime wiring
* adapter refactoring
* VdrConfig refactoring

## Final Recommendation

Proceed with Phase 8.38 only as a minimal domain object phase.

Do not implement capability or permission logic in the same step.

Do not modify adapters in the same step.

Do not modify existing VDR content objects in the same step.

The purpose of Phase 8.38 should be to establish a stable Source identity object, not to integrate Source into runtime behavior yet.
