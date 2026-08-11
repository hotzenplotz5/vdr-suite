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

## Current security position

- Phase-62 actor identity, scoped authorization, browser-session lifecycle, CSRF and accountability remain authoritative.
- Phase-63 Agent identity, backend generation, command/replay fencing and explicit provider ownership/selection remain authoritative for remote execution.
- Runtime Agent credentials cannot act as browser users or unrestricted administrators.
- Provider reachability/availability never creates execution authority and active work never silently switches provider.
- Unknown or stale generation, revision, assignment-set or readback evidence fails closed.
- An uncertain native dispatch is reconciled; it is not blindly retried.
- Bootstrap/runtime secrets, hashes/verifiers, Authorization headers, cookies, CSRF values, provider credentials, local secret paths and secret-bearing process environments must not be printed, committed or copied into public responses/accountability events.
- TVScraper remains unchanged upstream; do not write to TVScraper-owned databases or caches.

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and intentionally retained. `enforced` mode is the fail-closed target. Removing Legacy Basic requires a separate deployment-migration contract and is not unfinished Phase 62, Phase 63 or Phase 64.

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
- Do not create a second BackendRegistry, authorization service, accountability store, job system or scheduler authority.
- Unknown central POST routes remain subject to the Phase-62 fail-closed policy outside explicit Legacy Basic compatibility.
- Do not mix Phase-65 Streaming runtime into a Phase-64 Timer slice.
- Broad Timer UI work must not bypass the account/backend access-management gate.
- Do not require manual SQLite inspection for acceptance.
- Do not add unrelated refactors or cosmetic rewrites.

## Exact next action

1. Re-read PR #169, its exact head, diff, mergeability and CI.
2. Continue only from its exact current head if the stack is unchanged.
3. Add the operation-aware expected-absence contract as its own stacked Draft.
4. Keep absence verification, external-change classification, assignment transitions, failover and native execution out of that contract slice.
5. Evaluate exact-final-head CI according to `AGENTS.md`.
6. Require real yaVDR acceptance only when an installed/runtime boundary changes.
7. Keep review/merge state changes behind explicit user approval.

## Command presentation contract

Every shell command intended for the user to copy or execute must be presented inside a normal fenced Markdown code block, preferably tagged `bash`.

- Never place executable commands in prose, inline-code fragments, writing blocks, generated UI controls or custom code-block formats with IDs or metadata.
- Keep explanations outside the code block.
- Put complete, directly executable command sequences inside the code block.
- Use separate code blocks for logically separate steps when that improves safe execution.
- Preserve explicit checkout-path and repository-identity verification; never hide required setup in surrounding prose.
- When the user asks for build, test, installation, rollback or diagnostic commands, the final answer must contain those commands in ordinary copyable Markdown code blocks.
- Do not use `set -e` or another user-facing errexit wrapper; use explicit error handling.

## Binding daemon build and installation manifest

Every installation answer must be generated for the exact requested branch or pull request. Generic installation instructions and commands copied from another PR are forbidden.

Before producing the command block, the agent must:

- resolve the exact requested PR and branch from GitHub;
- resolve the exact current PR head SHA immediately before presenting the commands;
- inspect the root `Makefile`, all included install/build makefiles such as `mk/install.mk`, relevant packaging and systemd files, and any component-specific Makefile changed or required by that exact head;
- determine the exact build targets and whether `vdr-plugin-suite-bridge` must be rebuilt and installed;
- never infer installation options, target names, plugin paths or service names from an older branch, older PR, README excerpt or previous chat.

The answer must use the heading `## Lokaler Bau, Test und Installation`, followed by at most one short sentence and exactly one ordinary fenced Markdown `bash` block without IDs, attributes or metadata.

For the established yaVDR checkout, the mandatory daemon flow has this shape:

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

The placeholders above define the required structure only. In a user-facing answer they must always be replaced with the exact branch, exact current head SHA and exact build targets for the requested branch or PR. Never leave placeholders in executable instructions.

Additional binding rules:

