# VDR-Suite Completed Phases

## Purpose

This is the compact authoritative entry point for completed implementation. Detailed historical records remain in [the completed-phase archive](completed-phases/README.md); future work belongs in the strict roadmap.

## Latest completed markers

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

## Completed range overview

| Range / block | Status | Result | Archive / closeout |
| --- | --- | --- | --- |
| Phase 1.x-45.x | Completed | Core platform, database, daemon, VDR adapter, multi-backend reads, Recording actions, runtime hardening and EPG search. | Historical phase records |
| Phase 46 | Completed | Metadata and people foundations. | [Phase 46](completed-phases/phase-46.md) |
| Phase 47-50 | Completed | SearchTimer backend, REST/native validation and controlled workflow. | [Archive](completed-phases/README.md) |
| Phase 51-55 | Completed | Live parity discovery, SearchTimer preview, adapter/runtime hardening and acceptance. | [Archive](completed-phases/README.md) |
| Phase 56 | Completed | Library boundaries, packaging and developer documentation. | [Phase 56](completed-phases/phase-56.md) |
| Phase 57 | Completed | Multi-site backend administration and server-enforced read-only foundation. | [Phase 57](completed-phases/phase-57.md) |
| Phase 58 | Completed slices; historical umbrella retained | Frontend and Live-parity foundation slices. | [Phase 58](completed-phases/phase-58.md) |
| Phase 59.00-59.15e | Completed | Frontend Client API and module ownership. | [Phase 59](completed-phases/phase-59.md) |
| Phase 60.1-60.15 | Completed | Frontend platform, lazy Recording cache, Recordings 2, metadata and authenticated artwork preparation. | [Phase 60](completed-phases/phase-60.md) |
| Phase 61 | Completed | Persistent Recording/EPG metadata, people and Genre platform, query-only browse paths and frontend integration. | [Phase 61 archive](completed-phases/phase-61.md) / [closeout](phase-61-metadata-genre-performance-closeout.md) |
| B1-B4 | Completed, non-numbered | EPG/metadata query, transaction, no-op and snapshot-cadence hardening. | [Performance closeout](phase-61-metadata-genre-performance-closeout.md#post-phase-61-performance-hardening) |
| PR #110 | Completed cross-cutting feature | Current mobile Remote pressed-state and duplicate-dispatch behaviour. | [Platform closeout](post-phase-61-platform-runtime-closeout.md) |
| PR #111 | Completed cross-cutting feature | Backend-scoped global search over persisted Recording/EPG titles, subtitles and people. | [Platform closeout](post-phase-61-platform-runtime-closeout.md) |

## Completion boundaries

- Phase 61 is not reopened by optional provider adapters, diagnostics or recommendation work.
- PR #110 interaction behaviour is complete; the competing asset drafts #112/#113 remain separate open work.
- Global search is a completed cross-cutting platform slice, not a newly invented numbered phase.
- ADR acceptance remains separate from runtime completion.

## Next work

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

## Verification

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

## Related documents

- [Current State](../CURRENT.md)
- [Latest Completed Marker](completed-phases-latest.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)