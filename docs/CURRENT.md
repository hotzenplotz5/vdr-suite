# VDR-Suite Current State

## Operational status authority

**This file is the sole repository authority for volatile operational phase status.**

Stable architecture, historical evidence and workflow rules live in their respective ADRs, roadmap and closeout documents. Exact live GitHub state must still be re-read before implementation, review-state or merge actions.

## Navigation

- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase 69 Kickoff](development/phase-69-public-api-kickoff.md)
- [Phase 69.B Closeout](development/phase-69b-closeout.md)
- [Phase Map](planning/phase-map.md)
- [Golden User Journeys](planning/golden-user-journeys.md)
- [Current Project Status](development/current-status.md)
- [Phase 68 Closeout](development/phase-68-closeout.md)
- [Phase 68 Kickoff](development/phase-68-legacy-osd-kickoff.md)
- [Phase 68.D View-Session Closeout](development/phase-68d-view-session-authorization-closeout.md)
- [Phase 68.E Viewer-Binding Closeout](development/phase-68e-viewer-bindings-closeout.md)
- [Phase 67 Closeout](development/phase-67-closeout.md)
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
2b0d0990974244eac90e9be711843678c77e301d

Latest completed numbered runtime phase:
Phase 68 - Legacy OSD Compatibility Bridge

Current active numbered runtime phase:
Phase 69 - Public API and Client Compatibility Hardening

Next strict numbered runtime phase:
Phase 69 - Public API and Client Compatibility Hardening

Completed Phase-67 verticals:
Teletext read path / browser-TV view / page navigation
HbbTV discovery / authorized application session / presentation-media runtime

Accepted Phase-68 verticals:
68.A - Read-only semantic OSD observation
68.B - Agent-local buffering and resynchronization
68.C - Authenticated read-only Agent OSD transport
68.D - Authorized bounded Legacy OSD view sessions
68.E - Bounded viewer bindings and multi-viewer delivery
68.F - Exclusive controller lease and osd.control fencing
68.G - Allowlisted native OSD input

Current active runtime slice:
69.C - Revision/precondition/idempotency exposure

Current bounded 69.C step:
Actor-scoped MutationOperation read facade and single DaemonRuntime repository composition

Accepted Phase-69.C checkpoint:
PR #321 -> beb98f6edab39d962bd6415db7be21cf145e05cb
PR #321 hosted CI run 35873370127 / #9091: SUCCESS (6/6)

Phase-69.B closeout checkpoint:
PR #320 -> c4b9fc66d0f1286e72e82406ffd1f49acf4b331a
PR #320 hosted CI run 35864935626 / #9089: SUCCESS (6/6)

Accepted Phase-69.A checkpoints:
PR #314 -> d34058220653496a35fb7e1c073cc6981f4c72db
PR #315 -> bca8ff3bfd1216bda39f29105911f64853bd263c
PR #315 hosted CI run 35818928861 / #9074: SUCCESS (6/6)

Accepted Phase-69.B checkpoints:
PR #316 -> 72a637cc6f18fc1c1c1a77859abb3ab1f15e2b03
PR #316 hosted CI run 35843808669 / #9081: SUCCESS (6/6)
PR #317 -> 5ff36d635072c63e1c46ccb377c830ec99988108
PR #317 hosted CI run 35846333459 / #9083: SUCCESS (6/6)
PR #318 -> e9d59b87349d7e1be9356efc5cd656f41e11ea12
PR #318 hosted CI run 35861961215 / #9085: SUCCESS (6/6)
PR #319 -> 0248db3d63626a87d391f7649a984adc97b45232
PR #319 hosted CI run 35863518112 / #9087: SUCCESS (6/6)
69.B=COMPLETED

Latest accepted Phase-68 transport merge checkpoint:
PR #309 -> ed2f451d824e43c4264dee6caf8ae49dbeb71a88

Phase-68.D accepted candidate:
PR #310 -> 8807d587536daa6c27ae72a3f88d40b0c7de3480
Hosted CI run 35594914471: SUCCESS
real yaVDR acceptance: PASS

Phase-68.E accepted runtime candidate:
PR #311 -> 2b74b26ed33ce1a555a9cc037d44ad6768480eb2
Hosted CI run 35601863603: SUCCESS (6/6)
real yaVDR acceptance: RESULT=PHASE68E_REAL_ACCEPTANCE_PASS

Phase-68.F merge checkpoint:
PR #312 -> e3f9215f5f80dd230e1e855e3ce09f2ac70231ef
Hosted CI run 35610473828: SUCCESS

Phase-68.G accepted real-runtime candidate:
PR #313 runtime candidate -> 7ae51d090cbe06570b8a70e787137232df83f124
real yaVDR acceptance: RESULT=PHASE68G_REAL_NATIVE_OSD_INPUT_PASS

Latest Phase-67 Teletext merge checkpoint:
PR #293 -> d92e7907637122368908ce4c7564a0332db9c487

