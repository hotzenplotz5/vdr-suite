# VDR-Suite

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. VDR remains the native runtime authority; VDR-Suite owns backend scope, policy, orchestration, persistent read models and client-facing contracts.

## Start here

- [Current State](docs/CURRENT.md)
- [New Chat Handoff](docs/NEW-CHAT-HANDOFF.md)
- [Current Project Status](docs/development/current-status.md)
- [Post-Phase-62 Security Review](docs/development/post-phase-62-security-review.md)
- [Documentation Index](docs/index.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Completed History](docs/development/completed-phases.md)
- [Phase 62 Final Closeout](docs/development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](docs/development/phase-62-slice-2x-runtime-closeout.md)
- [Architecture Decision Records](docs/adr/index.md)

## Current verified position

```text
Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 repository state:
completed and merged through PR #117

Previous completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed cross-cutting platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
none; Phase 63 is planned but not started
```

PR #117 was merged as `f9e5f88bc223a2ce8a30fdbf4596893b34bc1551`.

The post-Phase-62 runtime/frontend baseline before the current documentation refresh is:

```text
2d04a963054e9925f6b8cb12392b188a89e11f07
```

## Phase 62 completion evidence

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

This remains the durable completion evidence for the accepted Phase-62 runtime. Later daemon builds have separate CI and functional evidence and are not byte-identical to this historical fingerprint.

Phase 62 provides persistent actor/device/session/credential identity, exact backend-scoped authorization, fixed Admin/Read-only roles, browser-session lifecycle and CSRF policy, complete central POST classification, append-only authorization evidence and protected mutation success/failure outcomes.

Legacy Basic remains an explicitly transitional compatibility mode. Its removal requires a future deployment-migration contract because `legacy-basic` remains the compatibility default and packaged deployments do not yet mandate migration to `enforced`.

## Completed post-Phase-62 work

Current accepted development additionally includes:

- TVScraper genre-classification and refresh corrections from PR #118;
- EPG artwork resolution beneath configured public base paths from PR #123;
- guarded external series-artwork fallback from PR #132;
- TVmaze and TMDB provider integration without exposing provider credentials to the browser;
- secure per-backend provider settings and private managed TMDB-token storage;
- deterministic TVDB/TMDB series identity preservation through SuiteBridge;
- series/season cover preference before episode imagery;
- poster-first TMDB selection with deterministic backdrop fallback;
- channel-detail artwork/text layout correction with regression coverage.

PR #132 was merged as:

```text
441e5febf7d3ab0121a585ce1176a8e5a7c67ce0
```

Its final feature head passed VDR-Suite CI #6982 with all five jobs successful. Real yaVDR operation proved multiple persisted `provider=tmdb` fallback assets, valid cached image files and successful browser delivery.

## Security position

The new series-artwork settings POST is integrated into the Phase-62 protected-mutation model with a dedicated backend-scoped permission, fixed Admin/Read-only semantics, browser CSRF, pre-dispatch accountability and post-dispatch success/failure outcomes. Managed TMDB tokens are stored beneath a private secret root and are neither returned by the API nor copied into accountability events.

No known authentication, authorization, CSRF, Read-only-role or cross-backend write bypass was introduced by the completed post-Phase-62 work. The historical Phase-62 acceptance remains historical rather than a current daemon fingerprint. See [Post-Phase-62 Security Review](docs/development/post-phase-62-security-review.md).

## Implemented runtime blocks

Current accepted development includes:

- daemon-owned SQLite persistence, backend registry, snapshots, change feed and server-enforced read-only backend policy;
- channels, EPG timeline and channel-day programme views;
- Recordings 2 with metadata, people, artwork, Genre integration and guarded actions;
- SearchTimer list, discovery, preview, validation and protected mutation foundations;
- persistent backend-scoped Recording and EPG metadata, people, Genre evidence, assignments and browse paths;
- post-Phase-61 query, transaction, no-op and snapshot-cadence hardening;
- backend-neutral VDR remote actions and live-overlay snapshots;
- backend-scoped global search;
- persistent identity and lifecycle resolution;
- optional Managed Basic and browser sessions with strict credential precedence;
- exact actor/backend grants, fixed roles and server-owned browser CSRF;
- protected Remote, Timer, Channel Move, Recording, SearchTimer, Native Fuzzy, cache-refresh and series-artwork-settings mutation families;
- append-only allow/deny and returned-outcome accountability;
- modular frontend ownership through `VdrSuiteClientApi`, without direct browser access to private providers.

Streaming Gateway, legacy OSD compatibility, secure Backend Agents, central TimerIntent orchestration and stable public API hardening remain later phases. They are not implemented by Phase 62 or the completed artwork work.

## Architecture direction

Accepted ADRs define target contracts; they do not by themselves prove runtime implementation. Current implementation truth comes from repository code, complete CI and recorded real-system acceptance.

The strict next numbered step is:

```text
Phase 63 - Backend Agent and Secure Multi-Site Runtime
```

Phase 63 must begin with a new bounded contract. Before that, any selected post-Phase-62 security hardening should remain an explicit, independently tested maintenance change.
