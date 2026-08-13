# VDR-Suite New Chat Handoff

## Purpose

This is the canonical operational entry point for every new VDR-Suite chat.

**Do not use this file as a second copy of exact active heads or CI state.** Volatile operational truth belongs only in [Current State](CURRENT.md), and live GitHub state must still be re-read before any action.

## Mandatory reading order

1. [Current State](CURRENT.md) — exact current repository checkpoint and implementation hold.
2. [Strict Roadmap](planning/roadmap.md) — binding phase order and completion gates.
3. [ADR-0044 Timer Model](adr/ADR-0044-timer-intent-assignment-native-timer-model.md) for Phase-64 Timer work.
4. [ADR-0046 Streaming Gateway](adr/ADR-0046-streaming-gateway-media-session-boundary.md) for Phase-65 media work.
5. [Golden User Journeys](planning/golden-user-journeys.md) for vertical product acceptance.
6. [Target Platform Architecture](architecture/target-platform-architecture.md), [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md) and [Architecture Decision Records](adr/index.md) as required by the task.
7. [Agent Workflow Rules](../AGENTS.md) before repository writes, PR-state changes or installation guidance.

[Current Project Status](development/current-status.md), [Completed Phases](development/completed-phases.md), [Phase 62 Final Closeout](development/phase-62-closeout.md) and [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md) provide stable historical/narrative context.

## Stable project position

- Latest completed numbered runtime phase: **Phase 63 - Backend Agent and Secure Multi-Site Runtime**.
- Current active numbered runtime phase: **Phase 64 - Timer Intent and Multi-Backend Orchestration**.
- Next strict numbered runtime phase after Phase 64: **Phase 65 - Streaming Gateway and Media Sessions**.
- Phase 58 - Frontend and Live Parity remains a historical umbrella track.
- Post-Phase 61 Performance Hardening (B1-B4), VDR Remote and Live Overlay hardening (#110), Backend-scoped Global Search (#111) and Configurable photorealistic VDR Remote (#115) are completed non-numbered platform work.

Do not paste a branch SHA or current PR tip into this section. Read it from `CURRENT.md` and GitHub instead.

## Current implementation boundary

The agreed implementation checkpoint is after **PR #190**, the disabled SuiteBridge Timer-delete transport checkpoint.

This means:

- Phase 64 is still active and **not declared complete**;
- production native Timer deletion is not enabled by the checkpoint;
- a concrete private SuiteBridge transport existing does not authorize its mutation callback;
- **no Phase-64 successor implementation and no `#191` are currently authorized**;
- the current authorized task is architecture/documentation/roadmap synchronization only.

A later implementation restart requires an explicit decision after the planning review. Do not treat a “next slice” paragraph inside an older development note as current permission to start it.

## Phase ordering and broad Timer UI

The binding intended order is:

```text
Phase 64 reliable Timer orchestration engine
  -> Phase 65 Streaming Gateway and Media Sessions
  -> Phase 66 Legacy OSD Compatibility Bridge
  -> Phase 67 Public API and Client Compatibility Hardening
```

The broad polished Timer UI is not the Phase-64 completion gate. It remains separately gated on account/backend access management built on the Phase-62 identity/authorization model.

Therefore Phase 65 Streaming may intentionally be implemented before the broad Timer UI is finished, but only after the reliable Phase-64 Timer engine satisfies its own completion gates.

## Streaming planning already exists

Do not invent a second media architecture.

Accepted ADR-0046 owns the server-side Streaming Gateway / MediaSession boundary. Draft PR #156 contains the proposed complementary client-playback/media-adaptation strategy and must be reviewed against the current canonical planning before acceptance.

Its intended direction is provider-private and transformation-minimal:

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> media adaptation boundary
  -> Streaming Gateway / selected MediaSession profile
  -> client playback adapter
  -> platform playback engine
```

Prefer `pass-through -> remux/repackage -> transcode`. Streamdev may be an explicitly owned private provider; it is not the public API, universal platform foundation or implicit fallback.

## Project-decision and slice rules

- A chat discussion is not a binding project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
- `CURRENT.md` is the sole repository copy of volatile operational status.
- A slice is the smallest **coherent** safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessary long dependency stacks unless a real safety, concurrency, compatibility or acceptance boundary requires the split.
- Technical CI and architecture guards are necessary but user-visible milestones additionally require the applicable Golden User Journeys.

## Current security position

- Phase-62 actor identity, scoped authorization, browser-session lifecycle, CSRF and accountability remain authoritative.
- Phase-63 Agent identity, backend generation, durable command semantics and explicit provider ownership/selection remain authoritative for remote execution.
- Runtime Agent credentials cannot act as browser users or unrestricted administrators.
- Provider reachability/availability never creates execution authority and active work never silently switches provider.
- Unknown or stale generation, revision, assignment-set, provider-epoch or readback evidence fails closed.
- An uncertain native dispatch is reconciled; it is not blindly repeated.
- Bootstrap/runtime secrets, hashes/verifiers, Authorization headers, cookies, CSRF values, provider credentials, local secret paths and secret-bearing process environments must not be printed, committed or copied into public responses/accountability events.
- TVScraper remains unchanged upstream; do not write to TVScraper-owned databases or caches.

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and intentionally retained. `enforced` mode is the fail-closed target. Removing Legacy Basic requires a separate deployment-migration contract and is not unfinished Phase 62, Phase 63 or Phase 64.

## Exact action for a new chat

1. Read `CURRENT.md` first.
2. Query live `main`, the relevant PR/branch and exact-final-head CI before making a status claim.
3. Respect the current implementation hold; do not start a Timer successor merely because the stack has an obvious next technical step.
4. For planning work, reconcile the roadmap and proposed media ADR with the current checkpoint before suggesting implementation.
5. Keep review/merge/retarget/close state changes behind explicit user approval.
6. Require real yaVDR acceptance when an installed/runtime or native-behaviour boundary changes.

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
