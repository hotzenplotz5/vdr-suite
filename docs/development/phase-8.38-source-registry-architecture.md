# Phase 8.38 – SourceRegistry Architecture

## Status

Architecture decision for future multi-source runtime support.

## Decision

VDR-Suite should treat SourceRegistry as a future core-domain concept, not merely as incidental runtime wiring.

The first implementation may later be in-memory only.

No persistence, REST API, frontend, federation or runtime multi-source wiring should be introduced in the first SourceRegistry step.

## Why SourceRegistry Is Needed

A single Source object is not enough for the long-term architecture.

Future VDR-Suite must support multiple origins:

* local VDR
* remote VDR
* remote VDR-Suite
* archive
* NAS import
* mock/test source

A registry is needed to answer:

* which sources exist?
* which source is local default?
* which source owns an item identity?
* which sources are enabled?
* which source types are available?
* which source should be selected for future adapter creation?

## Current Architecture

Current simplified runtime:

```text
DaemonRuntime
        ↓
VdrConfig
        ↓
VdrAdapterFactory
        ↓
IVdrAdapter
        ↓
VDR
```

This is still effectively single-source.

## Future Architecture

Future source-aware runtime:

```text
DaemonRuntime
        ↓
SourceRegistry
        ↓
Source
        ↓
VdrConfig or backend config
        ↓
VdrAdapterFactory
        ↓
IVdrAdapter
        ↓
VDR
```

## SourceRegistry Is Not A Database Repository

SourceRegistry should not initially be a persistence repository.

It should not imply database schema.

It should not be named SourceRepository unless persistence is actually introduced.

Recommended distinction:

* SourceRegistry: knows configured/available sources in memory.
* SourceRepository: future persistence access if database storage is added.

## SourceRegistry Is Not A Library

A Library is a content view.

A SourceRegistry is source inventory.

Incorrect relationship:

```text
Library owns Sources
```

Correct relationship:

```text
Library queries content across Sources known by SourceRegistry
```

A Source may appear in multiple future libraries.

## SourceRegistry Is Not An Adapter Factory

SourceRegistry must not create backend adapters directly in the first architecture.

Adapter creation remains the responsibility of VdrAdapterFactory or a future source-aware factory layer.

Correct separation:

* SourceRegistry knows sources.
* VdrAdapterFactory creates adapters.
* Source capability model describes available operations.
* Permission model controls actor access.

## Initial Registry Shape

The first SourceRegistry implementation should be small.

Possible future responsibilities:

* register Source
* list all Sources
* list enabled Sources
* find Source by id
* identify default local Source

Not included initially:

* database loading
* file config loading
* remote discovery
* federation sync
* permission checks
* capability checks
* adapter lifecycle ownership

## Source Identity Rule

SourceRegistry depends on stable Source ids.

Source ids must not be transient numeric database ids.

Recommended source ids are stable names such as:

* local-vdr
* house-a
* house-b
* archive-main
* nas-import
* mock

A future SourceId value object may be introduced later, but the first Source model can use string ids if the stability rule is documented and tested.

## Relationship To Federation

Federation should later exchange sources, not adapters.

Remote VDR-Suite instances should expose source information and capability information, not internal adapter implementation details.

Future federation relationship:

```text
Remote VDR-Suite
        ↓ exposes
Remote Sources
        ↓ represented locally as
SourceType::RemoteSuite or remote federated Sources
```

This keeps federation above adapter internals.

## Relationship To Capability And Permission

SourceRegistry does not decide whether an operation is possible or allowed.

Capability model answers whether a Source can provide an operation.

Permission model answers whether an Actor may use the operation.

Future rule:

```text
allowed = registry has source AND source has capability AND actor has permission
```

## Implementation Boundary

Do not implement SourceRegistry before Source and SourceType exist.

Do not make SourceRegistry persistent in its first phase.

Do not expose SourceRegistry through REST in its first phase.

Do not connect SourceRegistry to frontend navigation in its first phase.

Do not make SourceRegistry own adapters in its first phase.

## Recommended Phase Order

Recommended architecture-safe order:

1. SourceType decision
2. Minimal Source domain object
3. In-memory SourceRegistry
4. Source-aware runtime configuration
5. Source-aware adapter selection
6. Source-aware content identity
7. Capability integration
8. Permission integration
9. Federation strategy

## Final Recommendation

SourceRegistry is necessary for future multi-VDR and federation architecture.

However, the first implementation should remain purely in-memory and domain-level.

The immediate next code phase should still start with SourceType and minimal Source before SourceRegistry is implemented.
