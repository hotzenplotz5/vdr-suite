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
Backend Foundation        complete
Snapshot Runtime          complete
Read API                  complete
Change Feed               complete
Live Transport            planned
```

### Federation and Security

```text
Multi-VDR                 architecture prepared
Capability System         foundation implemented
Authentication            planned
Authorization             planned
```

### Client Platforms

```text
Web Frontend              planned
Windows Frontend          planned
Android Frontend          planned
iOS Frontend              planned
```

### Media Extensions

```text
Image Validation          planned
Preview Streams           planned
Media Streaming           planned
```

---

## Current Position

Current Major Phase:

```text
Phase 13.7e Complete
```

Current Focus:

```text
Phase 13.8 - Live Transport Foundation
```

Latest Completed Milestone:

```text
Phase 13.7e - Snapshot Cache Generation Tracking
```

---

## Roadmap Progress

Completed Major Phases:

```text
Phase 0 - Phase 13.7e
```

Planned Major Phases:

```text
Phase 13.8 - Phase 16
```

Overall Roadmap Progress:

```text
14 / 17 = 82.4%
```

Important:

This percentage describes documented roadmap progress by major phase. It is not a code coverage metric and not a production-readiness guarantee.

---

## Status Meaning

| Area | Meaning |
| --- | --- |
| Backend Foundation | Core database, services, jobs, dashboard, REST and daemon foundations are implemented. |
| Snapshot Runtime | Daemon-owned snapshot generation, cache, access boundaries and generation tracking are implemented. |
| Read API | Snapshot-backed read APIs for current VDR domains are implemented. |
| Change Feed | Transport-independent snapshot change feed and runtime feed update integration are implemented. |
| Live Transport | SSE/WebSocket transport remains planned above the snapshot change feed. |
| Multi-VDR | Architecture is considered and documented, but backend routing remains future work. |
| Capability System | Capability set and resolver foundations are implemented. |
| Authentication | Planned future concern, not implemented yet. |
| Client Platforms | Future frontend work, not implemented yet. |
| Media Extensions | Image, preview stream and media stream validation remain future work. |

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
