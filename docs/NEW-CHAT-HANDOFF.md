# VDR-Suite New Chat Handoff

## Purpose

This is the canonical operational entry point for every new VDR-Suite chat.

Repository, pull-request, CI and runtime facts must always be rechecked against the current `main` branch and the exact active PR head. Recorded hashes are checkpoints for their exact candidates, not permission to reuse stale assumptions after the branch or stack moves.

## Canonical reading

Read these first:

- [Current State](CURRENT.md)
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
- [Agent Workflow Rules](../AGENTS.md)

Useful Phase-64 foundation documents:

- [Phase 64 TimerIntent Contract](development/phase-64-timer-intent-contract.md)
- [Phase 64 TimerIntent Repository](development/phase-64-timer-intent-repository.md)
- [Phase 64 TimerAssignment Contract](development/phase-64-timer-assignment-contract.md)

Historical Phase-62/63 closeouts remain authoritative evidence for their accepted candidates but are no longer the current work entry point.

## Stable project position

```text
Repository: hotzenplotz5/vdr-suite
Current branch authority: main
Current merged main checkpoint:
96fab8ad88eae9ea0d46adf4db50ccf8d750a19b

Latest completed numbered runtime phase:
Phase 63 - Backend Agent and Secure Multi-Site Runtime

Previous completed numbered runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Current active numbered runtime phase:
Phase 64 - Timer Intent and Multi-Backend Orchestration

Merged Phase-64 foundation:
Slice 1 - TimerIntent Domain Contract, PR #150
Slice 2 - TimerIntent Persistence and Repository Semantics, PR #152

Current active stacked Draft tip:
PR #168 - Add failure-aware RESTfulAPI native Timer inventory reader
branch: agent/phase64-restfulapi-native-timer-inventory-reader
head checkpoint: a49a3b99167e8f1bbbbf220657d774aaf4038501
CI checkpoint: VDR-Suite CI #7412 / run 31462922497 - PASS
runtime change: none; Slice-16 is not linked into an installed runtime target
```

Never copy the head above into a later status claim without checking GitHub again.

## Completed Phase 63 platform boundary

Phase 63 is complete and must not be reopened for Timer work.

It established the platform prerequisites used by Phase 64:

- Backend Agent enrollment, technical identity and credential lifecycle;
- protected outbound transport, lease, Agent-instance and backend-generation fencing;
- generation-/sequence-fenced observation ingestion;
- explicit Channel observation with no hidden source fallback;
- durable command assignment, receipt, result and reconciliation state;
- fenced side-effect-free native execution through SuiteBridge;
- plugin-instance epoch and authoritative readback semantics;
- explicit provider facts, ownership and immutable provider selection;
- no silent provider fallback;
- a generic protected-write safety contract for durable idempotency, authorization, revision/generation/provider fencing and uncertain-outcome reconciliation.

Phase 63 did not own TimerIntent, TimerAssignment or NativeTimerBinding. Those are Phase-64 domains.

## Phase 64 current stack

The active Timer stack is intentionally layered:

```text
TimerIntent
  -> domain contract
  -> durable repository/revisions

TimerAssignment
  -> domain contract
  -> durable repository/revisions/epochs
  -> deterministic planner
  -> primary scheduling handoff
  -> assignment-set concurrency fence
  -> replica scheduling handoff

NativeTimerBinding
  -> domain contract
  -> durable optimistic-concurrency repository
  -> VDR NativeTimerObservation mapper
  -> safe present-readback application
  -> operation-bound expected PRESENT evidence
  -> operation-aware PRESENT verification
  -> complete native Timer inventory / absence evidence
  -> failure-aware RESTfulAPI complete-inventory reader
```

Exact Draft PR progression at this checkpoint:

```text
#153 -> #154 -> #155 -> #158 -> #159 -> #160
     -> #161 -> #162 -> #163 -> #164 -> #165 -> #166 -> #167 -> #168
```

Draft PR #157 is a separate repository-wide SQLite architecture-baseline repair. Draft PR #156 is a separate proposed client playback/media-adaptation ADR; it does not implement Phase-65 runtime and is not part of the Timer stack.

## Timer safety position at Slice 16

The stack now proves the foundations that must exist before production native Timer mutation:

- Suite Timer intent, assignment and native binding identity are separate;
- intent, assignment-set and binding concurrency are explicit;
- primary/replica planning consumes authoritative generation/capability/health/channel evidence and fails closed on stale or ambiguous facts;
- native Timer observations are copied backend-neutral evidence, not raw VDR objects;
- semantically unchanged present observations may refresh evidence without erasing unresolved drift;
- changed present state is not silently overwritten;
- expected PRESENT mutation results are bound to exact operation ID, binding revision, backend/native identity, backend generation, a post-operation time fence and normalized fingerprint;
- successful PRESENT verification records only authoritative post-operation readback;
- Timer absence can be established only from explicit complete-inventory evidence;
- HTTP failure, malformed JSON, incomplete parsing or missing identities cannot become absence proof;
- the Slice-16 RESTfulAPI reader remains outside daemon runtime wiring.

No current Phase-64 Draft enables `mutations=enabled` or a production Agent/SuiteBridge/RESTfulAPI/SVDRP Timer create/update/delete path.

