# VDR-Suite New Chat Handoff

## Purpose

This is the canonical operational entry point for every new VDR-Suite chat.

**Do not use this file as a second copy of active PR tips or transient CI state.** Volatile operational truth belongs only in [Current State](CURRENT.md), and live GitHub state must still be re-read before any action.

Root-level `AGENTS.md` is binding. In particular, once the user has authorized a bounded workstream, continue it without voluntary handoff or artificial stopping until the requested end state is reached. Status reports are progress updates, not stopping points. Surface-scoped validation governs iterative documentation/frontend/runtime work; unrelated queued/running CI does not block already-approved progress. A genuinely external dependency is a blocked wait state, not project completion: exhaust all independent approved work and re-read relevant repository/CI state before reporting it. If a relevant CI result is required for the next already-authorized gate, re-read that run before ending the working response rather than returning a stale queued/in-progress snapshot.

## Mandatory reading order

1. [Current State](CURRENT.md) — exact current repository checkpoint.
2. [Strict Roadmap](planning/roadmap.md) — binding phase order and completion gates.
3. [Phase 64 Final Closeout](development/phase-64-closeout.md) for the accepted Timer-engine boundary.
4. [ADR-0046 Streaming Gateway](adr/ADR-0046-streaming-gateway-media-session-boundary.md), [ADR-0053 Client Playback / Media Adaptation](adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md), [ADR-0055 Media Transcode Backend Selection](adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md) and [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md) for current Phase-65 media work.
5. [Phase 65 Recording Playback Closeout](development/phase-65-recording-playback-closeout-readiness.md), [Phase 65 Live-TV Playback Closeout](development/phase-65-live-tv-closeout.md), [Phase 65.C Recording Startup / Progressive Direct](development/phase-65-recording-startup-progressive-direct.md), [Phase 65 Media Transcode Performance / Output Policy](development/phase-65-media-transcode-performance-policy.md), [Phase 65.D.1 Persistent Browser Playback Shell Closeout](development/phase-65d1-persistent-browser-playback-shell-closeout.md), [Phase 65.D.2 Recording Playback Controls and Seek Closeout](development/phase-65d2-recording-playback-controls-seek-closeout.md), [Frontend Playback Integration Contract](development/frontend-playback-integration-contract.md) and [Phase 65.D Playback Semantics Consolidation](development/phase-65d-playback-semantics-consolidation.md) for accepted implementation history and the active semantic contract.
6. [Golden User Journeys](planning/golden-user-journeys.md) for vertical product acceptance.
7. [Target Platform Architecture](architecture/target-platform-architecture.md), [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md) and [Architecture Decision Records](adr/index.md) as required by the task.
8. If work concerns Teletext/HbbTV or future phase ordering, read accepted [ADR-0054 Broadcast Companion Services](adr/ADR-0054-broadcast-companion-teletext-hbbtv.md); acceptance defines the architecture but does not authorize Phase-66 runtime.
9. [Agent Workflow Rules](../AGENTS.md) before repository writes, PR-state changes or installation guidance.

[Current Project Status](development/current-status.md), [Current Architecture State](development/current-architecture-state.md), [Completed Phases](development/completed-phases.md), [Phase 62 Final Closeout](development/phase-62-closeout.md) and the Phase-63 development records provide stable historical/narrative context.

## Stable project position

- Latest completed numbered runtime phase: **Phase 64 - Timer Intent and Multi-Backend Orchestration**.
- Current active numbered runtime phase: **Phase 65 - Streaming Gateway and Media Sessions**.
- Completed Phase-65 verticals: **65.A Existing-Recording playback**, **65.B Live-TV playback**, **65.C Recording delivery performance and media output/transcode settings**.
- Current Phase-65 product vertical: **65.D Client playback abstraction**.
- Accepted Phase-65.D bounded work includes **65.D.1 Persistent Browser Playback Shell**, **65.D.2 Recording Playback Controls and Seek**, normalized Recording audio/subtitle selection, browser-local Volume/Mute, continuous-fMP4 browser MSE forward-buffer control, compatibility timeline-drag ownership and exact non-zero HLS Recording resume synchronization.
- The active Phase-65.D architecture gate is **ADR-0056 playback semantic consolidation**.
- Phase 58 - Frontend and Live Parity remains a historical umbrella track.
- A broad polished Timer UI remains outside the Phase-64 completion gate and is a cross-cutting product milestone gated on the required account/backend access administration.

The earlier planning label `65.C - Recording seek and growing-recording semantics` is superseded by accepted implementation history. Phase 65.C actually combined the completed-Recording startup/progressive-delivery work from PR #206 with the subsequently continued backend-scoped media-transcode/output policy and Web settings from PR #208. Truthful seek/range/growing capability remains binding.

