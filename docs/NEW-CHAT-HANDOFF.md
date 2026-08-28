# VDR-Suite New Chat Handoff

## Purpose

This is the canonical operational entry point for every new VDR-Suite chat.

**Do not use this file as a second copy of active PR tips or transient CI state.** Volatile operational truth belongs only in [Current State](CURRENT.md), and live GitHub state must still be re-read before any action.

Root-level `AGENTS.md` is binding. In particular, once the user has authorized a bounded workstream, continue it without voluntary handoff or artificial stopping until the requested end state is reached. Status reports are progress updates, not stopping points. Surface-scoped validation governs iterative documentation/frontend/runtime work; unrelated queued/running CI does not block already-approved progress. A genuinely external dependency is a blocked wait state, not project completion: exhaust all independent approved work and re-read relevant repository/CI state before reporting it. If a relevant CI result is required for the next already-authorized gate, re-read that run before ending the working response rather than returning a stale queued/in-progress snapshot.

## Mandatory reading order

1. [Current State](CURRENT.md) — sole volatile phase/status authority.
2. [Strict Roadmap](planning/roadmap.md) and [Phase Map](planning/phase-map.md) — accepted numbered execution order.
3. [Phase 65 Closeout](development/phase-65-closeout.md) — completed Streaming/MediaSession/playback boundary and durable acceptance evidence.
4. [ADR-0058 Media Home, Responsive Browse and Preview Experience](adr/ADR-0058-media-home-responsive-browse-preview.md) — accepted Phase-66 product/architecture decision.
5. [Phase 66 Media Home and Browse Experience](development/phase-66-media-home-browse-experience.md) — accepted bounded implementation sequence; runtime not started.
6. [Golden User Journeys](planning/golden-user-journeys.md) — desktop/mobile Home and later product acceptance.
7. ADR-0046/0053/0055/0056/0057 when Home work touches accepted Phase-65 playback semantics.
8. [ADR-0054 Broadcast Companion Services](adr/ADR-0054-broadcast-companion-teletext-hbbtv.md) for later Phase-67 Teletext/HbbTV architecture.
9. [Target Platform Architecture](architecture/target-platform-architecture.md), [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md) and [ADR Index](adr/index.md) as required.
10. [Agent Workflow Rules](../AGENTS.md) before repository writes, PR-state changes or installation guidance.

[Current Project Status](development/current-status.md), [Current Architecture State](development/current-architecture-state.md), [Completed Phases](development/completed-phases.md), [Phase 62 Closeout](development/phase-62-closeout.md), [Phase 64 Closeout](development/phase-64-closeout.md) and older Phase-65 development records provide stable historical context.

## Stable project position

- Latest completed numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.
- Current active numbered runtime phase: **none; Phase 66 has not started**.
- Next strict numbered runtime phase: **Phase 66 - Media Home and Browse Experience**.
- Phase 65.A through 65.D are closed for their accepted bounded scopes; ADR-0056 semantic consolidation and ADR-0057 Recording network recovery are completed Phase-65 history.
- ADR-0058 is accepted and owns the Media Home / responsive browse / deferred-preview architecture.
- The accepted Phase-66 contract is planning authority only. Runtime still requires a separate explicit kickoff.
- ADR-0054 remains accepted Broadcast Companion architecture and is now sequenced as Phase 67.
- Broad polished Timer UI remains a cross-cutting milestone gated on required access administration.

Do not copy current branch SHA or active PR tip here; use `CURRENT.md` and GitHub.

## Completed Phase-64 boundary

Phase 64 is complete. Its accepted engine separates:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

The completed scope includes deterministic multi-backend ownership, managed native Timer create/update/toggle/delete fulfillment, durable no-blind-retry handling, authoritative PRESENT/ABSENCE reconciliation and controlled reassignment/failover.

The final real-system gate proved both managed fulfillment and reassignment/failover on one exact accepted candidate before PR #195 merged it to `main`. See [Phase 64 Final Closeout](development/phase-64-closeout.md).

Do not reopen Phase 64 merely to add a broad Timer UI, diagnostics or later media work that consumes the accepted contracts.

## Current implementation boundary

Phase 65 is completed. Phase 66 - Media Home and Browse Experience is next but has not started.

Before any Phase-66 runtime work:

