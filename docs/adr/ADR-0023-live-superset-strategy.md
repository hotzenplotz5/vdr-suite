# ADR-0023: LIVE Superset Strategy

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current Project Status](../development/current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Status

Accepted

---

## Context

ADR-0022 defines the VDR LIVE plugin as the functional reference standard for VDR-Suite.

LIVE is the benchmark for mature user-facing VDR workflows such as recording management, EPG navigation, EPG search, timer creation and search timers.

VDR-Suite must first reach that functional baseline, but the long-term goal is not to clone LIVE. VDR-Suite is designed as a multi-backend, REST-first and client-independent platform.

Therefore the architecture must make LIVE parity the minimum requirement and define a higher VDR-Suite target above that baseline.

---

## Decision

VDR-Suite treats LIVE as the functional gold standard and aims to become a superset of that standard.

The product target is:

```text
LIVE feature parity
plus
multi-backend federation
plus
REST-first API contracts
plus
backend-specific permissions
plus
read-only backend support
plus
dry-run validation
plus
auditable action and job boundaries
plus
client-independent JSON APIs
```

LIVE defines the expected user workflow quality.

VDR-Suite defines the platform architecture above that workflow baseline.

---

## Strategic Rules

### 1. Feature parity before replacement

Core VDR workflows should not be considered complete until they reach at least LIVE-level usability.

This applies especially to:

- recordings
- recording details
- recording lifecycle actions
- timer listing
- timer creation and editing
- EPG browsing
- EPG search
- search timers
- conflict-relevant user feedback

### 2. RESTfulAPI remains the preferred backend transport

VDR-Suite must not depend on LIVE as a runtime backend.

When LIVE exposes a useful workflow that RESTfulAPI does not yet support, the preferred strategy is:

```text
Extend RESTfulAPI
then consume the new capability through VDR-Suite adapters
```

A LIVE-specific adapter should only be considered if RESTfulAPI extension is impossible or explicitly rejected by a later ADR.

### 3. Backend identity must remain explicit

LIVE is primarily a frontend for one VDR context.

VDR-Suite must keep backend identity explicit in all relevant domains so that multiple VDR backends can be represented safely.

This affects:

- recording identity
- timer identity
- channel identity
- event identity
- action execution
- permissions
- read-only backend behavior

### 4. Actions must be safer than traditional frontend actions

Recording and timer actions must not be direct frontend-to-filesystem or frontend-to-backend shortcuts.

They must pass through explicit boundaries:

```text
request
validation
dry-run plan
capability check
permission check
job or execution boundary
result and diagnostics
```

This makes VDR-Suite more suitable for multi-client and multi-backend deployments than a traditional single-frontend model.

### 5. Client independence is mandatory

LIVE provides an integrated web frontend.

VDR-Suite must expose backend-neutral API contracts that can support:

- web clients
- mobile clients
- TV or OSD clients
- automation clients
- future external integrations

The REST API contract must not assume one specific frontend implementation.

---

## Functional Superset Areas

### Recordings

LIVE baseline:

- list recordings
- show recording details
- delete recordings
- rename or move recordings where supported
- expose user-relevant recording metadata

VDR-Suite superset:

- backend-aware recording identity
- backend-aware recording query filters
- recording action validation
- dry-run planning
- read-only backend guards
- RESTfulAPI-compatible action execution boundaries
- job and audit readiness

### Timers

LIVE baseline:

- list timers
- create timers
- edit timers
- delete timers
- represent timer state clearly

VDR-Suite superset:

- backend-aware timer identity
- REST-first timer mutation contracts
- capability-aware timer operations
- permission-aware timer operations
- future conflict and federation-aware timer behavior

### EPG and EPG Search

LIVE baseline:

- browse current and upcoming EPG
- search EPG data
- filter by channel and time
- provide useful event detail density

VDR-Suite superset:

- selective backend EPG queries
- channel-window and time-window APIs
- backend-aware EPG query results
- client-independent JSON contracts
- avoidance of heavy full-domain runtime refreshes

### Search Timers

LIVE baseline:

- search timer workflows, commonly through epgsearch integration

VDR-Suite superset:

- RESTfulAPI-compatible search timer access where available
- later backend-neutral search timer contract
- capability detection for epgsearch-backed and native implementations
- multi-backend search timer boundaries

---

## Consequences

Benefits:

- defines a clear quality target beyond simple feature completion
- prevents VDR-Suite from becoming a weaker reimplementation of existing VDR frontends
- keeps LIVE as a reference while preserving RESTfulAPI as the strategic transport
- supports the long-term multi-backend architecture
- supports future web, mobile and automation clients
- makes destructive actions safer through validation, permission and job boundaries

Costs:

- requires repeated comparison against LIVE behavior
- may require RESTfulAPI extensions before VDR-Suite can implement some features cleanly
- increases architecture and validation effort before real write operations are enabled
- requires careful compatibility handling for existing RESTfulAPI semantics

---

## Follow-up Work

Future phases should use this ADR together with ADR-0022 when planning:

- recording identity exposure
- recording action validation APIs
- timer APIs
- EPG search APIs
- search timer architecture
- RESTfulAPI extension proposals

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