Phase 65.D.2 and later accepted follow-ups provide arbitrary completed-Recording time-seek and stop/resume for the supported progressive-fMP4 and HLS restart-seek profiles. PR #219 closed the demonstrated continuous-fMP4 browser MSE forward-buffer/backpressure defect. PR #220/#221 closed the compatibility timeline drag ownership defect and established the sync-safe/fail-closed exact non-zero HLS video resume rule. User-visible growing-Recording seek, Live-TV timeshift and broader VDR-index mapping beyond those accepted paths remain deferred.

The old unstarted 65.D compatibility-escalation block was consumed inside completed 65.C. The replacement 65.D Client playback abstraction vertical remains active, now under ADR-0056 rather than an obsolete list of already-closed individual client gaps.

Do not paste a current branch SHA or active PR tip into this section. Read exact operational state from `CURRENT.md` and GitHub instead.

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

Phase 65 is active. Phase 65.A through 65.C are closed for their accepted bounded scopes. Phase 65.D remains active with the accepted work listed above closed for its bounded scopes.

The next coherent Phase-65.D work is the ADR-0056 semantic consolidation. Before a new runtime block:

1. re-read live `main` and `CURRENT.md`;
2. review ADR-0046, ADR-0053, ADR-0055 and ADR-0056 against the current client/media code;
3. read [Phase 65.D Playback Semantics Consolidation](development/phase-65d-playback-semantics-consolidation.md) and the [Frontend Playback Integration Contract](development/frontend-playback-integration-contract.md);
4. inspect the exact remaining semantic gap in the accepted playback owner/API rather than reviving a closed historical gap;
5. choose the smallest coherent ADR-0056 slice;
6. preserve truthful capability reporting, provider privacy, least-transformation selection, output policy and deterministic cleanup;
7. do not start Phase 66.

The mandatory remaining ADR-0056 semantic areas are:

```text
normalized MediaPlaybackContract
canonical playback-owner lifecycle snapshot/subscription
continuity/discontinuity and playback-presentation generation semantics
classified playback failure semantics
```

Read-only media diagnostics are sequenced after semantic correctness. Shared fMP4/MSE helper deduplication is technical debt, not a Phase-65.D completion gate.

Do not treat an old draft PR, historical “next slice” note, the now-closed continuous-fMP4 forward-buffer gap, the accepted audio/subtitle work, obsolete 65.C seek label or prior chat plan as current authorization.

## Phase ordering and broad Timer UI

The binding/planned order is:

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [ACTIVE]
  -> Phase 66 Broadcast Companion Services: Teletext and HbbTV
  -> Phase 67 Legacy OSD Compatibility Bridge
  -> Phase 68 Public API and Client Compatibility Hardening
  -> Phase 69 Recommendation and Content Knowledge Graph
```

Completed history is not renumbered. The post-Phase-65 sequence concerns only not-yet-started future phases. Phase 66 is backed by accepted ADR-0054 and remains runtime-blocked until Phase 66 is explicitly started after Phase 65 closes.

The broad polished Timer UI is not the Phase-64 completion gate and is not inserted as a numbered phase. Its prerequisite chain is:

```text
Phase 62 identity/RBAC [DONE]
+ Phase 64 Timer engine [DONE]
+ required account/backend access administration [OPEN]
```

Therefore Phase 65 Streaming may intentionally be implemented before the broad Timer UI is finished.

## Streaming planning already exists

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
  -> client playback abstraction [65.D ACTIVE]
       -> Persistent Browser Playback Shell [CLOSED]
       -> Recording Playback Controls and Seek [CLOSED]
       -> normalized audio/subtitle selection [CLOSED]
       -> browser-local Volume/Mute [CLOSED]
       -> continuous-fMP4 MSE forward buffering [CLOSED]
       -> compatibility timeline + exact HLS resume [CLOSED]
       -> ADR-0056 playback semantic consolidation [ACTIVE]
```

Seek/growing-recording truthfulness is a cross-cutting media contract rather than the old 65.C label. Continuous progressive fMP4 must not invent HTTP byte-range semantics, completed-only fast paths must not treat growing sources as immutable, and unsupported growing/timeshift behavior remains explicitly unsupported. User-owned seek preview, transport-local time and canonical absolute Recording position are distinct domains. MediaSession identity, route epoch and playback presentation generation are also distinct concepts.

## Broadcast Companion planning

Teletext and HbbTV are planned as normal television-domain capabilities, not as Legacy OSD shortcuts.

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