1. re-read live `main`, `CURRENT.md`, ADR-0058 and the Strict Roadmap;
2. verify no Phase-66 runtime branch has already started elsewhere;
3. create a fresh runtime branch from the accepted planning baseline;
4. begin only with Slice 66.1 — Home Shell and Responsive Information Architecture;
5. preserve existing Channel/EPG/Recording/Metadata truth and the completed Phase-65 MediaSession/playback-owner architecture;
6. do not pull deferred preview, history persistence, Teletext/HbbTV, Legacy OSD, public-API hardening or native-app work into Slice 66.1.

Architecture acceptance is not runtime authorization; do not start Phase 66. A separate explicit kickoff is required.

## Phase ordering and broad Timer UI

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 Media Home and Browse Experience [NEXT; NOT STARTED]
  -> Phase 67 Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 Legacy OSD Compatibility Bridge
  -> Phase 69 Public API and Client Compatibility Hardening
  -> Phase 70 Recommendation and Content Knowledge Graph
```

Completed history is not renumbered. ADR-0058 changes only the not-yet-started future sequence; ADR-0054/0047/0048 retain their architecture.

Broad Timer UI remains non-numbered and depends on completed Phase 62 + completed Phase 64 + required account/backend access administration.

## Completed Phase 65 media architecture

Do not invent a second media architecture.

Accepted ADR-0046 owns the server-side Streaming Gateway / MediaSession boundary. Accepted ADR-0053 owns the complementary client playback and media-adaptation strategy. Accepted ADR-0055 owns media-transcode backend selection and hardware-acceleration policy. Accepted ADR-0056 owns the normalized client-facing playback presentation, timeline, continuity and failure semantics.

The intended direction is provider-private and transformation-minimal:

```text
private VDR / Recording source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> media adaptation boundary / internal MediaPresentationProfile
  -> Streaming Gateway / MediaSession
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

Prefer `pass-through -> remux/repackage -> transcode`. Operation-specific stronger adaptation is allowed only when demonstrated correctness requires it and the path remains policy-governed/fail-closed. Exact non-zero HLS video resume is the accepted current example: ordinary start-at-zero remains copy/remux when valid; an exact non-zero video resume uses the implemented synchronized transcode path or fails closed.

Streamdev may be an explicitly owned private provider; it is not the public API, universal platform foundation or implicit fallback.

Accepted product sequence to date:

```text
Recording playback [65.A CLOSED]
  -> Live TV [65.B CLOSED]
  -> Recording delivery performance + media output/transcode settings [65.C CLOSED]
  -> client playback abstraction [65.D CLOSED]
       -> Persistent Browser Playback Shell [CLOSED]
       -> Recording Playback Controls and Seek [CLOSED]
       -> normalized audio/subtitle selection [CLOSED]
       -> browser-local Volume/Mute [CLOSED]
       -> continuous-fMP4 MSE forward buffering [CLOSED]
       -> compatibility timeline + exact HLS resume [CLOSED]
       -> ADR-0056 playback semantic consolidation [CLOSED]
```

Seek/growing-recording truthfulness is a cross-cutting media contract rather than the old 65.C label. Continuous progressive fMP4 must not invent HTTP byte-range semantics, completed-only fast paths must not treat growing sources as immutable, and unsupported growing/timeshift behavior remains explicitly unsupported. User-owned seek preview, transport-local time and canonical absolute Recording position are distinct domains. MediaSession identity, route epoch and playback presentation generation are also distinct concepts.

## Phase 67 Broadcast Companion planning

Teletext and HbbTV remain planned as normal television-domain capabilities for Phase 67, not as Legacy OSD shortcuts.

Accepted ADR-0054 defines the distinction:

```text
TeletextService / TeletextPage
  != LegacyOsdSession

BroadcastApplication / HbbTV Application Session
  != LegacyOsdSession
```

Normal Teletext should use structured service/page/subpage data rather than OSD screenshots or provider cache paths. HbbTV should use bounded application discovery and an authorized isolated application runtime; raw plugin/browser URL, JavaScript, key or attach/detach commands must not become a public Suite API.

Phase-65 MediaSession/provider-ownership semantics remain authoritative when Suite-owned media is involved.

## Project-decision and slice rules

- A chat discussion is not a binding project decision until represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
- `CURRENT.md` is the sole repository copy of volatile operational status.
- Root-level `AGENTS.md` owns the top-level non-stop execution rule: already-authorized work continues until the requested end state. A genuine external dependency is recorded as a blocked wait state only after all independent approved work and authoritative state reads are exhausted; it is not a generic permission to end or declare the workstream complete.
- Status updates, intermediate commits and unrelated queued/running CI are not handoff points.
- Validation during iterative work is surface-scoped; a complete repository CI graph is reserved for the documented Ready/merge/phase-closeout full-stabilization boundary.
- A slice is the smallest **coherent** safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessary long dependency stacks unless a real safety, concurrency, compatibility or acceptance boundary requires the split.
- Technical CI and architecture guards are necessary but user-visible milestones additionally require the applicable Golden User Journeys.
- An accepted ADR defines architecture but does not by itself prove or complete runtime implementation.

