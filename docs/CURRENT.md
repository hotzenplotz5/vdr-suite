# VDR-Suite Current State

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR-0044 Timer Intent, Assignment and Native Timer Model](adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway and Media Session Boundary](adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [Architecture Decision Records](adr/index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main baseline:
96fab8ad88eae9ea0d46adf4db50ccf8d750a19b

Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Merged Phase-64 foundation:
Slice 1 - TimerIntent Domain Contract, PR #150
Slice 2 - TimerIntent Persistence and Repository Semantics, PR #152

Current active stacked Draft tip:
PR #168 - Add failure-aware RESTfulAPI native Timer inventory reader
branch: agent/phase64-restfulapi-native-timer-inventory-reader
head: a49a3b99167e8f1bbbbf220657d774aaf4038501
state: open Draft, mergeable
CI: VDR-Suite CI #7412 / run 31462922497, all five jobs successful
installed runtime change: none; the Slice-16 reader is not wired into mk/vdr-sources.mk
```

Repository and pull-request facts must still be re-read from GitHub before work resumes. A SHA recorded here is a checkpoint, not a substitute for exact-current-head verification.

## Phase 64 stack now established

The active Timer stack separates Suite intent, scheduler ownership and backend-native Timer state rather than treating a native VDR Timer as the global product identity.

The current stacked Draft sequence is:

```text
#153  TimerAssignment domain contract
#154  TimerAssignment persistence repository
#155  deterministic TimerAssignment planner
#158  primary assignment scheduling handoff
#159  assignment-set revision concurrency fence
#160  replica assignment scheduling handoff
#161  NativeTimerBinding domain contract
#162  NativeTimerBinding persistence repository
#163  VDR -> NativeTimerObservation mapper
#164  safe present-readback application
#165  operation-bound expected PRESENT readback evidence
#166  operation-aware PRESENT readback verification
#167  complete native Timer inventory / authoritative absence evidence
#168  failure-aware RESTfulAPI complete-Timer-inventory reader
```

Draft PR #157 is a separate repository-wide SQLite architecture-baseline repair and is not part of the Timer feature stack. Draft PR #156 is a separate proposed client playback/media-adaptation ADR and does not implement Phase-65 runtime.

## Current Timer safety position

The stack already provides the core safety prerequisites for later orchestration:

- stable `TimerIntent`, `TimerAssignment` and `NativeTimerBinding` identities remain distinct;
- TimerIntent and assignment persistence use repository-issued optimistic-concurrency revisions;
- primary and replica planning is deterministic and generation/evidence fenced;
- assignment-set concurrency prevents stale replica-set decisions from being committed silently;
- native Timer observations are backend-neutral copied evidence rather than VDR pointers or provider objects;
- safe present readback does not overwrite unresolved changed/missing state;
- an expected mutation result is bound to exact operation, binding revision, backend generation, native Timer identity, time fence and normalized fingerprint;
- successful present verification records only evidence that has actually been observed after the operation;
- absence can be proven only by an explicitly complete, successful, generation-fenced Timer inventory;
- HTTP, parse or incomplete-read failure can never become Timer absence evidence;
- the RESTfulAPI evidence reader remains test-only/provider-side and does not yet change installed daemon behaviour.

No current Phase-64 Draft enables production native Timer create/update/delete or `mutations=enabled`.

## Exact next bounded work

After PR #168, the next bounded slice is a backend-neutral NativeTimerBinding absence-application service.

It must consume current durable binding state plus one valid complete inventory evidence value and:

- persist authoritative missing observation evidence only;
- preserve the last known present representation and fingerprint;
- require exact backend/generation/time fences;
- use the exact current binding revision and surface optimistic-concurrency conflict without hidden retry;
- classify the cause conservatively rather than guessing `external_delete`.

Operation-aware delete verification, changed-state/external-drift classification, assignment/binding lifecycle transitions, controlled replacement/failover, concrete native Timer mutation, daemon orchestration and public Timer mutation surfaces remain later bounded work.

## Phase ordering and Timer UI gate

The strict numbered runtime order remains:

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration
  -> Phase 65 - Streaming Gateway and Media Sessions
  -> Phase 66 - Legacy OSD Compatibility Bridge
  -> Phase 67 - Public API and Client Compatibility Hardening
```

A broad polished Timer UI is **not** the Phase-64 completion gate. It remains separately gated on account/backend access management built on the completed Phase-62 identity, session, authorization and backend-scoped permission model.

Therefore Phase 65 Streaming may deliberately begin after the Phase-64 Timer engine is complete even if the broad Timer UI is still deferred. Streaming is not technically dependent on the Timer UI, and the Timer UI is not a prerequisite for Phase 65.

Existing legacy/current Timer read/action compatibility surfaces are not reclassified as the final Phase-64 public Timer product.

## Current security and authority position

- Phase-62 identity, authorization, browser-session security, CSRF and accountability remain authoritative.
- Phase-63 Agent identity, backend generation, command/replay fencing and explicit provider ownership/selection remain authoritative prerequisites for protected remote execution.
- Runtime Agent credentials are technical identities and cannot act as browser users or unrestricted administrators.
- Provider availability is descriptive and never silently grants execution authority.
- Unknown or stale generation/revision/evidence fails closed.
- Secrets, Authorization headers, cookies, CSRF values, provider credentials and secret-bearing process environments must not be printed or committed.
- TVScraper remains an unchanged upstream dependency; Suite code does not write TVScraper-owned databases or caches.

## Documentation synchronization note

`CURRENT.md`, `NEW-CHAT-HANDOFF.md` and `development/current-status.md` are the direct operational status entry points.

Some older current-position blocks in `planning/roadmap.md` and `planning/phase-map.md` still lag the active Phase-64 stacked Drafts. Their architecture and strict phase-order rules remain useful, but an older recorded active-slice marker must not override the exact GitHub PR stack or this synchronized current-state checkpoint. Synchronizing those broader planning documents is a separate guarded documentation step.
