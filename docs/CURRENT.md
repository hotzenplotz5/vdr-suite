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
- [Phase 66 Closeout](development/phase-66-closeout.md)
- [Post-Phase-66 Home Rebuild Closeout](development/post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Home Performance Hardening](development/post-phase-66-home-performance-hardening.md)
- [Post-Phase-66 Native Recording Editing Closeout](development/post-phase66-recording-editing-closeout.md)
- [ADR-0054 Broadcast Companion Services](adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0058 Media Home](adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR Index](adr/index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Branch authority: main
Accepted Home rebuild merge checkpoint:
ea5967b983aee9ccc3f855b685db01abbfb2326a

Latest completed numbered runtime phase:
Phase 66 - Media Home and Browse Experience

Current active numbered runtime phase:
none - Phase 67 has not started

Next strict numbered runtime phase:
Phase 67 - Broadcast Companion Services: Teletext and HbbTV

Current active runtime slice:
none - post-Phase-66 Home rebuild is completed; Phase 67 has not started
```

The accepted post-Phase-66 Home rebuild branch `work/home-rebuild` ended at `0cce4d1c9e58abe4d529132e92340ae4cbb7a99c` and was merged to `main` as `ea5967b983aee9ccc3f855b685db01abbfb2326a`. The merge tree is identical to the accepted branch tree. This SHA is a durable Home-rebuild checkpoint, not a substitute for reading the live `main` head.

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
- the subsequent Home-rebuild commit series ending at `0cce4d1c...`, merged as `ea5967b9...`.

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

## Performance boundary retained

Home performance work must preserve the accepted reduction of Recording metadata fan-out. In particular:

- do not restore hundreds of browser `/recordings/metadata` reads;
- keep Series completion scoped/progressive rather than global;
- keep folder cards on the embedded folder read model when native metadata is already persisted;
- preserve generation/backend/Home-active fencing and authoritative owner boundaries;
- use preview/cache variants without changing original artwork ownership.

## Completed platform foundations retained

- **Phase 62 - Identity, RBAC and Accountability Foundation** remains authoritative for actor identity, backend-scoped authorization, browser session/CSRF and accountability.
- **Phase 63 - Backend Agent and Secure Multi-Site Runtime** remains authoritative for Agent identity, generation/lease fencing, provider ownership and durable command/result handling.
- **Phase 64 - Timer Intent and Multi-Backend Orchestration** remains authoritative for `TimerIntent -> TimerAssignment -> NativeTimerBinding`, managed fulfillment and controlled reassignment/failover.
- **Phase 65 - Streaming Gateway and Media Sessions** remains authoritative for Recording/Live MediaSession, least-transformation delivery and normalized playback ownership.
- **Phase 66 - Media Home and Browse Experience** remains the completed numbered Home/browse phase; later Home work is non-numbered hardening.

## Next authorization boundary

Phase 67 - Broadcast Companion Services: Teletext and HbbTV is next in the strict numbered sequence, but **Phase 67 has not started**.

Before Phase-67 runtime work:

1. re-read live `main`, this file, the Strict Roadmap and ADR-0054;
2. require an explicit Phase-67 runtime kickoff;
3. start domain-first with Teletext service/page/subpage contracts and HbbTV broadcast-application discovery/session boundaries;
4. do not use Legacy OSD or raw plugin/browser command channels as the primary Teletext/HbbTV contract;
5. preserve Phase-65 MediaSession ownership whenever Suite-owned media is involved;
6. keep Phase 68 Legacy OSD, Phase 69 public API hardening and Phase 70 recommendation work out of the Phase-67 kickoff.

## Historical evidence rule

Exact old candidate SHAs, CI runs and once-open follow-up notes remain valid in their closeouts as historical evidence. They are not current execution authority. Use this file for volatile status and query GitHub for the live head before acting.
