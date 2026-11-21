# Media Platform Comparison and VDR-Suite Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document compares established media platform architectures and derives early architecture requirements for VDR-Suite.

The goal is not to copy Plex, Jellyfin, Kodi, Tvheadend or Emby.

The goal is to identify architectural patterns that VDR-Suite should consider before the data model, REST API, plugin model and frontend architecture become too rigid.

---

## Core Observation

Successful media platforms are not only defined by playback features.

They are defined by how they model:

- libraries
- sources
- users
- permissions
- devices
- remote access
- backend capabilities

For VDR-Suite, the most important strategic question is:

```text
Does the user work with servers?

or

Does the user work with content?
```

VDR-Suite should move toward a content-oriented model.

```text
User
        ↓
Library
        ↓
Content
        ↓
Source
        ↓
Backend
```

not a server-first model.

```text
User
        ↓
Server
        ↓
Content
```

---

## Plex

### Relevant Strengths

Plex is successful because it presents media as libraries rather than as server paths or device-specific storage.

Typical user-facing concepts:

- Movies
- TV Shows
- Music
- Photos
- Live TV
- DVR
- shared libraries
- users and managed users
- remote access

The user usually does not think about individual storage locations or backend details.

### Lessons for VDR-Suite

VDR-Suite should avoid exposing the underlying VDR instance as the primary user concept.

Instead, VDR-Suite should expose:

- Recordings
- Live TV
- Timers
- Series
- Movies
- Archives
- Music
- Photos

The source of an item should remain available, but it should not dominate the user experience.

### Relevant Architecture Pattern

```text
Library
        ↓
Media Item
        ↓
Source
        ↓
Storage / Backend
```

---

## Jellyfin

### Relevant Strengths

Jellyfin provides an open-source media server model with libraries, users, access control and multiple clients.

It is relevant because VDR-Suite should also remain open and self-hosted.

### Lessons for VDR-Suite

VDR-Suite should treat multi-user operation as a first-class architecture concern.

Even if early versions run as a single-user local system, the architecture should not prevent:

- multiple users
- multiple roles
- shared libraries
- restricted libraries
- remote clients
- server-to-server access

### Relevant Architecture Pattern

```text
User
        ↓
Permissions
        ↓
Library
        ↓
Media Items
```

---

## Kodi

### Relevant Strengths

Kodi is strong as a frontend platform.

It can aggregate local media, network media and PVR backends.

Kodi demonstrates that the frontend should not be tightly bound to one backend implementation.

### Lessons for VDR-Suite

VDR-Suite frontends should remain backend-neutral.

Future frontends may include:

- web frontend
- Windows frontend
- Android frontend
- iOS frontend
- TV frontend
- OSD frontend

These frontends should talk to VDR-Suite APIs and not directly to VDR plugins or storage backends.

### Relevant Architecture Pattern

```text
Frontend
        ↓
Application API
        ↓
Backend Services
        ↓
Sources
```

---

## Tvheadend

### Relevant Strengths

Tvheadend is strong in Live TV, tuner abstraction, EPG and streaming.

It models multiple inputs and exposes them through a unified TV server interface.

### Lessons for VDR-Suite

VDR-Suite should learn from Tvheadend's ability to abstract multiple TV-related sources.

For VDR-Suite this means:

- multiple VDR instances
- multiple channel sources
- multiple timer sources
- multiple recording sources
- optional remote Live TV access

### Relevant Architecture Pattern

```text
TV Source
        ↓
Channel / EPG / Timer Model
        ↓
Unified API
        ↓
Client
```

---

## Emby

### Relevant Strengths

Emby is relevant as another media server with libraries, users, clients and remote access.

Its importance for VDR-Suite is similar to Plex and Jellyfin: it reinforces the need for a library and permissions model.

### Lessons for VDR-Suite

The media platform pattern is consistent across successful systems:

- libraries are central
- users are central
- clients are replaceable
- backends are implementation details

VDR-Suite should follow this direction while preserving VDR-specific strengths.

---

## VDR Today

Traditional VDR systems are powerful but often server-centric.

Typical concepts:

- one VDR instance
- plugins
- recordings
- timers
- channels
- local OSD

This is technically strong, but it can become limiting when the goal is a modern multi-device media platform.

The risk for VDR-Suite is to expose the VDR instance too directly to users and frontends.

---

## VDR-Suite Strategic Target

VDR-Suite should become a modern media platform around VDR, not only a web UI for one VDR server.

Target model:

```text
Library
        ↓
Source
        ↓
Backend
```

A backend may be:

- local VDR
- remote VDR
- remote VDR-Suite instance
- recording archive
- NAS source
- future plugin bridge
- future external media source

The user should primarily interact with:

- recordings
- live TV
- timers
- series
- movies
- archives

not with backend implementation details.

---

## Multi-VDR and Federation

A key future capability should be VDR-Suite federation.

Example:

```text
VDR-Suite A
        ↔
VDR-Suite B
        ↔
VDR-Suite C
```

A realistic use case:

```text
House A
        VDR-Suite

House B
        VDR-Suite

One frontend
        sees allowed content from both locations
```

Federation should not mean unrestricted trust.

Each VDR-Suite instance must be able to decide what other users or instances are allowed to see and do.

---

## Remote Permissions

VDR-Suite should prepare for a permission model that can control access to remote capabilities.

Possible permissions:

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

metadata.view

admin.manage_permissions
```

Future granular permissions may include:

```text
recordings.folder:<folder>
livetv.channel_group:<group>
source.access:<source>
backend.access:<backend>
```

This allows configurations such as:

```text
Remote Suite A:
        may view all recordings
        may not view Live TV
        may not create timers

Remote Suite B:
        may view only selected recording folders
        may stream Live TV from selected channel groups
        may not delete recordings

Admin User:
        may manage timers
        may manage permissions
```

---

## Plugin Configuration Implications

A future VDR-Suite plugin or bridge may need to expose local policy settings.

Possible configuration questions:

- Which remote VDR-Suite instances may connect?
- Which users may authenticate?
- Which recordings are visible?
- Which recording folders are shared?
- Is Live TV allowed?
- Are all channels allowed or only channel groups?
- Are timers visible?
- May timers be created?
- May timers be modified or deleted?
- May recordings be moved, deleted or renamed?

This means the plugin side should not be treated as a simple transport pipe.

It may eventually need a policy boundary.

---

## Architecture Requirements for VDR-Suite

The following requirements should be considered early:

### Library First

The user interface and API should be able to present content by library, not only by backend.

### Source Awareness

Every item should still know where it came from.

The source should be metadata, not necessarily the primary navigation model.

### Backend Neutrality

Services and frontends should not depend directly on a concrete VDR backend.

### Federation Readiness

The architecture should allow remote VDR-Suite instances as sources.

### Permission Layer

Permission checks should become a separate architecture concern before destructive operations are exposed remotely.

### Capability Model

Different backends may support different capabilities.

Example:

```text
Backend A:
        recordings.view
        timers.create

Backend B:
        recordings.view only

Backend C:
        livetv.view only
```

The API should eventually expose backend capabilities explicitly.

---

## Early Design Direction

A future architecture may evolve toward:

```text
User / Remote Instance
        ↓
Permission Layer
        ↓
Library Layer
        ↓
Source Layer
        ↓
Backend Adapter
        ↓
VDR / Archive / Remote Suite
```

This does not need to be implemented immediately.

However, future data models, APIs and frontend decisions should avoid blocking this direction.

---

## Non-Goals

This document does not implement:

- users
- authentication
- authorization
- federation protocol
- remote access
- Live TV streaming
- timer synchronization
- library database schema
- plugin configuration

This document only captures the strategic architecture direction.

---

## Follow-Up Work

Possible future documents or ADRs:

- Library First Architecture
- VDR-Suite Federation Strategy
- Permission and Role Model
- Backend Capability Model
- Remote VDR-Suite Source Strategy
- Plugin Policy Boundary

Possible later implementation phases:

- Source domain object
- Library domain object
- Backend capability model
- Permission model foundation
- Remote suite adapter foundation
---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