Phase-67 HbbTV / numbered closeout merge checkpoint:
PR #300 -> 5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30
```

The accepted post-Phase-66 Home rebuild branch `work/home-rebuild` ended at `0cce4d1c9e58abe4d529132e92340ae4cbb7a99c` and was merged to `main` as `ea5967b983aee9ccc3f855b685db01abbfb2326a`. The merge tree is identical to the accepted branch tree. This SHA is a durable Home-rebuild checkpoint, not a substitute for reading the live `main` head.

The later accepted post-phase runtime chain includes the Recordings 2 cinematic detail/presentation work, the related-Genre portrait correction, the Live-TV `ended` lifecycle stabilization, and the subsequent Recording-detail hardening through clickable cast/person search, corrected metadata navigation, strict paused prewarm and inline YouTube trailers. The current live `main` head must still be queried before repository-state actions; the runtime checkpoint above is historical evidence, not a permanent branch tip.

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
- PR #291 documentation of the accepted Live-TV lifecycle stabilization;
- PR #294 restoration of Recording actions plus paused Hero prewarm;
- PR #295 Recording detail metadata-cache fallback and technical playback/marks layout repair;
- PR #296 clickable Hero cast with canonical local person search and neutral related-card styling;
- PR #297 bottom Metadata navigation plus end-to-end preservation of `autoPlay:false` through fallback/restart-seek decorators;
- PR #298 explicit server-side TMDB Trailer lookup with privacy-enhanced inline YouTube playback below related Recording cards.

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
- restored Recording actions through the existing action workflow;
- Recording-cache metadata/artwork fallback for the selected detail without restoring Home-wide metadata fan-out;
- clickable cast/person presentation backed by the existing local person-search owner;
- related Recording rails that reuse canonical Recording navigation;
- metadata, marks and playback submodes retained inside the same Recordings 2 detail owner;
- Metadata-mode bottom navigation for `Aufnahme | Scraper | Schauspieler | Bilder`, including usable mobile 2 x 2 layout;
- canonical-owner playback prewarm that buffers/prepares only and preserves `autoPlay:false` through compatibility decorators;
- related-Genre cards using canonical `kind=poster` selection so locked manual posters remain authoritative and native portrait metadata wins before weaker preferred artwork fallback;
- conditional explicit Trailer lookup through the server-side TMDB provider and canonical Web Client API owner;
- validated `youtube-nocookie.com` embedding with autoplay disabled and an inline Trailer section below related Recording cards;
- real yaVDR/browser/mobile acceptance for the Hero, playback/prewarm, metadata navigation, cast search and inline Trailer behavior.

This presentation work does not create a new Recording identity, metadata authority or playback lifecycle. TMDB credentials remain server-side, YouTube Trailer playback is not a Recording MediaSession, and normal detail rendering does not perform an external Trailer lookup. Native Recording marks/cutting remains separately governed by the accepted native Recording editing architecture and closeout.

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

## Completed Phase-67 boundary

**Phase 67 is completed.** Teletext merged through PR #293 and the HbbTV discovery/session/presentation-media vertical merged through PR #300 / `5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30`.

Accepted Broadcast Companion capability includes:

- normalized Teletext service/page/subpage contracts and first-party 25 x 40 rendering;
- private SuiteBridge/provider transport through the existing Phase-63 Agent path;
- backend-generation, channel/application-context and session fencing;
- normalized HbbTV application discovery with backend/service/application provenance;
- authorized Suite-owned HbbTV application sessions with normalized bounded input;
- session-owned presentation/media integration without exposing raw browser/plugin commands;
- preservation of Phase-65 MediaSession ownership whenever Suite-owned media is involved;
- real yaVDR/browser acceptance for Teletext Journey 8 and HbbTV Journey 9 on the supported deployment profile;
- full PR #300 CI, including architecture, frontend, packaging/install-staging and fast regression.

Durable evidence is in [Phase 67 Closeout](development/phase-67-closeout.md) and the earlier [Phase 67 Teletext Closeout](development/phase-67-teletext-closeout.md).

Phase 68 Legacy OSD Compatibility Bridge is **completed for the accepted 68.A-G scope**. The final 68.G real yaVDR acceptance proved fenced allowlisted native OSD input through the authenticated Control Plane -> Agent -> SuiteBridge -> VDR path, including native DOWN/UP effect, idempotent replay and stale-authority rejection. Durable evidence is in [Phase 68 Closeout](development/phase-68-closeout.md) and [Phase 68 Kickoff](development/phase-68-legacy-osd-kickoff.md). Phase 69 public API/client compatibility hardening is active at 69.C Revision/precondition/idempotency exposure; 69.A and 69.B are accepted. Phase 70 recommendation work remains later.

## Historical evidence rule

Exact old candidate SHAs, CI runs and once-open follow-up notes remain valid in their closeouts as historical evidence. They are not current execution authority. Use this file for volatile status and query GitHub for the live head before acting.
