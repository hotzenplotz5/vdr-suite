# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)

---

## Current Position

```text
Completed implementation state
Phase 54.2 - SearchTimer warm EPG cache architecture

Documentation consolidation step
Phase 54.2 - SearchTimer warm EPG cache architecture

Next major implementation milestone
Phase 54.3 - SearchTimer warm EPG cache implementation
```

---

## Purpose

This roadmap describes the completed milestones, current milestone, planned major milestones and long-term vision of VDR-Suite.

It is intended to answer four questions:

- What is already implemented?
- Which completed phases belong to which milestone?
- What is currently in progress?
- What is planned next?

Detailed chronological implementation history belongs to [Completed Phases](../development/completed-phases.md).

Current build and project state belongs to [Current Project Status](../development/current-status.md) and [Project Status Dashboard](../project-status-dashboard.md).

---

## Roadmap Status Terms

| Status | Meaning |
| --- | --- |
| Completed | The milestone has an implemented and tested foundation in the repository. |
| In Progress | The milestone has begun and still needs further implementation before it can be considered a foundation. |
| Planned | The milestone is a concrete intended implementation direction. |
| Vision | The milestone is strategically important but intentionally deferred until earlier foundations exist. |

---

## Completed Milestones

### Core Platform Foundation

Status: Completed.

Primary result:
- Establishes the initial VDR-Suite platform foundation with database schema, repositories, services, dashboard aggregation, REST-facing boundaries and daemon-oriented runtime structure.

### VDR Backend Foundation

Status: Completed.

Primary result:
- Introduces VDR domain objects, adapter boundaries and RESTfulAPI integration foundations.

### Multi-Backend and Snapshot Runtime Foundation

Status: Completed.

Primary result:
- Turns the daemon into a snapshot-backed runtime that can represent one or more VDR backends.

### Live Runtime and Change Feed Foundation

Status: Completed.

Primary result:
- Adds transport-independent live update infrastructure based on snapshot change detection.

### Recording Query Foundation

Status: Completed.

Primary result:
- Adds structured recording query support over snapshot-backed recordings.

### Recording Action Foundation

Status: Completed.

Primary result:
- Adds validation and execution foundations for recording actions while preserving safety boundaries.

### EPG Search Foundation

Status: Completed.

Primary result:
- Adds selective EPG search over backend-neutral EPG query boundaries.

### Content Classification Foundation

Status: Completed.

Primary result:
- Establishes source-aware content classification as the architectural foundation for genre, ratings, keywords, tags and future policy.

### Person Metadata Foundation

Status: Completed.

Primary result:
- Adds source-aware person metadata as a reusable foundation for actors, characters, directors, writers, producers, moderators, guests and composers.

### Recording Person and Character Search Foundations

Status: Completed.

Primary result:
- Connects real recording metadata to person and character search while preserving recording context.

### SearchTimer Backend and Capability Foundation

Status: Completed.

Primary result:
- Establishes the backend-facing SearchTimer foundation and validates native fuzzy backend capability behavior through real VDR-Suite and RESTfulAPI workflows.

### SearchTimer User Workflow Foundation

Status: Completed.

Primary result:
- Turns the backend-facing SearchTimer and native fuzzy capability foundation into a practical, tested user workflow foundation.

### SearchTimer Runtime Mutation Policy Foundation

Status: Completed.

Primary result:
- Adds the runtime mutation policy wiring and keeps backend mutation guarded behind explicit policy boundaries.

### SearchTimer Warm EPG Cache Architecture

Status: Completed.

Primary result:
- ADR-0034 defines that interactive SearchTimer preview must use warm backend-scoped EPG input instead of fetching the full RESTfulAPI EPG dump per request.

Completed phase highlights:
- Phase 54.1 - SearchTimer preview comparison-option fix.
- Phase 54.2 - SearchTimer warm EPG cache architecture.

---

## Current Milestone

### Phase 54 - SearchTimer Preview Runtime and EPG Input Performance

Status: In Progress.

Goal:
- Make SearchTimer preview correct and fast enough for operator-facing use against real VDR EPG data.

Completed results:
- Phase 54.0 wires runtime mutation policy execution while keeping the runtime gate closed by default.
- Phase 54.1 fixes SearchTimer preview comparison options and verifies title-only matching against live VDR EPG input.
- Phase 54.2 accepts ADR-0034 for warm EPG cache input, change-state invalidation and future SSE-triggered refresh.

Next implementation step:
- Phase 54.3 - SearchTimer warm EPG cache implementation.

Expected outcomes:
- Backend-scoped warm EPG cache for SearchTimer preview.
- Explicit cache readiness and stale-state diagnostics.
- No full RESTfulAPI EPG dump per interactive preview request.
- Change-state driven cache invalidation through polling first.
- Future RESTfulAPI SSE change stream support as an invalidation signal.

Important boundaries:
- Keep VDR as the source of truth for VDR-owned state.
- Keep SearchTimer preview read-oriented while implementing the cache path.
- Treat SSE as an invalidation signal, not as a full EPG data stream.
- Keep backend-native epgsearch fast paths capability-dependent and explicit.

---

## Planned Major Milestones

### Phase 55 - Backend Management and Client Administration Foundation

Status: Planned.

Goal:
- Turn backend configuration, capability visibility and backend health into manageable client-facing concepts.

Expected outcomes:
- Backend management API.
- Backend capability visibility.
- Backend health and diagnostics.
- Backend connection validation.
- Preparation for frontend backend administration.

### Recommendation Foundation

Status: Planned.

Goal:
- Build recommendation primitives on top of explicit metadata, search history, recording metadata and classification evidence.

Expected outcomes:
- Recommendation candidate model.
- Evidence-based ranking inputs.
- Profile-aware recommendation preparation.
- Explainable recommendation output.

### Cross-Backend Search and Federation

Status: Planned.

Goal:
- Make multi-backend search first-class across recordings, EPG and metadata after the SearchTimer preview data path is fast and reliable.

Expected outcomes:
- Cross-backend result aggregation.
- Backend-specific result context.
- Conflict and duplicate handling.
- Remote backend visibility rules.

---

## Long-Term Vision

### Content Knowledge Graph

Status: Vision.

Goal:
- Connect recordings, EPG events, people, characters, genres, ratings, backends, search timers and user profiles into a structured metadata graph.

Potential outcomes:
- Relationship-based browsing.
- Actor-to-character-to-recording navigation.
- Genre and rating based filtering.
- Backend-aware availability views.
- Recommendation and discovery foundations.

This is intentionally deferred until SearchTimer, unified search, profiles and metadata enrichment are more mature.

---

## Roadmap Maintenance Rules

- This file describes milestones and direction.
- Detailed chronological implementation history belongs in [Completed Phases](../development/completed-phases.md).
- Project status snapshots belong in [Current Project Status](../development/current-status.md) and [Project Status Dashboard](../project-status-dashboard.md).
- When a major feature block is completed, it should be promoted into a completed milestone here.
- When a planned milestone becomes active, it should move into the current milestone section.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../../README.md)
