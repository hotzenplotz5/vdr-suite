# External Project Analysis

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document records external VDR-related projects that may provide useful domain knowledge, workflow ideas or implementation lessons for VDR-Suite.

It is intentionally not an Architecture Decision Record.

The purpose is not to declare dependencies or integration targets.

The purpose is to preserve research findings:

```text
What can VDR-Suite learn from existing VDR projects?
```

This is different from:

```text
How should VDR-Suite integrate with a specific project?
```

VDR-Suite should remain a platform of reusable capabilities. External projects may be useful as knowledge sources, but VDR-Suite should not become coupled to them as special cases.

---

## Classification

External projects should be classified by the kind of value they may provide.

### Architecture Reference

Projects that may provide useful ideas about service boundaries, API design, multi-user behavior, streaming, federation, monitoring or platform design.

These projects are not necessarily VDR-specific.

Examples:

- Jellyfin
- Kodi
- TVHeadend
- Plex-like systems as comparison targets, not cloning targets

### VDR Domain Knowledge Source

Projects that encode years of VDR-specific workflow knowledge.

These are valuable because they show which problems VDR users and plugin authors have already solved.

Examples:

- `vdr-plugin-live`
- `vdr-plugin-epgsearch`
- `vdr-plugin-restfulapi`
- `scraper2vdr`
- `tvscraper`
- `vdr-plugin-dynamite`
- `vdr-plugin-osd2web`
- SVDRP-based applications

### Direct Code Reuse Candidate

Projects or components whose code may be reusable directly.

This category should be used very carefully.

VDR-Suite should prefer learning from proven domain logic, data models and workflows over copying old web, HTTP, template, CGI or session infrastructure.

---

## Evaluation Template

Each analyzed project should be documented using this structure:

```text
Project
Purpose
Category
Useful concepts
Relevant VDR domain knowledge
Possible VDR-Suite impact
What not to copy
Open questions
```

---

## vdr-plugin-live

### Purpose

`vdr-plugin-live` is a long-standing VDR web interface plugin.

### Category

VDR Domain Knowledge Source.

### Useful Concepts

Potentially useful areas include:

- timer workflows
- EPG browsing
- recording navigation
- channel views
- search and filtering behavior
- practical VDR user workflows
- which data a VDR web UI actually needs

### Relevant VDR Domain Knowledge

Live may contain valuable knowledge about how VDR users expect to interact with:

- timers
- recordings
- EPG events
- channels
- remote VDR operations

The value is primarily in the domain workflows and UI information requirements, not in making Live part of the VDR-Suite architecture.

### Possible VDR-Suite Impact

Live can help answer questions such as:

```text
Which VDR data must be exposed by VDR-Suite platform capabilities?
Which workflows are important for real VDR users?
Which timer and EPG edge cases already exist in mature VDR tools?
```

### What Not To Copy

VDR-Suite should not copy or depend on:

- old web server architecture
- template system design
- session handling
- CGI-style infrastructure
- frontend-specific architecture

### Architectural Boundary

Live is not a special integration target for VDR-Suite.

If useful later, Live or a similar VDR plugin could become a consumer of VDR-Suite APIs or libraries.

```text
Live may use VDR-Suite capabilities.
VDR-Suite should not become Live-specific.
```

---

## vdr-plugin-epgsearch

### Purpose

`vdr-plugin-epgsearch` provides advanced EPG search and timer automation for VDR.

### Category

VDR Domain Knowledge Source.

### Useful Concepts

Potentially useful areas include:

- search profiles
- search timers
- timer automation
- conflict handling
- recurring search logic
- EPG-based recording workflows

### Possible VDR-Suite Impact

EPGSearch may be highly relevant when VDR-Suite expands beyond basic event listing into:

- advanced EPG search
- automated timer creation
- rule-based recording workflows
- conflict detection
- saved searches

### What Not To Copy

VDR-Suite should not blindly copy plugin-internal implementation details before defining its own backend-neutral service boundaries.

---

## vdr-plugin-restfulapi

### Purpose

`vdr-plugin-restfulapi` exposes VDR data through HTTP/JSON endpoints.

### Category

VDR Domain Knowledge Source and current backend integration provider.

