# VDR-Suite New Chat Handoff

## Purpose

This is the canonical operational entry point for every new VDR-Suite chat.

Always verify repository, pull-request, CI and runtime facts against current `main` and the exact active PR head. Recorded hashes are checkpoints only.

## Canonical reading

- [Current State](CURRENT.md)
- [Current Project Status](development/current-status.md)
- [Completed Phases](development/completed-phases.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md)
- [ADR-0044 Timer Model](adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [Architecture Decision Records](adr/index.md)
- [Agent Workflow Rules](../AGENTS.md)

Merged Phase-64 foundation documents:

- [TimerIntent Contract](development/phase-64-timer-intent-contract.md)
- [TimerIntent Repository](development/phase-64-timer-intent-repository.md)

Historical Phase-62/63 closeouts remain evidence for their accepted candidates but are no longer the current work entry point.

## Stable project position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Current merged main checkpoint:
96fab8ad88eae9ea0d46adf4db50ccf8d750a19b

Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Next strict runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Merged Phase-64 foundation:
Slice 1 - TimerIntent Domain Contract, PR #150
Slice 2 - TimerIntent Persistence and Repository Semantics, PR #152

Current stacked Draft tip:
PR #169 - Add NativeTimerBinding absence application
branch: agent/phase64-native-timer-binding-absence-application
head checkpoint: 9e54a1c2c3087f6eb9a9317b5c1f8ab3dd43525e
CI checkpoint: VDR-Suite CI #7413 / run 31463690316; recheck exact current result
runtime change: none; Slice 17 does not wire an installed native Timer mutation path
```

Do not reuse the recorded PR head without checking GitHub again.

## Completed Phase 63 platform boundary

Phase 63 is complete. It established Backend Agent identity/enrollment, protected transport, backend-generation and lease fencing, observation ingestion, durable command/result handling, fenced SuiteBridge native execution, explicit provider ownership/selection and the generic protected-write safety contract. These remain prerequisites for Phase 64.

Phase 63 did not own TimerIntent, TimerAssignment or NativeTimerBinding.

## Historical completed context

The following markers are retained for documentation-entrypoint continuity; they are historical, not the current active phase:

- Phase 58 - Frontend and Live Parity
- Phase 61 - Suite Metadata and Genre Platform
- Post-Phase 61 Performance Hardening (B1-B4)
- VDR Remote and Live Overlay hardening (#110)
- Backend-scoped Global Search (#111)
- Phase 62 - Identity, RBAC and Accountability Foundation

## Phase 64 stack

```text
TimerIntent
  -> contract
  -> durable repository/revisions

TimerAssignment
  -> contract
  -> durable repository/revisions/epochs
  -> deterministic planner
  -> primary scheduling
  -> assignment-set concurrency fence
  -> replica scheduling

NativeTimerBinding
  -> contract
  -> durable optimistic-concurrency repository
  -> VDR NativeTimerObservation mapper
  -> safe present-readback application
  -> expected PRESENT operation evidence
  -> operation-aware PRESENT verification
  -> complete inventory / authoritative absence evidence
  -> failure-aware RESTfulAPI complete-inventory reader
  -> authoritative absence application
```

Exact stacked Draft progression at this checkpoint:

```text
#153 -> #154 -> #155 -> #158 -> #159 -> #160
     -> #161 -> #162 -> #163 -> #164 -> #165 -> #166 -> #167 -> #168 -> #169
```

Draft PR #157 is a separate SQLite architecture-baseline repair. Draft PR #156 is a separate proposed media/player ADR and does not implement Phase-65 runtime.

## Timer safety position at Slice 17

The stack now has:

- separate Suite intent, assignment and native-binding identities;
- intent, assignment-set and binding concurrency fences;
- deterministic generation/evidence-fenced primary and replica planning;
- backend-neutral copied native observations;
- safe present refresh that does not erase unresolved state;
- operation-bound expected PRESENT evidence and authoritative PRESENT verification;
- complete-inventory absence proof that never treats transport/parser failure as absence;
- a failure-aware RESTfulAPI inventory reader;
- durable absence application that preserves the last known present state/fingerprint and first `missingSince`;
- conservative missing-state classification that never invents an external-delete cause;
- reconciliation-required handling when a Timer reappears after durable missing evidence.

No current Phase-64 Draft enables production native Timer create/update/remove or `mutations=enabled`.

## Exact next bounded work

After PR #169, define a separate operation-aware **expected absence** contract for Suite-managed native Timer removal.

The contract must bind:

- operation ID and allowed operation state;
- exact NativeTimerBinding ID and expected binding revision;
- backend ID and backend generation;
- backend-native Timer identity;
- positive `readbackNotBefore`.

Only this operation context plus complete-inventory absence may later verify a removal. The contract slice must not itself classify external change, transition TimerAssignment, execute replacement/failover or perform native mutation.

Operation-aware absence verification, changed-state/external-drift reconciliation, binding/assignment transitions, controlled replacement/failover, protected native Timer execution, daemon orchestration and public Timer surfaces remain later bounded work.

## Phase ordering and broad Timer UI

The strict numbered runtime order is:

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration
  -> Phase 65 - Streaming Gateway and Media Sessions
  -> Phase 66 - Legacy OSD Compatibility Bridge
  -> Phase 67 - Public API and Client Compatibility Hardening
```

Phase 65 follows completion of the reliable Phase-64 Timer **engine**, not completion of a broad polished Timer UI.

Broad Timer mutation controls remain separately gated on account/backend access management built on Phase 62. This frontend/security gate does not block Phase 65 once Phase 64 is complete.

Therefore Streaming Gateway runtime work may intentionally happen before the broad Timer UI is finished. This is project sequencing, not a technical dependency of Streaming on Timer UI.

## Documentation synchronization rule

`NEW-CHAT-HANDOFF.md`, `CURRENT.md` and `development/current-status.md` are the direct operational status entry points.

The current-position markers in `planning/roadmap.md` and `planning/phase-map.md` still lag the active Phase-64 stack. Their architecture and strict phase-order rules remain useful, but stale active-slice markers must not override exact GitHub state. Synchronizing those broader planning files is a separate guard-aware task.

## Current work boundary

- Phase 62 is complete.
- Phase 63 is complete.
- Phase 64 is active.
- Re-read PR #169 before continuation.
- Keep stacked Timer PRs Draft until explicit user approval changes review state.
- Do not jump directly from evidence/reconciliation work to production native mutation.
- Do not infer external change or failover eligibility from transport failure or absence alone.
- Do not create parallel backend, authorization, accountability, job or scheduler authorities.
- Do not mix Phase-65 Streaming runtime into a Phase-64 Timer slice.
- Broad Timer UI work must not bypass the account/backend access-management gate.
- No manual SQLite inspection is required for acceptance.
- Avoid unrelated refactors and cosmetic rewrites in bounded slices.

## Exact next action

1. Re-read PR #169, its exact head, diff, mergeability and CI.
2. Continue only from its exact current head if the stack is unchanged.
3. Add the operation-aware expected-absence contract as its own stacked Draft.
4. Keep absence verification, external-change classification, assignment transitions, failover and native execution out of that contract slice.
5. Evaluate exact-final-head CI according to `AGENTS.md`.
6. Require real yaVDR acceptance only when an installed/runtime boundary changes.
7. Keep review/merge state changes behind explicit user approval.

## Command presentation contract

Every shell command intended for the user to copy or execute must be inside an ordinary fenced Markdown `bash` block.

- Keep explanations outside the block.
- Put complete executable sequences inside the block.
- Preserve checkout-path and repository-identity verification.
- Never hide setup in prose.
- Do not wrap user-facing commands in `set -e`; use explicit error handling.

## Binding daemon build and installation manifest

Every installation answer must be derived from the exact requested branch or PR.

Before writing installation commands:

- resolve the exact current PR head from GitHub;
- inspect the root `Makefile`, included install/build fragments, relevant packaging/systemd files and any changed component Makefile;
- determine exact required build targets;
- determine from the exact diff whether `vdr-plugin-suite-bridge` must be rebuilt;
- never reuse target names, service names, paths or installation options from an older branch without verification.

The user-facing answer must use the heading `## Lokaler Bau, Test und Installation`, followed by at most one short sentence and exactly one ordinary fenced `bash` block.

The established yaVDR daemon flow has this required structure:

```bash
cd /home/yavdr/vdr-suite

git switch <exact-branch>
git pull --ff-only origin <exact-branch>

git rev-parse HEAD
test "$(git rev-parse HEAD)" = "<exact-head-sha>" || exit 1

make clean
make -j2 --output-sync=target <exact-required-build-targets>

systemctl stop vdr-suite-daemon
make install PREFIX=/usr
systemctl daemon-reload
systemctl restart vdr-suite-daemon

systemctl is-active vdr-suite-daemon
systemctl --no-pager --full status vdr-suite-daemon
```

The placeholders document structure only. In a user-facing installation answer they must always be replaced with exact verified values.

Do not add package-manager setup, a second clone, unrelated diagnostics or broad local regressions unless the exact change or an observed failure requires them.

### Conditional SuiteBridge installation

Do not rebuild the plugin merely because it exists. Inspect the exact PR diff and dependency contract first. If the plugin is required, inspect its exact-head Makefile and the established VDR service/layout values before presenting commands. Never guess plugin paths, API version or VDR service unit.

## Installed-result acceptance manifest

Daemon startup alone does not prove feature acceptance. For a runtime-changing PR, the follow-up section `## Prüfung des installierten Ergebnisses` must cover the relevant exact-head layers:

1. installed identity/startup;
2. positive feature path;
3. readback plus persistence/restart;
4. changed search/presentation paths;
5. replacement/withdrawal semantics when applicable;
6. authorization/failure boundaries;
7. nearest adjacent regression;
8. exact source/CI/build and redacted observed evidence.

Only include layers the exact PR can affect, but do not omit its primary persistence/restart and regression boundaries merely for brevity. Never claim functional acceptance only because CI or service startup is green.

## Credential and secret restrictions

Never print, store or commit authentication headers, plaintext credentials, browser session material, anti-forgery values, provider tokens, enrollment secrets, Agent credential secrets or secret-bearing process environments.
