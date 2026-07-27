# VDR-Suite Project Status Dashboard

## Current position

```text
Latest completed numbered runtime phase: Phase 61 - Suite Metadata and Genre Platform
Completed hardening: Post-Phase 61 Performance Hardening (B1-B4)
Completed cross-cutting features: Remote/Live Overlay (#110), Global Search (#111)
Historical umbrella: Phase 58 - Frontend and Live Parity
Next strict runtime phase: Phase 62 - Identity, RBAC and Accountability Foundation
```

## Runtime dashboard

| Area | Current status | Evidence / boundary |
| --- | --- | --- |
| Core daemon, SQLite, repositories | Implemented foundation | Domain-owned persistence and migrations. |
| Backend registry and read-only policy | Implemented | Backend scope and server-enforced read-only access. |
| Channels and EPG | Strong | Timeline, channel-day views, cache, details and Genre paths. |
| Recordings | Strong | Recordings 2 is the sole delivered browser and detail/action owner. |
| Recording actions | Strong foundation | Preview, validation, policy, execution and readback; universal mutation contract remains incomplete. |
| SearchTimer | Strong foundation | List, discovery, preview, validation and controlled mutation; edge-semantic parity remains partial. |
| Metadata, people, artwork and Genres | Implemented Phase 61 slice | Persistent backend-scoped read models and provider-neutral GET paths. |
| EPG/metadata performance | Completed B1-B4 | PRs #102-#108. |
| VDR remote and live overlay | Implemented current interaction contract | PRs #99 and #110; competing asset drafts #112/#113 remain open. |
| Backend-scoped global search | Implemented | PR #111; query-only, provider-free reads and 174,164-event regression. |
| User identity, RBAC, accountability | Next | Phase 62. |
| Secure Backend Agents | Planned | Phase 63. |
| TimerIntent orchestration | Planned | Phase 64. |
| Streaming Gateway | Planned | Phase 65. |
| Legacy OSD bridge | Planned | Phase 66. |
| Stable public `/api/v1` | Planned | Phase 67. |
| Recommendations / knowledge graph | Later vision | Phase 68. |

## Current cautions

- The main recording-person contract is 128 people / 65,535 bytes; Draft PR #101 is not compatible as-is.
- Draft PR #109 is an old-base documentation proposal and is superseded by the current truth-refresh work.
- Draft PRs #112 and #113 are competing remote asset proposals; select at most one after rebase and mobile acceptance.
- Accepted ADRs define target contracts but do not automatically mark runtime gaps implemented.

## Links

- [Current State](CURRENT.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase 61 Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Closeout](development/post-phase-61-platform-runtime-closeout.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)