## Current security position

- Phase-62 actor identity, scoped authorization, browser-session lifecycle, CSRF and accountability remain authoritative.
- Phase-63 Agent identity, backend generation, durable command semantics and explicit provider ownership/selection remain authoritative for remote execution.
- Phase-64 assignment/binding/operation/fingerprint fences and authoritative readback remain authoritative for Timer orchestration.
- Runtime Agent credentials cannot act as browser users or unrestricted administrators.
- Provider reachability/availability never creates execution authority and active work never silently switches provider.
- Unknown or stale generation, revision, assignment-set, provider-epoch or readback evidence fails closed.
- An uncertain native dispatch is reconciled; it is not blindly repeated.
- Bootstrap/runtime secrets, hashes/verifiers, Authorization headers, cookies, CSRF values, provider credentials, local secret paths and secret-bearing process environments must not be printed, committed or copied into public responses/accountability events.
- TVScraper remains unchanged upstream; do not write to TVScraper-owned databases or caches.
- HbbTV integration must not expose arbitrary broadcaster/plugin URL or JavaScript execution as a general Suite API.

## Compatibility-retirement decision

Legacy Basic compatibility remains transitional and intentionally retained. `enforced` mode is the fail-closed target. Removing Legacy Basic requires a separate deployment-migration contract and is not unfinished Phase 62, Phase 63 or Phase 64.

## Exact action for a new chat

1. Read `CURRENT.md` first.
2. Query live `main` and the relevant PR/branch before making repository-state claims. If a GitHub Actions run exists for the relevant head, report its exact status/link. Do not wait on unrelated jobs before continuing already-approved surface-scoped work. If a relevant run is required for the next already-authorized gate and is queued or in progress, continue independent work and re-read that run before ending the working response; never return the stale non-terminal snapshot as the final state.
3. Treat Phase 64 as completed and Phase 65 as active with 65.A through 65.C closed and 65.D active.
4. Treat D.1/D.2, normalized Recording audio/subtitle selection, browser-local Volume/Mute, continuous-fMP4 MSE forward-buffer control and the compatibility timeline/exact-HLS-resume follow-up as accepted bounded work unless live repository state supersedes that evidence.
5. Derive the next Phase-65.D block from ADR-0056 and `phase-65d-playback-semantics-consolidation.md`: normalized `MediaPlaybackContract`, canonical owner lifecycle publication, continuity/discontinuity or classified failure semantics. Do not revive a stale closed-gap label.
6. Preserve truthful Range/seek/growing capability; completed-Recording seek/resume is accepted for the supported profiles, while growing-Recording seek and Live-TV timeshift remain explicit non-support until separately implemented.
7. Treat Phase-66 Teletext/HbbTV architecture as accepted via ADR-0054, but do not start its runtime before Phase 65 closes and Phase 66 is explicitly authorized.
8. Keep the broad Timer UI as a cross-cutting product milestone; do not reopen Phase 64 or block Streaming solely for that UI.
9. Keep review/merge/retarget/close state changes behind explicit user approval; do not ask again when that exact authorization is already present.
10. Require real yaVDR acceptance when an installed/runtime, native, media or broadcast-behaviour boundary changes; do not repeat accepted runtime evidence for documentation/workflow-only follow-ups.
11. Continue the authorized workstream to its requested end state. Do not turn analysis, a fixable CI failure, an intermediate commit, a queued/running relevant CI run or an ordinary response boundary into a voluntary stop. Never end with an executable next step still available through the current tools; execute it instead.

## Command presentation contract

Every shell command intended for the user to copy or execute must be presented inside a normal fenced Markdown code block, preferably tagged `bash`.

- Never place executable commands in prose, inline-code fragments, writing blocks, generated UI controls or custom code-block formats with IDs or metadata.
- Keep explanations outside the code block.
- Put complete, directly executable command sequences inside the code block.
- Use separate code blocks for logically separate steps when that improves safe execution.
- The established yaVDR checkout is exactly `/home/yavdr/vdr-suite`; do not derive or substitute checkout paths from a person's name or from an unverified chat assumption.
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
