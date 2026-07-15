# VDR-Suite Current State

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR Index](adr/index.md)
- [Completed Phases](development/completed-phases.md)

---

## Current Verified State

Current completed major project block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Current umbrella implementation track:

```text
Phase 58 - Frontend and Live Parity
```

Latest completed implementation slice:

```text
Phase 60.14k - Recording Detail UX Polish
```

Next planned implementation slice:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

Current architecture documentation package:

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
```

---

## Phase Numbering Note

Phase 58 remains the current broad frontend and Live-parity track.

The completed 59.x and 60.x implementation slices are concrete frontend API, module, platform, recording-cache and recording-UX work performed under that continuing product track.

Future major milestones that previously reused Phase 59 and Phase 60 numbers are moved to conflict-free Phase 61 and Phase 62 planning slots.

---

## Documentation Reading Rule

Before proposing frontend, Live-parity, RESTfulAPI, epgsearch, metadata or architecture work, first inspect:

- `docs/development/current-status.md`
- `docs/planning/phase-map.md`
- `docs/planning/roadmap.md`
- `docs/planning/parity-audit-and-frontend-gap-roadmap.md`
- `docs/development/client-api-frontend-module-boundary-plan.md`
- `docs/architecture/restfulapi-integration.md`
- `docs/development/epgsearch-capability-matrix.md`
- `docs/adr/index.md`

---

## Next Work

Next implementation focus:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

Architecture follow-up after the first audit ADR package:

```text
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
```

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
