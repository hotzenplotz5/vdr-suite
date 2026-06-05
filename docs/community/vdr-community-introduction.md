# VDR-Suite – Community Introduction

## Purpose

This document is an introduction for the VDR community and for technically interested contributors.

It explains what VDR-Suite is intended to become, what it is deliberately not intended to become, which architectural ideas already exist, how existing VDR projects are treated, and how the project is currently developed.

It is not an Architecture Decision Record.

It is meant as a readable entry point for discussions in the VDR community, for example in forums, mailing lists or early contributor conversations.

---

## Short Summary

VDR-Suite is a modern service-oriented platform around VDR.

VDR remains the source of truth.

VDR-Suite complements VDR.

VDR-Suite does not replace VDR.

The project is not intended to become another isolated web frontend. The long-term idea is to provide reusable VDR-centered platform capabilities that can be consumed by different frontends, applications, tools and possibly VDR plugins.

The core question is:

```text
Which VDR-Suite capabilities should be reusable by other programs?
```

---

## What VDR-Suite Is

VDR-Suite is intended to become a VDR-centered backend platform.

Its long-term purpose is to make VDR data and VDR-related functionality available through a cleaner, modern and reusable architecture.

Important capability areas include:

```text
Recording access
Timer access
Channel access
EPG access
Metadata access
Artwork access
Job and queue access
Runtime diagnostics
Backend identity
Backend capability discovery
Backend lifecycle state
Permission-aware operations
Live update events
Stream provider access
Future AI-assisted capabilities
```

These capabilities should be useful for multiple kinds of consumers:

```text
Web frontends
Desktop frontends
Mobile frontends
VDR OSD integration
VDR plugins
Automation tools
Monitoring tools
External services
Third-party VDR management tools
```

The intention is to build platform capabilities first and consumer-specific presentation later.

---

## What VDR-Suite Is Not

VDR-Suite is not intended to be:

- a replacement for VDR
- a Plex clone
- a Jellyfin clone
- a Kodi replacement
- a TVHeadend replacement
- a generic media server that merely happens to support VDR
- a special integration layer for one frontend or plugin
- a reimplementation of every existing VDR plugin
- a project that ignores existing VDR ecosystem knowledge

VDR remains central.

The architecture should preserve VDR strengths and add a modern service layer around them.

---

## Why VDR-Suite Exists

Traditional VDR environments are powerful, but functionality is often spread across plugins, SVDRP commands, REST endpoints, OSD integrations and frontend-specific code.

This works, but it can create tight coupling between clients and backend implementation details.

VDR-Suite introduces a stable service layer between consumers and backend access.

Expected benefits:

- clearer backend abstraction
- consistent domain models
- less repeated live backend access
- daemon-owned runtime state
- snapshot-based API responses
- easier frontend development
- better diagnostics
- preparation for future multi-VDR environments
- preparation for permission-aware clients
- reusable platform capabilities for multiple consumers

---

## Platform Instead Of Frontend

The project deliberately distinguishes between platform capabilities and consumer-specific applications.

A possible future web UI should not define the whole backend architecture.

A possible future desktop client should not define the whole backend architecture.

A possible future VDR plugin should not define the whole backend architecture.

Instead, all of these should consume reusable platform capabilities.

Conceptually:

```text
Consumer
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
VDR / RESTfulAPI / Remote VDR-Suite / Archive
```

This model is not fully implemented yet. It is a long-term architectural guide.

---

## Example: vdr-plugin-live

`vdr-plugin-live` is not treated as something VDR-Suite should integrate as a special case.

The more important question is what VDR-Suite can learn from Live.

Live may contain useful VDR domain knowledge about:

- timer workflows
- EPG browsing
- recording navigation
- channel views
- search and filtering behavior
- practical VDR user workflows
- which data a VDR web UI actually needs

The architectural relationship should be understood like this:

```text
Live may use VDR-Suite capabilities later.
VDR-Suite should not become Live-specific.
```

This is the same rule that applies to other potential consumers.

---

## Existing VDR Projects As Knowledge Sources

VDR-Suite should learn from existing projects instead of ignoring them.

However, learning from a project is different from directly integrating or copying it.

Examples of projects that may provide useful VDR domain knowledge:

- `vdr-plugin-live`
- `vdr-plugin-epgsearch`
- `vdr-plugin-restfulapi`
- `scraper2vdr`
- `tvscraper`
- `vdr-plugin-dynamite`
- `vdr-plugin-osd2web`
- SVDRP-based applications

The guiding question is:

```text
What problem did this project already solve for VDR users?
```

not:

```text
How do we integrate this project directly?
```

The goal is to preserve domain knowledge and implementation lessons without creating unnecessary architecture dependencies.

---

## Snapshot-Oriented Runtime

The current backend direction is snapshot-oriented.

Instead of making every frontend request query VDR or RESTfulAPI directly, the daemon owns a snapshot of backend state.

Conceptually:

