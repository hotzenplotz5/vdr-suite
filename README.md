# VDR-Suite

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. VDR remains the native runtime authority; VDR-Suite owns backend scope, policy, orchestration, persistent read models and client-facing contracts.

## Start here

- [Current State](docs/CURRENT.md)
- [New Chat Handoff](docs/NEW-CHAT-HANDOFF.md)
- [Current Project Status](docs/development/current-status.md)
- [Documentation Index](docs/index.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Completed History](docs/development/completed-phases.md)
- [Post-Phase-62 Security Review](docs/development/post-phase-62-security-review.md)
- [Phase 62 Final Closeout](docs/development/phase-62-closeout.md)
- [Architecture Decision Records](docs/adr/index.md)

## Current verified position

```text
Current merged main checkpoint:
96fab8ad88eae9ea0d46adf4db50ccf8d750a19b

Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Previous completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Next strict runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Merged Phase-64 foundation:
PR #150 - TimerIntent Domain Contract
PR #152 - TimerIntent Persistence and Repository Semantics

Current stacked Draft tip checkpoint:
PR #169 - NativeTimerBinding absence application
head 9e54a1c2c3087f6eb9a9317b5c1f8ab3dd43525e
```

The exact current branch, PR head and CI must be re-read from GitHub before work resumes; the values above are status checkpoints, not permanent authority.

## Completed platform foundations

Phase 62 established persistent actor/device/session/credential identity, exact backend-scoped authorization, fixed Admin/Read-only roles, browser-session lifecycle and CSRF policy, complete central mutation classification, append-only authorization evidence and protected mutation outcomes.

Legacy Basic remains an explicitly transitional compatibility mode. Its removal requires a separate deployment-migration contract.

Phase 63 completed the secure multi-site execution foundation:

- Backend Agent enrollment, technical identity and protected transport;
- credential lifecycle, lease and backend-generation fencing;
- generation-/sequence-fenced observation ingestion;
- durable Agent command, receipt, result and reconciliation flow;
- fenced SuiteBridge native execution and authoritative readback;
- explicit local-provider facts, ownership and immutable provider selection with no silent fallback;
- the generic protected-write safety contract used by later domain mutations.

Historical acceptance evidence remains in the phase closeout documents and is evidence for its exact accepted runtime candidate only.

## Historical completed context

The following completed predecessors remain part of the project history:

- Phase 61 - Suite Metadata and Genre Platform
- Post-Phase 61 Performance Hardening (B1-B4)

## Active Phase 64

Phase 64 is active and deliberately separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
  -> authoritative readback/reconciliation
  -> later protected native Timer execution
```

The merged foundation is the TimerIntent contract and repository. The active stacked Drafts add TimerAssignment persistence and scheduling, assignment-set concurrency fencing, replica scheduling, NativeTimerBinding persistence, backend-neutral native Timer observations, operation-aware PRESENT verification, complete-inventory absence proof, failure-aware inventory acquisition and durable absence application.

The current stack tip is Draft PR #169. Production central native Timer create/update/remove remains deferred until the remaining verification, reconciliation and handover safety slices are complete.

## Implemented runtime blocks

Current accepted development includes:

- daemon-owned SQLite persistence, backend registry, snapshots, change feed and server-enforced read-only backend policy;
- channels, EPG timeline and channel-day programme views;
- Recordings 2 with metadata, people, artwork, Genre integration and guarded actions;
- SearchTimer list, discovery, preview, validation and protected mutation foundations;
- persistent backend-scoped Recording and EPG metadata, people, Genre evidence, assignments and browse paths;
- backend-neutral VDR remote actions and live-overlay snapshots;
- backend-scoped global search;
- persistent identity, authorization, browser-session and accountability foundations;
- secure Backend Agent lifecycle, observations, commands, native-operation fencing and explicit provider ownership;
- modular frontend ownership through `VdrSuiteClientApi`, without direct browser access to private providers.

## Timer UI and Streaming order

The broad Timer UI is not the Phase-64 completion gate. Broad Timer mutation controls remain separately gated on account/backend access management built on Phase 62.

The strict numbered sequence is:

```text
Phase 64 Timer orchestration engine
  -> Phase 65 Streaming Gateway
  -> Phase 66 Legacy OSD bridge
  -> Phase 67 Public API/client hardening
```

Phase 65 may begin after the reliable Phase-64 Timer engine is complete even if the broad Timer UI remains deferred. Streaming is not technically dependent on the broad Timer UI.

## Architecture direction

Accepted ADRs define target contracts; they do not by themselves prove runtime implementation. Current implementation truth comes from repository code, exact-head CI and recorded real-system acceptance where runtime behaviour changes.

The strict current numbered step is:

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration
```

After Phase 64 completes, the strict next numbered runtime phase is Phase 65 Streaming Gateway and Media Sessions.
