# ADR-0011: VDR Source Model Architecture

## Status

Accepted

## Context

VDR-Suite currently has a backend-oriented VDR architecture:

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

The existing adapter boundary is intentionally backend-neutral.

`IVdrAdapter` exposes VDR status, events, channels, timers and recordings without exposing transport details.

`VdrService` acts as a service boundary above the adapter.

ADR-0010 introduced the Library First VDR Architecture.

The user-facing model should not primarily expose servers or backend instances.

Users should work with content such as:

- recordings
- Live TV
- timers
- EPG
- channels
- archives
- metadata

However, future requirements require a stable Source concept:

- multiple VDR instances
- remote VDR-Suite instances
- archives
- NAS imports
- permission-aware remote access
- source-specific capabilities
- future federation

Therefore VDR-Suite needs a Source domain concept.

## Decision

VDR-Suite will introduce Source as a future core domain concept.

A Source represents the origin and policy context of content and capabilities.

A Source is not a backend adapter.

A Source is not a replacement for `VdrConfig`.

A Source is not a Library.

A Source is not a REST API concept.

A Source belongs above the backend adapter boundary.

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

For the current architecture this means:

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

A Source answers these questions:

- where does this content come from?
- which backend is responsible for this content?
- which source owns this backend-local item identity?
- which capabilities are available for this source?
- which permissions apply to actions against this source?

Examples:

```text
local-vdr
remote-vdr-house-a
remote-vdr-house-b
archive
nas-import
test-mock
```

## Non-Responsibilities

Source must not perform backend communication.

Source must not parse RESTfulAPI responses.

Source must not execute SVDRP commands.

Source must not know HTTP details.

Source must not expose frontend navigation.

Source must not enforce permissions directly.

Source must not become a duplicate of `VdrConfig`.

## Source and VdrConfig

`VdrConfig` remains a technical adapter configuration object.

Current `VdrConfig` responsibilities:

```text
enabled
mode
host
port
```

Future Source may reference or own a backend configuration, but Source itself is the domain identity and policy context.

Correct relationship:

```text
Source
        ↓
VdrConfig
        ↓
VdrAdapterFactory
        ↓
IVdrAdapter
```

Incorrect relationship:

```text
VdrConfig
        ↓
Source
```

Incorrect relationship:

```text
IVdrAdapter
        ↓
Source
```

## Source and Content Identity

Backend-local IDs are not globally safe.

In a future multi-VDR system, two different VDR instances may return the same recording ID, timer ID, channel ID or event ID.

Therefore global identity must later be source-aware.

Future identity rule:

```text
globalContentIdentity = sourceId + backendLocalId
```

This ADR does not implement that rule yet.

It only establishes it as an architectural requirement.

## Source and Library

A Library is a user-facing content view.

A Source is a backend-origin and policy context.

Example:

```text
Recordings Library
        ├── local-vdr
        ├── remote-vdr-house-a
        ├── remote-vdr-house-b
        ├── archive
        └── nas-import
```

The user should browse recordings through the library.

The system must still preserve the source behind every item.

## Source and Capability

Capabilities describe what a Source can technically and politically provide.

Future examples:

```text
recordings.read
recordings.rename
recordings.delete
timers.read
timers.create
timers.modify
timers.delete
epg.read
channels.read
live-tv.stream
remote-control.execute
```

Capabilities must be evaluated above the adapter layer.

An adapter may technically support an operation, but the Source policy may still deny it.

## Source and Permission

Permissions must be evaluated against Actor, Source and Capability.

Future example:

```text
Actor: Holger
Source: remote-vdr-house-b
Capability: recordings.delete
Result: denied
```

This keeps permission logic outside low-level backend adapters.

## Scope

This ADR introduces Source as an architecture concept only.

In scope:

- define Source responsibility
- define Source placement
- define relationship to VdrConfig
- define relationship to IVdrAdapter
- define future identity rule
- define future capability and permission relationship

Out of scope:

- Source C++ class
- Source repository
- database schema
- REST endpoint
- frontend navigation
- permission enforcement
- federation protocol
- source discovery
- remote authentication
- synchronization

## Consequences

Benefits:

- prepares Multi-VDR without rewriting the adapter boundary
- keeps VDR as the primary backend
- avoids exposing backend topology directly to frontends
- creates a future location for capability and permission policy
- prevents destructive actions from losing source context
- supports future federation without changing current REST API

Trade-offs:

- introduces another domain concept
- requires future source-aware identity handling
- requires care to avoid duplicating `VdrConfig`
- requires later migration of content models to carry source context

## Architectural Rules

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

## Non-Goals

This ADR does not implement Source.

This ADR does not introduce:

- Source C++ classes
- Source repositories
- Source services
- Source REST controllers
- Source JSON serializers
- database schema changes
- frontend navigation changes
- permission enforcement
- federation protocol
- remote authentication
- source discovery
- synchronization

## Follow-Up Work

Future phases may introduce:

- minimal Source domain object
- source type constants
- source identity rules
- source-aware VDR content identity
- source capability model
- permission model integration
- multi-VDR source registry
- remote VDR-Suite source strategy
- federation strategy

The next implementation phase should begin with a minimal Source domain object only after this architecture decision is accepted.
