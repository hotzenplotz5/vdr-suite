# ADR-0013: Permission Model

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

ADR-0012 introduced Source capabilities.

The core platform model now separates:

```text
Actor
        ↓
Permission Layer
        ↓
Library
        ↓
Content
        ↓
Source
        ↓
Capability Layer
        ↓
Backend Adapter
        ↓
VDR
```

The next required architecture decision is how permissions should be understood.

VDR-Suite must support future scenarios such as:

- local users
- remote users
- remote VDR-Suite instances
- multiple VDR sources
- selected recording folders
- selected channel groups
- Live TV sharing
- timer creation and modification
- destructive recording operations

These scenarios require a permission model that is separate from technical capabilities.

## Decision

VDR-Suite will treat permissions as a future first-class architecture concept.

A permission describes whether an Actor is allowed to perform an operation.

A permission does not describe whether a Source technically supports the operation.

That is the role of capabilities.

Core distinction:

```text
Capability:
        Can the Source technically do it?

Permission:
        May the Actor do it?
```

An operation should eventually require both:

```text
operation allowed = source has capability AND actor has permission
```

## Actor

An Actor is anything that wants to access VDR-Suite functionality.

Possible future actors:

- local user
- remote user
- remote VDR-Suite instance
- frontend client
- plugin bridge
- API client

Actor is intentionally broader than User.

This is important because federation requires one VDR-Suite instance to act as a client of another VDR-Suite instance.

## Permission Examples

Possible future permissions:

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

admin.manage_permissions
```

These names are architectural examples, not a final API contract.

## Scoped Permissions

Permissions should eventually be scoping-capable.

Examples:

```text
source.view:house-a
recordings.view:house-a
recordings.folder:children
livetv.channel_group:public
backend.access:local-vdr
```

This enables configurations such as:

```text
Remote Suite B:
        may view selected recordings from House A
        may not view Live TV
        may not create timers

Guest User:
        may stream recordings
        may not delete recordings
        may not create timers

Admin User:
        may manage timers
        may manage permissions
```

## Permission and Federation

Federation requires permissions to apply not only to users, but also to remote instances.

Example:

```text
Actor:
        House B VDR-Suite

Source:
        House A VDR

Requested operation:
        recordings.view

Capability:
        available

Permission:
        granted

Result:
        allowed
```

Another example:

```text
Actor:
        House B VDR-Suite

Source:
        House A VDR

Requested operation:
        timers.create

Capability:
        available

Permission:
        denied

Result:
        rejected
```

This prevents remote access from becoming all-or-nothing.

## Permission and Destructive Operations

Destructive operations require special care.

Examples:

- delete recording
- move recording
- rename recording
- modify timer
- delete timer
- remote control / OSD actions

Future implementations should avoid exposing destructive operations remotely until permission checks are explicit and tested.

## Permission and Frontends

Frontends may eventually use permission and capability information to hide unavailable actions.

Examples:

- hide Delete if not permitted
- hide Timer creation if not permitted or not supported
- hide Live TV if not permitted or not supported

However, frontend behavior must never be the security boundary.

The backend must always enforce permissions.

## Permission and Plugin Policy

A future VDR-Suite plugin or bridge may need a policy configuration.

Possible local policy questions:

- Which remote VDR-Suite instances may connect?
- Which users may authenticate?
- Which recording folders are visible?
- Is Live TV allowed?
- Which channel groups are visible?
- Are timers visible?
- May timers be created?
- May timers be modified or deleted?
- May recordings be moved, renamed or deleted?

This implies that plugin-side integration should not be treated as an unrestricted transport pipe.

It may need a policy boundary.

## Consequences

Benefits:

- permissions stay separate from technical capabilities
- federation can be controlled safely
- destructive operations can be protected
- remote VDR-Suite access can be granular
- future frontends can adapt to allowed actions
- VDR-centered operations remain explicit

Trade-offs:

- additional architecture concept
- more complex authorization paths
- future tests must cover capability and permission combinations
- destructive operations require careful policy design
- federation cannot be implemented as simple blind trust

## Non-Goals

This ADR does not implement:

- Actor domain object
- Permission domain object
- user database
- authentication
- authorization
- role model
- federation protocol
- permission storage
- REST API output
- frontend permission handling

This ADR only records the architecture role of permissions.

## Follow-Up Work

Future phases may introduce:

- Actor model ADR
- Role model ADR
- permission value object
- scoped permission model
- source-aware permission checks
- permission-aware REST responses
- plugin policy boundary
- federation trust model

The next implementation-oriented step should not expose destructive remote operations before capability and permission checks exist.
---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
