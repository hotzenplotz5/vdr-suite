# ADR-0022: LIVE Functional Reference Strategy

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

VDR-Suite aims to become a multi-backend VDR platform rather than a frontend for a single VDR instance.

The VDR LIVE plugin represents the most mature and widely used reference implementation for VDR recording management, EPG navigation, EPG search, timer creation and related user workflows.

Real-world user expectations are therefore already defined by LIVE behavior.

At the same time, VDR-Suite must support capabilities that are outside the scope of LIVE, including multi-backend federation, backend-specific permissions, backend-neutral APIs and multiple client types.

---

## Decision

VDR-Suite adopts LIVE as its functional reference standard.

For user-visible VDR functionality, the target is:

```text
At least LIVE feature parity
plus
multi-backend capabilities
plus
backend-neutral APIs
plus
multiple client types
```

VDR-Suite will not depend on LIVE itself.

RESTfulAPI is the preferred backend integration mechanism.

When a desired LIVE-level capability is not available through RESTfulAPI, the preferred solution is to extend RESTfulAPI rather than introducing LIVE-specific dependencies into VDR-Suite.

---

## Functional Areas

The LIVE reference applies in particular to:

- recordings
- EPG navigation
- EPG search
- timer creation
- search timers
- conflict handling
- recording lifecycle workflows
- user-facing VDR information density

---

## Preferred Capability Workflow

For new functionality:

```text
1. Identify desired user feature.
2. Compare against LIVE behavior.
3. Check existing RESTfulAPI support.
4. Use existing RESTfulAPI capability if available.
5. Extend RESTfulAPI if capability is missing.
6. Consume capability through VDR-Suite adapters.
```

---

## Consequences

Benefits:

- clear functional benchmark
- avoids feature regressions relative to established VDR frontends
- keeps backend integration architecture clean
- strengthens RESTfulAPI as strategic backend interface
- supports long-term multi-backend architecture

Costs:

- requires periodic LIVE capability reviews
- may require additional RESTfulAPI development
- increases validation effort against real VDR systems

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)