# ADR-0010: Library First VDR Architecture

## Status

Accepted

## Context

VDR-Suite started as a modern service-oriented backend architecture around VDR recordings, metadata, jobs, dashboards, REST APIs and future frontends.

The current architecture already separates technical backend access behind adapter boundaries.

Existing VDR-side pattern:

```text
RuntimeConfig
        ↓
VdrConfig
        ↓
VdrAdapterFactory
        ↓
IVdrAdapter
```

Recent media platform analysis identified an important product-level architecture concern:

Successful media platforms usually present content as libraries, not as raw server structures.

However, VDR-Suite must not lose its VDR focus.

VDR remains the primary domain and the strongest differentiator of the project.

VDR-Suite should therefore adopt a Library First architecture with a VDR-centered interpretation.

## Decision

VDR-Suite will move toward a Library First architecture while keeping VDR as the primary backend domain.

The user-facing model should primarily expose content and capabilities:

- recordings
- Live TV
- timers
- EPG
- channels
- archives
- metadata

The system should not force users to navigate primarily by server or backend instance.

Target conceptual model:

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

The VDR backend remains central, but it should appear as a source behind the library model rather than as the only top-level navigation concept.

## VDR-Centered Interpretation

Library First does not mean that VDR becomes an optional afterthought.

For VDR-Suite, the first-class content types remain VDR-oriented:

- recordings
- timers
- Live TV
- channels
- EPG events
- VDR metadata
- future OSD and remote-control capabilities

A future library may aggregate content from multiple VDR-related sources:

```text
Recordings Library
        ├── Local VDR
        ├── Remote VDR-Suite House A
        ├── Remote VDR-Suite House B
        ├── Archive
        └── NAS import
```

The user should be able to browse recordings without first choosing the physical VDR server.

The source should remain visible as metadata and policy context.

## Consequences

Benefits:

- users interact with content instead of server internals
- multiple VDR instances can later be unified under one user experience
- remote VDR-Suite instances can become library sources
- permissions can be evaluated at user, library, source and capability levels
- frontends remain independent from backend topology
- VDR remains central while still allowing future sources

Trade-offs:

- additional domain concepts will be needed later
- source identity must be preserved for every item
- permission checks will become more important
- duplicate content and cross-source conflicts may need future handling
- destructive operations must remain source-aware

## Non-Goals

This ADR does not implement:

- Library domain objects
- Source domain objects
- permission checks
- federation protocol
- remote VDR-Suite access
- content deduplication
- frontend navigation changes
- database schema changes

This ADR only records the core architecture direction.

## Follow-Up Work

Future phases may introduce:

- Library model
- Source model
- Backend capability model
- permission and role model
- federation strategy
- remote VDR-Suite source adapter
- plugin policy boundary

The next architecture step should clarify the Source model with VDR as the primary backend type.
