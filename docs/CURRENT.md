# VDR-Suite Current State

## Operational status authority

**This file is the sole repository authority for volatile operational phase status.**

Stable architecture, historical evidence and workflow rules live in their respective ADRs, roadmap and closeout documents. Exact live GitHub state must still be re-read before implementation, review-state or merge actions.

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Golden User Journeys](planning/golden-user-journeys.md)
- [Current Project Status](development/current-status.md)
- [Phase 67 Teletext Closeout](development/phase-67-teletext-closeout.md)
- [Phase 66 Closeout](development/phase-66-closeout.md)
- [Post-Phase-66 Home Rebuild Closeout](development/post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Recording Detail Closeout](development/post-phase66-recording-detail-closeout.md)
- [Post-Phase-66 Home Performance Hardening](development/post-phase-66-home-performance-hardening.md)
- [Post-Phase-66 Native Recording Editing Closeout](development/post-phase66-recording-editing-closeout.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](development/phase-65d1-persistent-browser-playback-shell-closeout.md)
- [ADR-0054 Broadcast Companion Services](adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0058 Media Home](adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR Index](adr/index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Accepted Home rebuild merge checkpoint:
ea5967b983aee9ccc3f855b685db01abbfb2326a

Latest accepted post-phase runtime merge checkpoint:
7d850123aaa1d04362d1a94ce584a4cc3c3a5b3b

Latest completed numbered runtime phase:
Phase 66 - Media Home and Browse Experience

Current active numbered runtime phase:
Phase 67 - Broadcast Companion Services: Teletext and HbbTV

Completed Phase-67 vertical:
Teletext read path / browser-TV view / page navigation

Current active runtime slice:
Phase 67 HbbTV discovery is next; HbbTV session/runtime is not yet implemented

Latest Phase-67 Teletext merge checkpoint:
PR #293 -> d92e7907637122368908ce4c7564a0332db9c487
```

The accepted post-Phase-66 Home rebuild branch `work/home-rebuild` ended at `0cce4d1c9e58abe4d529132e92340ae4cbb7a99c` and was merged to `main` as `ea5967b983aee9ccc3f855b685db01abbfb2326a`. The merge tree is identical to the accepted branch tree. This SHA is a durable Home-rebuild checkpoint, not a substitute for reading the live `main` head.

The later accepted post-phase runtime chain includes the Recordings 2 cinematic detail/presentation work, the related-Genre portrait correction and the Live-TV `ended` lifecycle stabilization. The current live `main` head must still be queried before repository-state actions; the runtime checkpoint above is historical evidence, not a permanent branch tip.

## Phase 66 and post-phase completion state

**Phase 66 is completed.** Slices 66.1 through 66.8 remain the numbered Phase-66 completion boundary. The later work below is deliberately non-numbered hardening/correctness work and does not reopen Phase 66 or start Phase 67.

Merged post-Phase-66 work includes, among other bounded follow-ups:

- PR #265 Home performance hardening;
- PR #266 Recording Discovery performance hardening;
- PR #267 Series metadata/artwork completion;
- PR #268/#269 native Recording marks/cutting and documentation closeout;
- PR #270 retained Series UI/revalidation hardening;
- PR #272 browser diagnostics route recovery;
- PR #273 removal of legacy EPG overview contamination from Media Home;
- PR #274 bounded persistent Home artwork previews;
- PR #277 bounded random-Genre metadata work;
- PR #278 EPG cache worker timeout recovery;
- PR #279 in-place Recent Movies expansion;
- PR #280 representative Series metadata scheduling;
- PR #281 Home artwork preview diagnostics;
- PR #282 artwork-preview cache-hit fast path;
- PR #283 deferred Series metadata hierarchy completion;
- the subsequent Home-rebuild commit series ending at `0cce4d1c...`, merged as `ea5967b9...`;
- PR #285 restoration of the EPG artwork/metadata cache routes dropped during the Home H2/H2.1 rewrite;
- PR #287 cinematic Recordings 2 detail/hero presentation and canonical-owner playback prewarm;
- PR #288 canonical portrait poster selection for related-Genre Recording cards;
- PR #289 Live-TV `ended` lifecycle stabilization so a transient media-element EOF does not destroy the canonical Live MediaSession;
- PR #291 documentation of the accepted Live-TV lifecycle stabilization.

Open or draft branches/PRs are not accepted current-state truth until they are merged and separately documented where required.

## Current accepted Home boundary

The first-party Home state now includes the completed original Phase-66 experience plus the merged rebuild/hardening work:

- canonical Home shell and navigation ownership;
- `Was läuft jetzt` / `Was läuft danach` with compact EPG critical-path loading and bounded artwork enrichment;
- newly recorded content and Recording discovery;
- Movies / recent movies and Genre browsing;
- Series projection with Series -> seasons -> episodes;
- progressive/scoped metadata completion without restoring the old global per-Recording metadata fan-out;
- TVScraper/canonical portrait artwork priority for Home cards;
- manual Series hierarchy overrides, pilot/miniseries and TV-film/special handling;
- persistent Series artwork overrides and reset-to-automatic hierarchy behavior;
- retained Series/folder/detail identity and navigation ownership;
- canonical native Recording artwork embedded into folder projections so folder cards do not need per-Recording metadata HTTP reads;
- real-system acceptance of the folder-poster regression using `The Exorcist`, where the canonical red TVScraper portrait again wins over a weak Recording still.

The earlier Phase-66-closeout note that visible Series metadata/artwork could remain unresolved is historical evidence of the state at that closeout. It is no longer an open current-state item: subsequent merged work established the root causes and completed the bounded Series metadata/artwork/hierarchy corrections.

## Current accepted Recording detail boundary

The merged Recordings 2 detail surface now includes the accepted post-Phase-66 presentation work documented in [Post-Phase-66 Recording Detail Closeout](development/post-phase66-recording-detail-closeout.md):

- cinematic/full-page Hero detail composition;
- compact Recording facts and retained canonical title/subtitle/description metadata;
- prominent playback entry into the existing canonical Recording playback owner;
- cast/person presentation and related-Genre Recording rail;
- metadata, marks and playback submodes retained inside the same Recordings 2 detail owner;
- canonical-owner playback prewarm without autoplay, duplicate MediaSessions or false Continue Watching/history publication;
- related-Genre cards using canonical `kind=poster` selection so locked manual posters remain authoritative and native portrait metadata wins before weaker preferred artwork fallback;
- real yaVDR/browser acceptance for the Hero/mobile presentation, immediate playback startup and corrected portrait covers.

This presentation work does not create a new Recording identity, metadata authority or playback lifecycle. Native Recording marks/cutting remains separately governed by the accepted native Recording editing architecture and closeout.

## Live-TV lifecycle stabilization

The accepted Phase-65 persistent playback architecture remains authoritative. A later real-system regression was traced to the browser shell treating `HTMLMediaElement.ended` as a terminal Live-TV stop even though continuous Live/MSE playback is intentionally open-ended.

PR #289 removed that false stop boundary. The accepted behavior is now:

- a transient Live `ended` signal does not destroy the active canonical MediaSession;
- explicit Live stop, backend change, browser-session loss and replacement handoff remain real lifecycle boundaries;
- fatal player `error` remains terminal;
- the real yaVDR/browser regression test passed and Live-TV continued beyond the prior failure window;
- the durable evidence is recorded in [Phase 65.D.1 Persistent Browser Playback Shell Closeout](development/phase-65d1-persistent-browser-playback-shell-closeout.md).

This is post-closeout stabilization of completed Phase-65 playback behavior and does not reopen Phase 65 or Phase 66.

## Performance boundary retained

Home performance work must preserve the accepted reduction of Recording metadata fan-out. In particular:

- do not restore hundreds of browser `/recordings/metadata` reads;
- keep Series completion scoped/progressive rather than global;
- keep folder cards on the embedded folder read model when native metadata is already persisted;
- preserve generation/backend/Home-active fencing and authoritative owner boundaries;
- use preview/cache variants without changing original artwork ownership.

Recording-detail enrichment and playback prewarm must remain scoped to the selected/opened Recording and must not become a global Home metadata or MediaSession fan-out.

## Completed platform foundations retained

- **Phase 62 - Identity, RBAC and Accountability Foundation** remains authoritative for actor identity, backend-scoped authorization, browser session/CSRF and accountability.
- **Phase 63 - Backend Agent and Secure Multi-Site Runtime** remains authoritative for Agent identity, generation/lease fencing, provider ownership and durable command/result handling.
- **Phase 64 - Timer Intent and Multi-Backend Orchestration** remains authoritative for `TimerIntent -> TimerAssignment -> NativeTimerBinding`, managed fulfillment and controlled reassignment/failover.
- **Phase 65 - Streaming Gateway and Media Sessions** remains authoritative for Recording/Live MediaSession, least-transformation delivery and normalized playback ownership.
- **Phase 66 - Media Home and Browse Experience** remains the completed numbered Home/browse phase; later Home/Recording/Live work is non-numbered hardening.

## Current Phase-67 boundary

Phase 67 is **active**. The Teletext vertical is completed and merged through PR #293.

Accepted Teletext capability now includes:

- normalized Teletext service/page/subpage contracts;
- private SuiteBridge/provider transport through the existing Phase-63 Agent path;
- backend-generation and embedded-lifecycle fencing;
- authorized `broadcast.teletext.view` HTTP reads;
- real 25 x 40 / 1000-cell page rendering;
- Live-TV companion navigation and direct Media Home entry;
- accepted Golden Journey 8 behavior on the supported browser/yaVDR deployment.

The next coherent Phase-67 vertical is HbbTV discovery. HbbTV session/runtime work remains open after discovery.

Retained boundaries:

1. do not use Legacy OSD or raw plugin/browser command channels as the primary Teletext/HbbTV contract;
2. preserve Phase-65 MediaSession ownership whenever Suite-owned media is involved;
3. keep Phase 68 Legacy OSD, Phase 69 public API hardening and Phase 70 recommendation work outside the current Phase-67 HbbTV vertical;
4. treat [Phase 67 Teletext Closeout](development/phase-67-teletext-closeout.md) as the durable Teletext acceptance record.

## Historical evidence rule

Exact old candidate SHAs, CI runs and once-open follow-up notes remain valid in their closeouts as historical evidence. They are not current execution authority. Use this file for volatile status and query GitHub for the live head before acting.
