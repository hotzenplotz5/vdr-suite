# ADR-0029: Backend-Neutral SearchTimer Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Roadmap](../planning/roadmap.md)
- [Current Project Status](../development/current-status.md)

---

## Status

Accepted.

---

## Context

SearchTimer functionality is a central VDR automation concept.

A real yaVDR RESTfulAPI validation showed that RESTfulAPI already exposes SearchTimer-related endpoints when the VDR epgsearch plugin is installed.

Observed RESTfulAPI endpoints include:

- search timer list
- search timer result query
- recording directories
- channel groups
- blacklists
- timer conflicts
- extended EPG info
- update trigger

The tested system currently had no configured search timers, but the endpoints were available and returned valid JSON payloads.

VDR-Suite is a multi-backend system. A SearchTimer must therefore not be modeled as an epgsearch-specific object.

---

## Decision

SearchTimer is a VDR-Suite domain concept.

The VDR-Suite SearchTimer model is backend-neutral.

The SearchTimer domain must not depend on:

- epgsearch
- RESTfulAPI
- SVDRP
- a specific VDR backend
- a specific future scheduler implementation

Backend implementations must map their native SearchTimer representation to and from the VDR-Suite SearchTimer domain model.

---

## Initial Provider

The initial provider is RESTfulAPI backed by epgsearch.

This provider is treated as an adapter implementation, not as the domain model itself.

Initial provider capabilities should be modeled separately from the domain:

- searchTimers.read
- searchTimers.write
- searchTimers.delete
- searchTimers.queryResults
- searchTimers.triggerUpdate
- searchTimers.conflicts
- searchTimers.recordingDirs
- searchTimers.channelGroups
- searchTimers.blacklists
- searchTimers.extendedEpgInfo

---

## SeriesTimer Boundary

SearchTimer and SeriesTimer are related but not identical concepts.

SearchTimer describes rule-based EPG searches that can create or update VDR timers.

SeriesTimer describes higher-level series semantics such as seasons, episodes, duplicate handling, episode progress and provider metadata.

SeriesTimer behavior is intentionally out of scope for the initial SearchTimer foundation.

A future SeriesTimer milestone may build on top of SearchTimer and future media metadata foundations.

---

## Consequences

VDR-Suite services operate on backend-neutral SearchTimer objects.

Provider-specific fields remain inside adapter and mapper implementations.

The RESTfulAPI epgsearch adapter can be the first concrete implementation.

Future providers remain possible, including:

- SVDRP
- plugin bridge
- native VDR-Suite scheduler
- remote backend federation

SearchTimer capabilities must be advertised per backend.

SearchTimer IDs must remain backend-aware and must not be assumed to be globally unique without backend identity.

---

## Non-Goals

- No SearchTimer persistence schema is defined by this ADR.
- No REST API contract is defined by this ADR.
- No SearchTimer execution engine is defined by this ADR.
- No SeriesTimer model is defined by this ADR.
- No media metadata provider decision is defined by this ADR.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to ADR Index](index.md)