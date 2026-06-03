# VDR-Suite Core Platform Model

## Purpose

This document summarizes the emerging long-term core platform architecture of VDR-Suite.

It connects the architecture decisions from:

- ADR-0010: Library First VDR Architecture
- ADR-0011: VDR Source Model
- ADR-0012: Source Capability Model

The goal is to keep the architecture VDR-centered while preparing VDR-Suite for multiple VDR instances, remote VDR-Suite federation, permissions and future frontends.

---

## Core Principle

VDR remains the primary backend domain.

VDR-Suite should not become a generic media server that merely happens to support VDR.

Instead, VDR-Suite should become a modern VDR-centered media platform.

The platform should expose VDR strengths through a modern architecture:

- recordings
- Live TV
- timers
- channels
- EPG
- metadata
- future OSD and remote-control functionality

---

## Conceptual Architecture

Long-term conceptual model:

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
VDR / Remote VDR-Suite / Archive
```

This model is not yet implemented.

It is a guiding architecture for future domain models, APIs, permissions and frontend design.

---

## Actor

An Actor is anything that wants to access VDR-Suite functionality.

Possible future actors:

- local user
- remote user
- remote VDR-Suite instance
- web frontend
- mobile frontend
- desktop frontend
- VDR plugin bridge
- API client

Actor is broader than user.

This distinction matters for federation.

Example:

```text
House B VDR-Suite
        acts as an Actor
        when accessing House A VDR-Suite
```

The Actor model is not implemented yet.

It is expected to become important before permissions and federation are implemented.

---

## Permission Layer

Permissions answer the question:

```text
Is this Actor allowed to do this?
```

Possible future permission examples:

```text
recordings.view
recordings.stream
recordings.delete

livetv.view

timers.view
timers.create
timers.modify
timers.delete

admin.manage_permissions
```

Permissions must be separate from capabilities.

A Source may technically support an operation, but the Actor may still not be allowed to use it.

---

## Library

A Library is the user-facing organization layer.

Possible future libraries:

- Recordings
- Live TV
- Timers
- Archives
- Series
- Movies
- Music
- Photos

The VDR-centered libraries are the priority:

- Recordings
- Live TV
- Timers
- EPG

The user should usually work with libraries and content first, not with server names first.

---

## Content

Content is what users interact with.

Examples:

- recording
- channel
- EPG event
- timer
- archived recording
- metadata item

A content item should eventually retain source identity.

Example:

```text
Content:
        Tatort recording

Library:
        Recordings

Source:
        House A VDR
```

The user may browse the Recordings library without manually choosing House A first.

But VDR-Suite must still know that the item belongs to House A.

---

## Source

A Source describes where content or capabilities originate.

Examples:

- Local VDR
- Remote VDR
- Remote VDR-Suite instance
- Recording archive
- NAS import
- future plugin bridge

Source is not the same as Backend.

A Source is visible to policy and user experience.

A Backend is the technical access mechanism.

Example:

```text
Source:
        House A VDR

Backend:
        RemoteVdrSuiteAdapter
```

Another example:

```text
Source:
        Local VDR

Backend:
        RestfulApiVdrAdapter
```

---

## Capability Layer

Capabilities answer the question:

```text
Can this Source technically do this?
```

Possible capabilities:

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
```

Capabilities are not permissions.

A capability means the Source supports the operation.

A permission means the Actor may use the operation.

Both are required:

```text
operation allowed = source has capability AND actor has permission
```

---

## Backend Adapter

The Backend Adapter describes how VDR-Suite accesses a Source.

Existing and planned adapter concepts already exist:

```text
IVdrAdapter
        ↑
   +--------------------+
   |        |           |
Mock   External   RestfulApi
```

Future backend adapters may include:

- RemoteVdrSuiteAdapter
- SvdrpVdrAdapter
- PluginBridgeVdrAdapter
- ArchiveAdapter

The Backend Adapter must remain hidden behind service boundaries.

Frontends should not depend directly on backend adapter details.

---

## VDR-Centered Federation

Federation means that one VDR-Suite instance may access another VDR-Suite instance.

Example:

```text
House A VDR-Suite
        ↔
House B VDR-Suite
```

This must remain VDR-centered.

The goal is not arbitrary server federation.

The goal is controlled VDR-Suite-to-VDR-Suite access to VDR-related content and capabilities.

Possible federation scenarios:

```text
House A:
        full local access

House B:
        may view selected recordings from House A
        may not view Live TV
        may not create timers

Admin user:
        may manage timers and permissions
```

Federation requires Actor, Permission, Source and Capability concepts.

---

## Operation Evaluation

A future operation may need to evaluate multiple questions:

```text
Who is acting?
        Actor

What do they want?
        Operation

Where is the content?
        Source

Can the Source do it?
        Capability

Is the Actor allowed?
        Permission

How is it executed?
        Backend Adapter
```

Example:

```text
Actor:
        Remote Suite B

Operation:
        timers.create

Source:
        House A VDR

Capability:
        timers.create available

Permission:
        denied

Result:
        operation rejected
```

Another example:

```text
Actor:
        Local admin user

Operation:
        recordings.stream

Source:
        Archive

Capability:
        recordings.stream available

Permission:
        granted

Result:
        operation allowed
```

---

## Architecture Boundaries

The following boundaries should remain separate:

```text
Actor identity
Permission decisions
Library organization
Source identity
Capability discovery
Backend adapter execution
```

These concepts should not be collapsed into one class or one service.

Keeping them separate allows VDR-Suite to grow without making the VDR backend, REST API or frontend architecture too rigid.

---

## Non-Goals

This document does not implement:

- Actor domain object
- Permission model
- Library domain object
- Source domain object
- Capability model
- Federation protocol
- database schema
- REST API changes
- frontend navigation changes

This document only summarizes the emerging platform architecture.

---

## Follow-Up Work

Likely follow-up architecture decisions:

- Actor Model
- Permission Model
- Federation Strategy
- Source Domain Object
- Capability Domain Object
- Library Domain Object

Likely future implementation phases:

- Source domain object
- Source capability value object
- permission value object
- source-aware recording summary
- source-aware API responses
