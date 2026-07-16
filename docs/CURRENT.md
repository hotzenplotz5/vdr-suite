# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Completed Architecture Source Audit](development/architecture-source-audit-2026-07-15.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR Index](adr/index.md)
- [Completed Phases](development/completed-phases.md)
- [Completed Phases Latest Marker](development/completed-phases-latest.md)

---

## Current Verified State

Current completed project block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Previous completed major project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Historical umbrella implementation track:

```text
Phase 58 - Frontend and Live Parity
```

Latest completed implementation slice:

```text
Phase 60.14k - Recording Detail UX Polish
```

Next planned runtime implementation slice:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

The Phase 58 umbrella label is retained for product-history grouping. It does not control the strict future sequence.

---

## Completed Architecture Audit

```text
Architecture Source Audit - 2026-07-15
Status: Completed evidence and decision activity
```

The audit covered VDR Core, epgsearch, Live, RESTfulAPI, Streamdev, TVScraper, scraper2vdr, osd2web, epg2vdr and epgd.

Results are split into:

- [Completed audit evidence](development/architecture-source-audit-2026-07-15.md)
- [Living implementation-gap matrix](planning/architecture-audit-gap-matrix.md)
- [Strict future execution order](planning/roadmap.md)

The completed audit is not completed runtime implementation.

---

## Accepted Architecture Package

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
```

ADR-0042 accepts the common mutation contract. It does not mark universal revision, durable idempotency storage or all domain migrations as implemented.

---

## Immediate Repository Work

Continue the second contract package before Phase 60.15:

```text
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
ADR-0046 - Streaming Gateway and Media Session Boundary
ADR-0047 - Legacy OSD Compatibility Bridge
ADR-0048 - Public API Versioning, Error and Compatibility Contract
ADR-0049 - Audit and Security Event Model
```

Required follow-up:

```text
Update affected architecture diagrams.
Create domain and implementation dependency maps.
Keep the strict Phase 60.15-68 sequence aligned with the decisions.
```

---

## Strict Future Sequence

```text
1. ADR-0043 through ADR-0049 and diagrams
2. Phase 60.15 - Recording Metadata Preparation
3. Phase 61 - Suite Metadata Platform
4. Phase 62 - Identity, RBAC and Audit
5. Phase 63 - Backend Agent and Multi-Site Runtime
6. Phase 64 - Timer Intent and Orchestration
7. Phase 65 - Streaming Gateway
8. Phase 66 - Legacy OSD Bridge
9. Phase 67 - Public API and Client Hardening
10. Phase 68 - Recommendation and Knowledge Graph
```

ADR-0042 remains part of Step 1 and supplies the accepted mutation prerequisite for the remaining contract package.

---

## Documentation Reading Rule

Before proposing frontend, Live-parity, RESTfulAPI, epgsearch, metadata, multi-site or architecture work, inspect:

- `docs/development/current-status.md`
- `docs/planning/roadmap.md`
- `docs/planning/phase-map.md`
- `docs/planning/architecture-audit-gap-matrix.md`
- `docs/development/architecture-source-audit-2026-07-15.md`
- `docs/development/completed-phases.md`
- `docs/planning/parity-audit-and-frontend-gap-roadmap.md`
- `docs/development/client-api-frontend-module-boundary-plan.md`
- `docs/architecture/restfulapi-integration.md`
- `docs/development/epgsearch-capability-matrix.md`
- `docs/adr/index.md`

---

## Boundary Rules

- Completed Phases records finished implementation only.
- The completed source audit records evidence and conclusions only.
- The Architecture Audit Gap Matrix records open, partial and implemented audit gaps.
- The strict roadmap owns future order.
- The older parity matrix owns product and ecosystem parity questions, not architecture sequencing.
- Additional plugin source audits require a concrete feature, adapter, migration or risk question.

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
