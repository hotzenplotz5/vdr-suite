# VDR-Suite

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. VDR remains the native runtime authority; VDR-Suite owns backend scope, policy, orchestration, persistent read models and client-facing contracts.

## Start here

- [Current State](docs/CURRENT.md)
- [New Chat Handoff](docs/NEW-CHAT-HANDOFF.md)
- [Documentation Index](docs/index.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Completed History](docs/development/completed-phases.md)
- [Phase 62 Final Closeout](docs/development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](docs/development/phase-62-slice-2x-runtime-closeout.md)
- [VDR Ecosystem Parity](docs/planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](docs/adr/index.md)

## Current verified position

Repository development baseline for PR #117 remains:

```text
main @ cb77ff66e11dca7db2eafa36525762dcde35102d
```

```text
Latest completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Previous completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next strict runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active runtime phase:
none; Phase 63 is planned but not started
```

## Final Phase 62 runtime marker

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
accepted_runtime_head=4762583d5b5170866838ed9f03b928adbf39f99e
source_ci_run_number=6884
source_ci_run_id=30752351218
daemon_sha256=488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5
runtime_report_sha256=bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf
evidence_directory=/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

Phase 62 provides persistent actor/device/session/credential identity, exact backend-scoped authorization, fixed Admin/Read-only roles, browser-session lifecycle and CSRF policy, complete central POST classification, append-only authorization evidence and protected mutation success/failure outcomes.

Legacy Basic remains an explicitly transitional compatibility mode. Its removal requires a future deployment-migration contract because `legacy-basic` remains the code default and packaged deployments do not yet mandate migration to `enforced`.

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
- protected Remote, Timer, Channel Move, Recording, SearchTimer, Native Fuzzy and cache-refresh mutation families;
- append-only allow/deny and returned-outcome accountability;
- modular frontend ownership through `VdrSuiteClientApi`, without direct browser access to private providers.

Streaming Gateway, legacy OSD compatibility, secure Backend Agents, central TimerIntent orchestration and stable public API hardening remain later phases. They are not implemented by Phase 62.

## Architecture direction

Accepted ADRs define target contracts; they do not by themselves prove runtime implementation. Current implementation truth comes from repository code, complete CI and recorded real-system acceptance.

The strict next step is:

```text
Phase 63 - Backend Agent and Secure Multi-Site Runtime
```

Phase 63 must begin with a new bounded contract after PR #117 disposition. Phase-62 actor representation alone does not count as Phase-63 runtime implementation.
