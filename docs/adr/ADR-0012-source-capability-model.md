# ADR-0012: Source Capability Model

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)

---

## Status

Accepted

## Context

ADR-0010 introduced the Library First VDR Architecture.

ADR-0011 introduced Source as a future first-class architecture concept.

A Source describes where content or VDR capabilities originate.

Examples:

- Local VDR
- Remote VDR-Suite instance
- Remote VDR
- Recording archive
- NAS import
- future plugin bridge

However, not every Source can do the same things.

A local VDR may support recordings, Live TV, timers, channels and EPG.

An archive may only support recordings.

A remote VDR-Suite instance may support only the capabilities that it exposes and allows.

VDR-Suite therefore needs a future capability model.

## Decision

VDR-Suite will treat Source capabilities as a future first-class architecture concept.

A capability describes what a Source can technically provide.

A capability does not describe whether a user is allowed to use it.

That distinction is important:

```text
Capability:
        The Source can do it.

Permission:
        The user or remote instance may do it.
```

Both are required for safe multi-VDR and federation architecture.

## Capability Examples

Possible future capabilities:

```text
recordings.view
recordings.stream
recordings.modify
recordings.delete

livetv.view

timers.view
timers.create
timers.modify
timers.delete

epg.view
epg.search

channels.view

metadata.view

osd.view
osd.remote_control
```

These names are architectural examples, not a final API contract.

## VDR-Centered Capability Model

The capability model must remain VDR-centered.

The first relevant capability groups should be:

- recordings
- Live TV
- timers
- EPG
- channels
- metadata
- OSD / remote control

Future non-VDR sources may expose only a subset.

Example:

```text
Local VDR Source:
        recordings.view
        recordings.stream
        livetv.view
        timers.view
        timers.create
        timers.modify
        epg.view
        channels.view

Archive Source:
        recordings.view
        recordings.stream

Remote VDR-Suite Source:
        capabilities depend on remote exposure and policy
```

## Capability vs Permission

Capabilities and permissions must not be mixed.

A Source may technically support timer creation.

But a user or remote instance may still not be allowed to create timers.

Example:

```text
Source capability:
        timers.create

User permission:
        not granted

Result:
        timer creation denied
```

Another example:

```text
Source capability:
        not available

User permission:
        granted

Result:
        operation impossible
```

The system should eventually require both:

```text
operation allowed = source has capability AND user has permission
```

## Capability and Federation

Federation makes capability handling mandatory.

A remote VDR-Suite instance may expose only selected functionality.

Examples:

```text
Remote Suite A:
        recordings.view
        recordings.stream

Remote Suite B:
        recordings.view
        livetv.view
        timers.view

Remote Suite C:
        recordings.view
        livetv.view
        timers.create
        timers.modify
```

A frontend should eventually be able to adapt to capabilities.

For example, if a Source does not expose `timers.create`, the frontend should not offer timer creation for that Source.

## Capability and UI Behavior

Capabilities should eventually help frontends decide which actions to show.

Examples:

- show Play button only when streaming is available
- show Timer button only when timer creation is available
- show Delete button only when recording deletion is technically supported and permitted
- show Live TV only when Live TV is available

The frontend must still rely on backend authorization for safety.

Capabilities are not a security boundary by themselves.

## Capability and Backend Adapters

Backend adapters may later report capabilities.

Example:

```text
IVdrAdapter
        ↓
VdrSourceCapabilities
```

or a future source-level service may aggregate backend capabilities.

This ADR does not define the final class names.

The important decision is that capabilities belong to the Source architecture, not to frontend assumptions.

## Consequences

Benefits:

- different Sources can be modeled safely
- frontends can adapt to available actions
- federation can expose limited functionality
- remote write operations become easier to control
- VDR-specific capabilities remain explicit
- permission design becomes cleaner

Trade-offs:

- additional architecture concept
- operations must check both capability and permission
- capability discovery may be needed for remote Sources
- test cases must eventually cover capability differences

## Non-Goals

This ADR does not implement:

- capability classes
- capability enums
- database schema
- REST API output
- permission checks
- federation protocol
- frontend behavior
- backend capability discovery

This ADR records the architecture role of Source capabilities only.

## Follow-Up Work

Future phases may introduce:

- Source capability value object
- VDR capability set
- source-aware capability service
- capability serialization for REST APIs
- frontend capability hints
- permission model ADR

The next architecture step should define the permission model and its relationship to Source capabilities.
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