### Useful Concepts

Relevant areas include:

- VDR status representation
- channel endpoint behavior
- recording endpoint behavior
- timer endpoint behavior
- event endpoint behavior
- JSON field conventions
- practical backend access constraints

### Possible VDR-Suite Impact

RESTfulAPI is currently the primary live integration path for VDR-Suite.

It is useful as:

- a real backend data source
- a mapping reference for VDR domain objects
- a compatibility boundary for current local VDR integration

### What Not To Copy

VDR-Suite should not let RESTfulAPI endpoint shapes permanently define the internal platform model.

RESTfulAPI remains behind adapter boundaries.

---

## scraper2vdr and tvscraper

### Purpose

These projects provide metadata and artwork-related functionality for VDR ecosystems.

### Category

VDR Domain Knowledge Source.

### Useful Concepts

Potentially useful areas include:

- metadata enrichment
- artwork discovery
- movie and series metadata
- mapping between VDR recordings and external metadata sources
- cache behavior
- failure handling for incomplete metadata

### Possible VDR-Suite Impact

These projects may inform future VDR-Suite metadata and artwork services.

Relevant future capability areas include:

- metadata access
- artwork access
- recording enrichment
- search and browsing improvements
- UI presentation data

### What Not To Copy

VDR-Suite should not hard-code one scraper implementation into the core architecture.

Metadata should remain behind service and capability boundaries.

---

## vdr-plugin-dynamite

### Purpose

`vdr-plugin-dynamite` is relevant for dynamic device handling in VDR environments.

### Category

VDR Domain Knowledge Source.

### Useful Concepts

Potentially useful areas include:

- dynamic backend or device availability
- runtime lifecycle changes
- device presence and absence
- service degradation
- reconnect and recovery thinking

### Possible VDR-Suite Impact

Dynamite may inform later backend lifecycle and capability-state work.

This is relevant to existing ADRs about:

- backend lifecycle
- backend capability strategy
- multi-backend behavior

### What Not To Copy

VDR-Suite should not couple its lifecycle model directly to VDR device internals.

The lesson is the need for dynamic availability handling, not direct adoption of device-management internals.

---

## vdr-plugin-osd2web

### Purpose

`vdr-plugin-osd2web` exposes or bridges VDR OSD concepts toward web presentation.

### Category

VDR Domain Knowledge Source.

### Useful Concepts

Potentially useful areas include:

- OSD representation
- remote UI projection
- interaction mapping
- plugin-facing UI concepts
- browser-based representation of VDR-specific UI state

### Possible VDR-Suite Impact

OSD2Web may become relevant when VDR-Suite considers:

- OSD integration
- plugin UI surfaces
- remote-control concepts
- bridging classic VDR UI expectations into modern frontends

### What Not To Copy

VDR-Suite should not copy OSD2Web as a frontend architecture.

The useful part is understanding VDR-specific OSD workflows and constraints.

---

## SVDRP-Based Applications

### Purpose

SVDRP-based applications interact with VDR through the SVDRP protocol.

### Category

VDR Domain Knowledge Source.

### Useful Concepts

Potentially useful areas include:

- command workflows
- remote-control operations
- timer management
- status queries
- compatibility constraints

### Possible VDR-Suite Impact

SVDRP applications may inform future adapter or compatibility work, especially if VDR-Suite later supports an SVDRP backend adapter.

### What Not To Copy

VDR-Suite should not expose raw SVDRP semantics as its platform API.

SVDRP should remain a possible backend access mechanism behind an adapter.

---

## Documentation Rule

When a new external project is analyzed, add a section to this document and classify it as one or more of:

- Architecture Reference
- VDR Domain Knowledge Source
- Direct Code Reuse Candidate

Each section should clearly state:

- what is useful
- what is not useful
- whether the project affects architecture, domain modeling, API design or implementation planning

---

## Current Conclusion

External VDR projects are valuable primarily because they preserve VDR-specific domain knowledge.

For VDR-Suite, the most important question is usually:

```text
What problem did this project already solve for VDR users?
```

not:

```text
How do we integrate this project directly?
```

This distinction keeps VDR-Suite open as a modern platform while still learning from the existing VDR ecosystem.
---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
