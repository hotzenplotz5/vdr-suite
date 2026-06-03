# ADR-0011: VDR Source Model

## Status

Accepted

## Context

ADR-0010 introduced the Library First VDR Architecture.

The target model is:

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
        ↓
VDR / Remote VDR-Suite / Archive
```

This creates a new core architecture question:

What is a Source?

VDR-Suite must answer this early because Source identity will affect future library views, permissions, capabilities, federation, remote access and destructive operations.

The Source model must remain VDR-centered.

VDR remains the primary backend domain.

## Decision

VDR-Suite will treat Source as a future first-class architecture concept.

A Source represents the origin and policy context of content or capabilities exposed through a library.

A Source is not the same as a backend implementation.

A backend is the technical adapter or transport.

A Source is the user- and policy-visible origin of content.

Conceptual relationship:

```text
Library
        ↓
Content Item
        ↓
Source
        ↓
Backend Adapter
        ↓
VDR / Remote VDR-Suite / Archive
```

## Source Examples

Possible future Source types:

- Local VDR
- Remote VDR
- Remote VDR-Suite instance
- Recording archive
- NAS import
- future plugin bridge
- future Tvheadend bridge
- future SAT>IP source

The first and primary Source type remains VDR.

Other sources are future extensions and must not dilute the VDR-centered project direction.

## VDR-Centered Source Meaning

For VDR-Suite, a Source may expose VDR-specific capabilities such as:

- recordings
- Live TV
- timers
- channels
- EPG events
- metadata
- OSD or remote control capabilities

Different sources may support different subsets.

Example:

```text
Local VDR Source:
        recordings
        Live TV
        timers
        EPG
        channels

Archive Source:
        recordings only

Remote VDR-Suite Source:
        capabilities depend on remote permissions
```

## Source Identity

Every future content item should be able to retain source identity.

This is important for:

- showing where an item came from
- applying permissions
- routing operations back to the correct backend
- avoiding unsafe destructive operations
- handling remote VDR-Suite federation
- resolving duplicate or overlapping content

Example:

```text
Recording: Tatort
Library: Recordings
Source: House A VDR
Backend: Remote VDR-Suite Adapter
```

The user may browse the Recordings library without first choosing House A.

But the system must still know that the item belongs to House A.

## Source vs Backend

A Source is not an adapter.

A Source describes what is exposed.

A Backend describes how it is accessed.

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

This separation allows VDR-Suite to change backend communication without changing the user-facing Source concept.

## Source and Permissions

Source is the natural boundary for future permissions.

Possible examples:

```text
source.view:house-a
source.recordings.view:house-a
source.livetv.view:house-a
source.timers.create:house-a
source.timers.modify:house-a
```

Future permission checks may combine:

- user identity
- remote instance identity
- source
- library
- capability
- operation

This ADR does not implement permissions.

It only identifies Source as a likely future permission boundary.

## Source and Capabilities

Each Source should eventually expose capabilities.

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

metadata.view
```

Capabilities are not the same as permissions.

Capability means the Source can technically support something.

Permission means the user or remote instance is allowed to do it.

Both will be needed for safe federation.

## Consequences

Benefits:

- Library First architecture remains source-aware
- multiple VDR instances can be unified without losing origin information
- federation can be added later without redesigning content ownership
- destructive operations can remain source-aware
- permissions and capabilities get a natural boundary
- frontends can show content first and source second

Trade-offs:

- future domain model becomes richer
- each content item may need source metadata
- duplicate content handling may become necessary
- operations must be routed through source-aware services
- permission design becomes more important before remote write operations

## Non-Goals

This ADR does not implement:

- Source domain object
- Source repository
- Source database schema
- Source REST API
- permission checks
- capability model
- federation protocol
- remote adapter
- frontend source navigation

This ADR records the architecture role of Source only.

## Follow-Up Work

Future phases may introduce:

- Source domain object
- SourceType enum or value object
- Source capability model
- Source-aware recording summaries
- Source-aware library views
- Remote VDR-Suite source strategy
- permission model foundation

The next implementation-oriented step may introduce a small backend-neutral Source domain object after another architecture review.
