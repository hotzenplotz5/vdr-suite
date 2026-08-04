# VDR-Suite Completed Phases

## Purpose

This is the compact authoritative entry point for completed implementation. Detailed historical records remain in [the completed-phase archive](completed-phases/README.md); future numbered work belongs in the strict roadmap.

## Latest completed markers

```text
Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 repository state:
completed and merged through PR #117

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime
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
| Phase 62 | Completed and merged | Persistent identity, scoped RBAC, browser-session lifecycle/CSRF, protected central mutations and append-only authorization/outcome evidence. | [Phase 62 closeout](phase-62-closeout.md) / [Slice 2X runtime closeout](phase-62-slice-2x-runtime-closeout.md) |
| B1-B4 | Completed, non-numbered | EPG/metadata query, transaction, no-op and snapshot-cadence hardening. | [Performance closeout](phase-61-metadata-genre-performance-closeout.md#post-phase-61-performance-hardening) |
| PR #110 | Completed cross-cutting feature | Mobile Remote pressed-state and duplicate-dispatch behaviour. | [Platform closeout](post-phase-61-platform-runtime-closeout.md) |
| PR #111 | Completed cross-cutting feature | Backend-scoped global search over persisted Recording/EPG titles, subtitles and people. | [Platform closeout](post-phase-61-platform-runtime-closeout.md) |
| PR #115 | Completed cross-cutting feature | Configurable photorealistic VDR Remote asset and interaction path. | Repository history |
| PR #118 | Completed post-Phase-62 correction | TVScraper genre classification, overview/detail consistency and low-latency continuation. | Repository history |
| PR #123 | Completed post-Phase-62 correction | EPG artwork resolution beneath configured public base paths. | Repository history |
| PR #132 | Completed post-Phase-62 platform feature | Guarded external series-artwork fallback, TVmaze/TMDB providers, secure backend settings, deterministic provider identity and poster/cover preference. | [Post-Phase-62 Security Review](post-phase-62-security-review.md) / artwork architecture docs |
| `96b97378` + `2d04a963` | Completed frontend correction | Channel-detail text remains beside artwork on wide layouts with mobile reset and regression coverage. | Repository history |

## Phase 62 durable completion marker

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

PR #117 was merged as `f9e5f88bc223a2ce8a30fdbf4596893b34bc1551`. Legacy Basic retirement was evaluated and explicitly deferred to a future deployment-migration contract. The transitional mode does not silently reopen Phase 62.

## Post-Phase-62 evidence boundary

The Phase-62 acceptance is historical evidence for its accepted runtime candidate. Later daemon work has its own CI and functional evidence and must not claim byte identity with the Phase-62 fingerprint.

PR #132 was merged as `441e5febf7d3ab0121a585ce1176a8e5a7c67ce0`. Its final feature head passed VDR-Suite CI #6982 with all five jobs successful, and real yaVDR operation proved persisted TMDB fallback assets and browser delivery. The security impact and remaining focused hardening recommendation are documented in [Post-Phase-62 Security Review](post-phase-62-security-review.md).

## Completion boundaries

- Phase 61 is not reopened by provider adapters, diagnostics or recommendation work.
- Phase 62 is not reopened by later protected feature routes that continue to use its identity, authorization, CSRF and accountability model.
- Historical runtime fingerprints must remain distinguishable from later daemon evidence.
- ADR acceptance remains separate from runtime completion.
- Phase 63-67 runtime has not been advanced.

## Next work

```text
Phase 63 - Backend Agent and Secure Multi-Site Runtime
```

Phase 63 requires a separate bounded contract. Before starting it, complete any selected post-Phase-62 hardening as an explicit, independently tested maintenance change.

## Verification

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

## Related documents

- [Current State](../CURRENT.md)
- [Latest Completed Marker](completed-phases-latest.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Post-Phase-62 Security Review](post-phase-62-security-review.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase 62 Final Closeout](phase-62-closeout.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