## Exact next bounded work

The next Phase-64 slice after PR #168 is a backend-neutral NativeTimerBinding absence-application service.

It must:

1. load the exact current durable binding;
2. accept only valid complete inventory evidence for the exact backend and generation;
3. require evidence new enough for the queried native Timer identity;
4. persist authoritative missing observation evidence while preserving the last known present representation and fingerprint;
5. use the exact current binding revision and surface repository conflict without hidden retry;
6. avoid guessing `external_delete`, ownership changes, assignment transitions or failover eligibility from absence alone.

Later separate slices must address operation-aware delete verification, changed-state/external-drift reconciliation, binding/assignment lifecycle transitions, controlled replacement/failover, protected native Timer create/update/delete, daemon orchestration and public Timer surfaces.

## Phase ordering and the broad Timer UI

The strict numbered runtime order is:

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration
  -> Phase 65 - Streaming Gateway and Media Sessions
  -> Phase 66 - Legacy OSD Compatibility Bridge
  -> Phase 67 - Public API and Client Compatibility Hardening
```

This means Phase 65 Streaming follows completion of the reliable Phase-64 Timer **engine**, not completion of a broad polished Timer UI.

Broad Timer mutation controls remain separately gated on account/backend access management built on the completed Phase-62 identity, browser-session, authorization and backend-scoped permission model. That frontend/security gate does not block the backend-neutral Timer engine and does not block Phase 65 once Phase 64 is complete.

Therefore it is valid and intended for Streaming Gateway runtime work to occur before the broad Timer UI is finished. This is project sequencing, not a technical dependency of Streaming on the Timer UI.

## Documentation synchronization rule

`NEW-CHAT-HANDOFF.md`, `CURRENT.md` and `development/current-status.md` are the direct operational status entry points.

Some current-position markers in `planning/roadmap.md` and `planning/phase-map.md` still lag the active Phase-64 stacked Drafts. Their architectural dependency rules and strict phase order remain useful, but a stale recorded active-slice marker must not override the exact GitHub stack or these synchronized status files. Updating those broader planning documents is a separate guarded documentation task.

## Current security and authority position

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

## Current work boundary

- Phase 62 is complete.
- Phase 63 is complete.
- Phase 64 is active.
- PR #168 is the current top Timer Draft at this checkpoint and must be re-read before continuation.
- Keep stacked Timer PRs Draft until explicit approval changes their review state.
- Do not jump from evidence/reconciliation work directly to production native Timer mutation.
- Do not infer external deletion, replacement or failover from transport failure or absence alone.
- Do not create a second BackendRegistry, authorization service, accountability store, job system or scheduler authority.
- Do not add Phase-65 Streaming runtime to a Phase-64 Timer slice.
- Broad Timer UI work must not bypass the account/backend access-management gate.
- Do not require manual SQLite inspection for runtime acceptance.
- Do not add unrelated refactors or cosmetic rewrites to bounded slices.

## Exact next action

1. Read PR #168 and its exact current head, diff, mergeability and CI from GitHub.
2. Treat Slice-16 as the current checkpoint only if that exact head remains authoritative and green.
3. Open the next bounded stacked Draft for NativeTimerBinding absence application on the exact current #168 head.
4. Add focused domain/repository regression and architecture guard coverage only for that slice.
5. Keep native mutation, delete verification, external-drift classification and failover out of the absence-application slice.
6. Evaluate the final exact-head CI according to `AGENTS.md`; require real yaVDR runtime acceptance only when an installed/runtime boundary actually changes.
7. Keep all review/merge state transitions behind explicit user approval.

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

## Binding branch- and PR-specific installed-result acceptance manifest

A successful build, file installation and `active (running)` service state prove only that deployment completed. They do not prove that the requested PR behavior works. Every installation answer must therefore be followed by a branch- or PR-specific acceptance section derived from the exact current head.

Before writing the acceptance steps, inspect:

- the exact PR diff and changed components;
- the current feature, ADR and runtime-acceptance documents;
- changed REST routes, persistence/schema behavior, frontend paths, services and plugin contracts;
- the closest existing regression behavior that the PR could unintentionally break.

The user-facing answer must use the heading `## Prüfung des installierten Ergebnisses`. Shell diagnostics must remain in ordinary fenced `bash` blocks. Browser, UI and functional actions may be a concise numbered checklist outside the shell block.

Every acceptance plan must cover the relevant layers for that exact PR:

1. installed identity and startup;
2. positive feature path;
3. readback and persistence/restart;
4. changed search/presentation surfaces;
5. replacement/withdrawal semantics where applicable;
6. authorization and failure boundaries;
7. nearest adjacent regression;
8. exact source/CI/build/redacted evidence.

Never describe an acceptance item as passed merely because the daemon started or automated CI is green. Mark it passed only after the exact-head functional test has actually been executed and observed.

## Credential and secret restrictions

Never print, store or commit Authorization headers, plaintext passwords, password hashes, cookies, CSRF tokens, raw session/verifier secrets, provider tokens, enrollment tokens, Agent credential secrets, secret-bearing login responses or process environments.
