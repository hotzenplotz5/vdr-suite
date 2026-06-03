# Phase 8 – Architecture Guardrails

## Status

Architecture guardrail document.

This document records practical rules for future Phase 8 work.

It does not introduce implementation changes.

## Core Direction

VDR-Suite remains VDR-centered.

The current implementation may continue to focus on one local VDR accessed through RESTfulAPI.

Future architecture must not block Multi-VDR, remote VDR-Suite instances, archives or federation.

The goal is not to build a generic media-center clone.

The goal is to keep VDR as the primary backend while avoiding irreversible single-VDR assumptions.

## Main Rule

Every new architecture or implementation step should ask:

```text
Does this introduce a permanent Single-VDR assumption?
```

If yes, the change must be reconsidered.

If the assumption is temporary and explicitly local to the current implementation phase, it is acceptable.

## Allowed Current Assumptions

The following assumptions are acceptable for the current implementation:

* one configured VDR
* RESTfulAPI as the primary backend integration path
* local development and testing with a single backend
* recording-focused jobs and workflows
* current `int recordingId` job model
* no database schema for Sources
* no REST API for Sources
* no frontend Source navigation
* no federation

These assumptions are implementation constraints, not long-term platform rules.

## Forbidden Long-Term Assumptions

Future architecture must not assume:

* there is exactly one VDR forever
* recording ids are globally unique
* timer ids are globally unique
* channel ids are globally unique
* event ids are globally unique
* backend equals VDR configuration
* Source equals VdrConfig
* Source equals adapter
* Library owns Sources
* Source owns content
* frontend hiding is a security boundary
* destructive operations can run without source-aware target identity

## VDR-Centered Rule

New work should provide direct value to VDR-Suite's VDR use case.

Acceptable near-term focus:

* VDR status
* VDR recordings
* VDR timers
* VDR channels
* VDR EPG
* RESTfulAPI integration
* VDR adapter quality
* recording workflow quality
* daemon/runtime stability

Not a near-term focus:

* generic media backends
* plugin marketplace
* arbitrary third-party backend API
* non-VDR media center abstraction
* full federation implementation
* generic policy engine

## Source Guardrails

Source is a future domain entity.

Source is not VdrConfig.

Source is not an adapter.

Source is not a Library.

Source is not a REST endpoint by itself.

Source represents origin and policy context.

VdrConfig remains technical backend configuration.

Correct future relationship:

```text
Source
        ↓
VdrConfig or backend config
        ↓
Adapter
        ↓
VDR
```

## Identity Guardrails

Backend-local ids must not be treated as globally unique.

Future global content identity must include source context.

Conceptual future rule:

```text
globalContentIdentity = sourceId + backendLocalId
```

Do not casually add `sourceId` fields everywhere without a dedicated migration plan.

ContentIdentity should be handled as a separate architecture step.

## Jobs And Actions Guardrails

The current job model uses `recording_id` and is valid for the current single-source recording workflow.

Do not change the jobs schema as part of SourceType or Source introduction.

Do not introduce destructive multi-source operations before ActionTarget and ContentIdentity exist.

Destructive operations include:

* delete recording
* move recording
* rename recording
* repair recording
* shrink recording
* cut recording
* modify timer
* delete timer
* remote-control actions

## Capability And Permission Guardrails

Capability and permission must remain separate.

Capability answers:

```text
Can this Source support this operation?
```

Permission answers:

```text
May this Actor perform this operation?
```

Do not put permission checks into adapters.

Do not treat frontend visibility as security.

Do not bake all capability logic directly into Source.

## Adapter Guardrails

Adapters are technical backend boundaries.

Adapters may know transport details.

Platform-level domain objects must not know transport details.

RESTfulAPI, SVDRP, HTTP, TCP and filesystem details belong below the adapter boundary.

The platform should operate in terms of Source, content identity, actions and targets.

## Frontend Guardrails

Do not expose Source as frontend navigation too early.

The user-facing model remains Library-first and content-first.

Sources may later inform filtering, permissions and origin display.

They should not force a server-first UI model.

## Database Guardrails

Do not introduce Source tables before the domain model is stable.

Do not change the jobs table before ActionTarget and ContentIdentity are designed.

Do not use database primary keys as long-term source identities.

## Practical Review Checklist

For each future phase, check:

1. Does it keep VDR as the primary backend?
2. Does it avoid permanent Single-VDR assumptions?
3. Does it avoid treating backend-local ids as global?
4. Does it keep Source separate from VdrConfig?
5. Does it avoid premature database schema changes?
6. Does it avoid premature REST API changes?
7. Does it avoid premature frontend navigation changes?
8. Does it keep adapters technical?
9. Does it avoid destructive operations without source-aware identity?
10. Does it provide direct value to the current VDR use case?

## Final Recommendation

Continue implementation in a VDR-centered way.

Use RESTfulAPI as the main practical backend path.

Keep platform concepts as guardrails, not as immediate generic abstractions.

Introduce Source-related implementation only when it directly reduces future Multi-VDR migration risk.