- `git pull --ff-only origin <exact-branch>` is mandatory for the established checkout.
- The exact SHA guard is mandatory and must abort before build or installation when the checkout does not match the verified PR head.
- Use `make clean` followed by `make -j2 --output-sync=target` with only the targets actually required by that branch or PR.
- Do not add `sudo` merely as a style preference; preserve the established host execution context unless the user explicitly requests a non-root form.
- Stop the daemon before `make install PREFIX=/usr`, then reload systemd, restart the daemon and show both `is-active` and the full service status.
- Do not add package-manager commands, dependency bootstrapping, a second clone, backups, rollback scripts, HTTP checks, browser checks or unrelated diagnostics unless explicitly requested or proven necessary by an observed failure.
- Keep the answer branch-/PR-specific and as short as the complete safe flow permits.

### Conditional SuiteBridge plugin installation

The plugin must not be rebuilt merely because it exists in the repository.

- First inspect the exact PR diff and component dependency contract.
- When the PR does not change `vdr-plugin-suite-bridge` and the runtime change does not require a new plugin binary or contract, omit all plugin build and installation commands.
- When the plugin is required, inspect the exact-head `vdr-plugin-suite-bridge/Makefile`, VDR `pkg-config` values and the established yaVDR service layout before writing commands.
- Add the exact plugin clean/build/install and required VDR service stop/restart/status commands to the same branch-/PR-specific Bash block.
- Never guess `VDRDIR`, `LIBDIR`, `APIVERSION`, destination paths or the VDR service unit name, and never reuse plugin commands from another PR without verifying them against the requested head.

This manifest overrides any tendency to provide generic setup instructions, a fresh-system installation tutorial, commands from a previous PR, or prose instead of one directly copyable branch-/PR-specific shell block.

## Binding branch- and PR-specific installed-result acceptance manifest

A successful build, file installation and `active (running)` service state prove only that deployment completed. They do not prove that the requested PR behavior works. Every installation answer must therefore be followed by a branch- or PR-specific acceptance section derived from the exact current head.

Before writing the acceptance steps, the agent must inspect:

- the exact PR diff and changed components;
- the current feature, ADR and runtime-acceptance documents;
- changed REST routes, persistence/schema behavior, frontend paths, services and plugin contracts;
- the closest existing regression behavior that the PR could unintentionally break.

The user-facing answer must use the heading `## Prüfung des installierten Ergebnisses`. Shell diagnostics must remain in ordinary fenced `bash` blocks. Browser, UI and functional actions may be a concise numbered checklist outside the shell block. Do not merge functional checks into the installation block when that would hide required user actions.

Every acceptance plan must cover the layers that are relevant to that exact PR:

1. **Installed identity and startup:** verify the checked-out exact head, installed binary or asset identity where the repository provides a reliable method, service state and absence of new startup errors.
2. **Positive feature path:** exercise the behavior introduced or changed by the PR using a real representative resource.
3. **Readback and persistence:** reload the UI or API, restart the affected service, and confirm the result survives without repeating the mutation or requiring an external provider read.
4. **Search and presentation:** verify every changed read model, detail view, search path or frontend rendering affected by the PR.
5. **Replacement and withdrawal semantics:** when the feature supports reassignment, deletion, withdrawal, rollback or fallback, verify the old active result disappears and the documented fallback becomes effective.
6. **Authorization and failure boundaries:** verify the relevant Read-only, wrong-scope, CSRF, provider-failure or invalid-input denial paths without exposing secrets.
7. **Adjacent regression:** repeat the nearest established workflow whose performance or correctness could be affected, such as folder navigation, restart behavior or automatic metadata fallback.
8. **Evidence:** record the exact source head, CI run, installed build identity when available, redacted test resource and observed result.

Only include layers that the exact PR can affect, but never omit persistence/restart or the primary regression boundary merely to shorten the answer. When a test requires credentials, cookies, CSRF values, tokens or private paths, instruct the user through the normal UI or a redacted safe procedure; never request or print those values.

Never describe an acceptance item as passed merely because the daemon started or automated CI is green. Mark it passed only after the user has actually executed the exact-head test and supplied or confirmed the observed result.

## Credential and secret restrictions

Never print, store or commit Authorization headers, plaintext passwords, password hashes, cookies, CSRF tokens, raw session/verifier secrets, TMDB tokens, enrollment tokens, Agent credential secrets, secret-bearing login responses or process environments.