```text
VDR
    ↓
RESTfulAPI
    ↓
BasicHttpClient
    ↓
RestfulApiVdrAdapter
    ↓
VdrService
    ↓
VdrSnapshotBuilder
    ↓
PollingService
    ↓
ChangeDetectionService
    ↓
SnapshotRefreshPlanner
    ↓
SnapshotUpdatePlan
    ↓
SnapshotCacheService
    ↓
SnapshotCache
```

This allows frontend-facing services to read from controlled snapshot state instead of repeatedly hitting the live backend.

This is especially important for future multi-client, multi-frontend and multi-VDR scenarios.

---

## Multi-VDR Direction

A long-term goal is to keep multi-VDR scenarios possible.

Example:

```text
House A
    VDR
    VDR-Suite

House B
    VDR
    VDR-Suite
```

Possible future questions include:

- Which backend owns a recording?
- Which backend is online?
- Which backend supports which capabilities?
- Which user or remote system may access which backend?
- Which operations are read-only?
- Which operations may modify timers or recordings?

This requires future concepts such as:

- stable backend identity
- backend lifecycle state
- backend capability discovery
- source identity
- actor identity
- permission checks
- backend routing

The project tries to avoid early decisions that would make these scenarios impossible later.

---

## AI As A Future Capability

AI may become useful for VDR-Suite later, especially for recordings and metadata.

Possible future use cases:

- recording content analysis
- automatic summaries
- chapter detection
- scene detection
- metadata enrichment
- artwork generation
- speaker recognition
- person recognition
- sports event extraction
- news topic extraction
- semantic search
- natural-language search
- storage recommendations
- recording cleanup suggestions

Examples:

```text
Show me all recordings about elections.

Show me all crime movies set in Hamburg.

Summarize this documentary.

Which recordings can safely be deleted?
```

AI should not be treated as a ChatGPT-specific integration.

It should be treated as a future provider-neutral platform capability.

Conceptually:

```text
AI Capability
        ↓
AI Provider Boundary
        ↓
OpenAI
Ollama
LM Studio
Local Models
Future Providers
```

VDR-Suite should not become coupled to a specific AI vendor.

---

## Development Model

The initial development phase has been strongly AI-assisted.

This is an explicit part of the project history.

The current working model is roughly:

```text
Human responsibilities:
    vision
    prioritization
    architecture decisions
    requirements
    review
    testing direction
    orchestration

AI responsibilities:
    documentation drafts
    architecture proposals
    implementation proposals
    test proposals
    refactoring assistance
    consistency checks
```

This does not mean that the architecture is generated automatically.

The project direction, priorities and acceptance decisions remain human-driven.

The AI-assisted workflow mainly accelerates documentation, implementation iteration, test scaffolding, refactoring and architectural consistency work.

This development model should be transparent because it explains the unusually fast initial project pace.

---

## Current Status Summary

The project currently contains foundations for:

- SQLite persistence
- recording, metadata, job and action services
- dashboard services and JSON serialization
- REST controllers and API routing
- HTTP client and server abstractions
- VDR domain objects and adapter boundaries
- RESTfulAPI integration
- daemon-owned VDR snapshots
- change-state polling
- change detection
- partial snapshot refresh planning
- runtime logging
- runtime timing
- diagnostics foundations
- platform API strategy
- core platform model
- external project analysis framework
- future AI capability strategy

The current implementation status changes frequently during active development. The authoritative status is maintained in `docs/development/current-status.md`.

---

## Questions For The VDR Community

Feedback from experienced VDR users and developers is especially valuable in the following areas:

- Which VDR workflows must not be lost?
- Which Live workflows are still important?
- Which EPGSearch workflows are essential?
- Which timer and conflict scenarios are common?
- How do people use multiple VDR systems today?
- Which multi-VDR scenarios are realistic?
- Which operations should be read-only for remote users?
- Which permissions would be needed in real households?
- Which VDR plugins contain important domain knowledge?
- Which existing solutions should VDR-Suite learn from?
- Which mistakes should VDR-Suite avoid?

The goal is early architectural feedback, not a premature feature wishlist.

---

## Important Documentation Links

Start here:

- [VDR-Suite Vision](../introduction/vdr-suite-vision.md)
- [Documentation Index](../index.md)
- [Current Project Status](../development/current-status.md)
- [Core Platform Model](../architecture/vdr-suite-core-platform-model.md)

Architecture and strategy:

- [Platform API Strategy](../adr/007-platform-api-strategy.md)
- [External Project Analysis](../architecture/external-project-analysis.md)
- [Snapshot Architecture](../architecture/snapshot-architecture.md)
- [VDR Backends](../architecture/vdr-backends.md)
- [Snapshot Access Architecture](../architecture/snapshot-access-architecture.md)

---

## Closing Note

VDR-Suite is still early.

The project is already architecture-heavy by design because several later decisions are difficult to retrofit:

- multi-VDR readiness
- backend identity
- source identity
- capability discovery
- permission-aware operations
- stream provider neutrality
- diagnostics
- reusable platform APIs
- provider-neutral AI capabilities

The goal is not to build a large frontend first and fix the architecture later.

The goal is to build the backend and platform model carefully enough that future frontends, plugins, tools and integrations can use the same foundation.
