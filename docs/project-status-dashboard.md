# VDR-Suite Project Status Dashboard

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)
- [Current Status](development/current-status.md)
- [Roadmap](planning/roadmap.md)
- [Architecture](architecture/index.md)
- [ADR](adr/index.md)

---

## Current Release State

### Core Platform

```text
Backend Foundation        ████████████ 100%
Snapshot Runtime          ████████████ 100%
Read API                  ████████████ 100%
Change Feed               ████████████ 100%
Live Transport            ░░░░░░░░░░░░   0%
```

### Federation and Security

```text
Multi-VDR                 ██░░░░░░░░░░  15%
Capability System         ░░░░░░░░░░░░   0%
Authentication            ░░░░░░░░░░░░   0%
Authorization             ░░░░░░░░░░░░   0%
```

### Client Platforms

```text
Web Frontend              ░░░░░░░░░░░░   0%
Windows Frontend          ░░░░░░░░░░░░   0%
Android Frontend          ░░░░░░░░░░░░   0%
iOS Frontend              ░░░░░░░░░░░░   0%
```

### Media Extensions

```text
Image Validation          ░░░░░░░░░░░░   0%
Preview Streams           ░░░░░░░░░░░░   0%
Media Streaming           ░░░░░░░░░░░░   0%
```

---

## Current Position

Current Major Phase:

```text
Phase 12 Complete
```

Current Focus:

```text
Phase 13 - Live Update Transport
```

Latest Completed Milestone:

```text
Phase 12.3 - Snapshot Change Feed REST Controller
```

---

## Roadmap Progress

Completed Major Phases:

```text
Phase 0 - Phase 12
```

Planned Major Phases:

```text
Phase 13 - Phase 16
```

Overall Roadmap Progress:

```text
13 / 17 = 76.5%
```

Important:

This percentage describes documented roadmap progress by major phase. It is not a code coverage metric and not a production-readiness guarantee.

---

## Status Meaning

| Area | Meaning |
| --- | --- |
| Backend Foundation | Core database, services, jobs, dashboard, REST and daemon foundations are implemented. |
| Snapshot Runtime | Daemon-owned snapshot generation, cache and access boundaries are implemented. |
| Read API | Snapshot-backed read APIs for current VDR domains are implemented. |
| Change Feed | Transport-independent snapshot change feed is implemented. |
| Live Transport | SSE/WebSocket transport is planned for Phase 13. |
| Multi-VDR | Architecture is considered, but backend routing is planned for Phase 14. |
| Capability System | Planned architecture area, not implemented yet. |
| Authentication | Planned future concern, not implemented yet. |
| Client Platforms | Future frontend work, not implemented yet. |
| Media Extensions | Image and preview stream validation is planned for Phase 16. |

---

## Primary Documentation Entry Points

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)
- [Current Status](development/current-status.md)
- [Roadmap](planning/roadmap.md)
- [Architecture](architecture/index.md)
- [ADR](adr/index.md)

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Project Overview](project-overview.md)
