# Phase 21.1 - RESTfulAPI Event Stream Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

This document starts the Phase 21.1 architecture work for RESTfulAPI event stream strategy.

Phase 21.1 is not an implementation phase yet.

The purpose is to define how backend-specific event streams can provide change hints without replacing the snapshot architecture.

---

## Core Rule

Event streams provide change hints.

Event streams do not provide authoritative domain data.

Authoritative data remains snapshot-based.

Correct direction:

```text
Backend-specific event stream
  -> change hint
  -> refresh planning
  -> snapshot refresh
  -> snapshot change feed
  -> live transport
```

Incorrect direction:

```text
Backend-specific event stream
  -> live transport directly
```

---

## Backend Neutrality

VDR-Suite must not become directly dependent on RESTfulAPI SSE.

RESTfulAPI event streams are a possible optimization for RESTfulAPI-backed adapters.

They are not the core architecture.

Possible future hint sources:

```text
PollingChangeHintSource
RestfulApiEventStreamSource
SvdrpNotificationSource
PluginBridgeChangeHintSource
```

All hint sources must feed the same domain-aware refresh strategy.

---

## Change Hint Semantics

A change hint describes that something may have changed.

A change hint does not carry the complete domain payload.

Examples:

```text
StatusChanged
ChannelsChanged
TimersChanged
RecordingsChanged
EpgChanged
```

The refresh planner decides what to refresh.

---

## Heavy Domain Rule

Events / EPG remains a heavy domain.

A hint such as `EpgChanged` must not automatically trigger a full `/events.json` refresh.

Instead, the hint should mark the EPG domain as dirty and defer refresh decisions to a heavy-domain policy.

Future heavy domains follow the same rule:

```text
Metadata
Posters
Fanart
Preview images
Preview streams
Scraper-derived data
```

---

## Relationship to Live Transport

Live transport remains above the snapshot change feed.

Live transport publishes already validated snapshot change feed entries.

It must not own:

- backend polling
- backend event stream parsing
- snapshot generation
- refresh planning
- heavy-domain policy

---

## Current Phase 21.1 Status

Phase 21.1 is currently the active architecture focus.

It is not yet marked as completed.

Current completed phase remains:

```text
Phase 21.0 - Real VDR Runtime Polling Findings
```

Current implementation focus remains:

```text
Phase 21.1 - RESTfulAPI Event Stream Strategy
```

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
