# Phase 8.39 – ContentIdentity Architecture

## Status

Architecture decision for future multi-source content identity.

No implementation has been performed by this document.

## Context

The Source architecture introduces the idea that VDR-Suite may later aggregate content from multiple origins:

* local VDR
* remote VDR
* remote VDR-Suite
* archive
* NAS import
* mock/test source

This creates an identity problem.

Backend-local ids are not globally unique.

Examples:

* House A may have recording id 42.
* House B may also have recording id 42.
* Archive may contain an imported recording with local id 42.

The same applies to timers, channels and EPG events.

## Decision

VDR-Suite should introduce ContentIdentity as a future architecture concept.

ContentIdentity represents global identity across sources.

A future ContentIdentity should combine:

* sourceId
* backendLocalId

Conceptual rule:

```text
globalContentIdentity = sourceId + backendLocalId
```

## Why ContentIdentity Is Needed

A single backend-local id is safe only inside one Source.

It is not safe across multiple Sources.

Incorrect future assumption:

```text
recording.id is globally unique
```

Correct future assumption:

```text
recording identity is unique only with Source context
```

## Affected Domain Areas

ContentIdentity is not only a Recording problem.

It affects:

* recordings
* timers
* channels
* EPG events
* future metadata links
* future job/action targets
* future federation references

## ContentIdentity As Value Object

ContentIdentity should eventually become a value object.

Reason:

sourceId and backendLocalId must travel together.

Avoid duplicating this pair in every content type separately.

Incorrect direction:

```text
Recording has sourceId and recordingId
Timer has sourceId and timerId
Channel has sourceId and channelId
Event has sourceId and eventId
```

Better future direction:

```text
Recording has ContentIdentity
Timer has ContentIdentity
Channel has ContentIdentity
Event has ContentIdentity
```

## ContentIdentity Is Not Persistence

This architecture decision does not introduce database schema.

It does not define primary keys.

It does not require migration of existing tables.

Database identity and domain identity must remain separate until a later migration phase explicitly connects them.

## ContentIdentity Is Not REST API Yet

This architecture decision does not introduce REST API changes.

Future REST responses may expose source-aware identities, but not in this phase.

Do not change current REST routes or JSON output as part of the first ContentIdentity architecture step.

## ContentIdentity And SourceRegistry

SourceRegistry knows available Sources.

ContentIdentity references one Source through sourceId.

Future lookup relationship:

```text
ContentIdentity
        ↓ sourceId
SourceRegistry
        ↓
Source
```

This allows VDR-Suite to resolve where content belongs without exposing adapter internals.

## ContentIdentity And Jobs

Future jobs and actions must become source-aware before destructive multi-source operations are allowed.

Examples:

* delete recording
* move recording
* rename recording
* modify timer
* delete timer

A job targeting only backendLocalId is unsafe in a multi-source architecture.

Future safe target:

```text
Job target = ContentIdentity + operation
```

## ContentIdentity And Federation

Federation requires source-aware identity.

Remote systems may expose local backend ids that collide with local ids.

Federation must not treat remote ids as globally unique.

Future federation reference:

```text
federation instance identity + sourceId + backendLocalId
```

This document does not define federation identity yet.

It only establishes that ContentIdentity is required before federation can safely reference content.

## Implementation Boundary

The first ContentIdentity implementation must not happen before minimal Source and SourceRegistry direction are stable.

Do not modify existing VDR domain objects in the same phase as the first Source object unless a dedicated migration step is approved.

Do not add sourceId fields to VdrRecording, VdrTimer, VdrChannel or VdrEvent casually.

That is a model migration and must be reviewed separately.

## Recommended Phase Order

Architecture-safe order:

1. SourceType
2. Source
3. SourceRegistry
4. ContentIdentity architecture
5. ContentIdentity value object
6. Source-aware content model migration
7. Source-aware jobs and actions
8. Capability integration
9. Permission integration
10. Federation identity model

## Non-Goals

This document does not implement:

* ContentIdentity class
* SourceId value object
* database schema
* REST output
* frontend behavior
* content migration
* job migration
* federation identity
* permission checks
* capability checks

## Final Recommendation

ContentIdentity is a critical future architecture concept.

It should be documented before source-aware content migration begins.

The first implementation should still begin with SourceType and Source.

ContentIdentity should follow before any multi-source runtime, destructive remote operation or federation work.
