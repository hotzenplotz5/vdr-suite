# VDR-Suite Current Project Status

## Status ownership

Exact operational phase state is maintained only in [Current State](../CURRENT.md). This file provides stable narrative context and must not become a second copy of active PR tips or transient CI state.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can perform the complete bounded edit safely. Read the complete file content required for the change, write a coherent commit on the intended branch and inspect the resulting diff before treating the update as correct.

Use local edits first only when the change requires:

- local build/test execution that cannot be represented by the connector;
- multi-file transformations that are materially safer in a checked-out worktree;
- binary/generated-file handling unavailable through the connector; or
- a workaround because the GitHub connector blocks a file operation.

GitHub-first does not weaken review safety: keep updates fast-forward-only, do not replace a complete file from a truncated fetch, and do not mark Draft PRs Ready or merge them without explicit approval.

## Platform position

Latest completed numbered runtime phase: **Phase 68 - Legacy OSD Compatibility Bridge**.

Current active numbered runtime phase: **Phase 69 - Public API and Client Compatibility Hardening**.

Next strict numbered runtime phase: **Phase 69 - Public API and Client Compatibility Hardening**.

Current active runtime slice: **69.C - Revision/precondition/idempotency exposure**. The accepted chain now includes public durable Timer CREATE admission, dormant reservation/dispatch/reconciliation prerequisites, typed Agent outcome evidence/application and readback reconciliation. The current bounded candidate is [Native Timer CREATE Productive Runtime](phase-69c-native-timer-create-productive-runtime.md): the existing daemon poll advances durable `timer.create` operations through exact reservation/claim/activation and then only through durable Agent result plus authoritative complete Timer readback to verified binding, bound assignment and succeeded operation. It adds no Timer-specific poller, retry queue, lifecycle or repository; `outcome_unknown` is reconciliation-only. Timer mutations remain on typed SVDRP under the closed ADR-0064 decision. This candidate has `NATIVE_EFFECT_REACHABLE=YES` and requires exact-head real yaVDR acceptance before merge. Phase 69.A route/resource inventory and Phase 69.B request/error foundations remain accepted; see [Phase 69.B Closeout](phase-69b-closeout.md) and [Phase 69 Public API Kickoff and Runtime Progress](phase-69-public-api-kickoff.md).

Accepted Phase-68 slices include **68.A semantic observation, 68.B Agent-local continuity/resynchronization, 68.C authenticated Agent transport, 68.D authorized bounded view sessions, 68.E bounded viewer bindings/multi-viewer delivery, 68.F exclusive controller leasing and 68.G allowlisted native OSD input**.

Phase 68 is completed for this accepted scope. Durable evidence: [Phase 68 Closeout](phase-68-closeout.md), [Phase 68 Kickoff](phase-68-legacy-osd-kickoff.md), [Phase 68.D View-Session Closeout](phase-68d-view-session-authorization-closeout.md) and [Phase 68.E Viewer-Binding Closeout](phase-68e-viewer-bindings-closeout.md).

Phase 67 completed both Broadcast Companion verticals: **Teletext** and **HbbTV discovery/application-session/presentation-media runtime**.

Phase 66 is completed. Later Home/Recording/Live performance and correctness work is non-numbered post-phase hardening and is also completed for the merged accepted scopes. The consolidated Home evidence is in [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md); the accepted current Recording-detail presentation is captured in [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md).

## Durable foundation

The current platform builds on these completed numbered foundations:

- **Phase 62 - Identity, RBAC and Accountability Foundation** — persistent identity, backend-scoped authorization, browser-session/CSRF protection and append-only accountability;
- **Phase 63 - Backend Agent and Secure Multi-Site Runtime** — Agent identity, generation/lease fencing, observation/command handling and explicit provider ownership;
- **Phase 64 - Timer Intent and Multi-Backend Orchestration** — `TimerIntent -> TimerAssignment -> NativeTimerBinding`, managed native fulfillment, authoritative reconciliation and controlled failover;
- **Phase 65 - Streaming Gateway and Media Sessions** — authenticated Recording/Live MediaSessions, least-transformation delivery/output policy and normalized persistent playback ownership;
- **Phase 66 - Media Home and Browse Experience** — responsive first-party Home, Live hero/preview, Continue Watching, Recording discovery, history and Golden desktop/mobile acceptance.

## Post-Phase-66 delivered state

The post-phase work did not create a new numbered phase. It hardened and completed product behavior on top of the accepted Phase-66 ownership model.

### Home performance and lifecycle

Merged work preserves immediate browse behavior while reducing unnecessary rebuilds and duplicated work:

- Hero browsing does not rebuild programme rails;
- same-backend warm Home return reuses complete projections where valid;
- Recording Discovery/Series work is progressively fenced and avoids global metadata fan-out;
- recent movie expansion preserves the mounted rail and position;
- EPG cache timeout recovery and preview-cache fast paths prevent avoidable stale/fallback presentation;
- the public EPG artwork/metadata cache routes required by the Home/metadata readers are restored and guarded after the H2/H2.1 rewrite regression.

### Home rebuild / current presentation

The merged post-Phase-66 Home rebuild establishes a coherent current presentation across:

- Home shell and responsive navigation;
- `Was läuft jetzt` / `Was läuft danach`;
- newly recorded content;
- Movies / recent movies / Genres;
- Series -> seasons -> episodes;
- canonical TVScraper/native artwork priority;
- manual Series hierarchy and artwork overrides;
- pilot/miniseries, TV-film and special handling;
- reset-to-automatic hierarchy semantics;
- canonical folder-card metadata/artwork without per-Recording browser metadata reads.

The final folder-poster regression was accepted on the real yaVDR/browser path: `The Exorcist` again shows the canonical red TVScraper portrait in Home -> Aufnahmeordner -> Horror instead of a Recording still.

### Series metadata/artwork follow-up

The Phase-66 closeout historically recorded an unresolved Series metadata/artwork projection symptom. That item is no longer current. Subsequent merged work added explicit unsettled/settled metadata semantics, bounded retries, progressive/scoped completion, hierarchy repair and retained canonical Series ownership. The later Home rebuild completed manual hierarchy/artwork behavior and protected the accepted no-fan-out performance boundary.

### Recording detail / cinematic Hero

The current Recordings 2 detail surface is no longer just the older technical detail layout. The accepted post-Phase-66 detail work provides:

- a cinematic/full-page Hero composition for the selected Recording;
- compact factual metadata and prominent playback entry;
- restored Recording actions through the existing action owner;
- selected-Recording metadata/artwork fallback from the existing Recording cache when richer TVScraper metadata is unavailable;
- clickable cast/person presentation backed by the canonical local person-search owner;
- related Recording cards using normal Recordings 2 navigation;
- retained metadata, marks and playback modes inside the canonical Recordings 2 detail owner;
- Metadata-mode bottom navigation for `Aufnahme | Scraper | Schauspieler | Bilder`, including a usable 2 x 2 mobile layout;
- canonical-owner playback prewarm so explicit Play can start immediately while `autoPlay:false` is preserved through fallback/restart-seek decorators and prewarm itself never advances playback;
- corrected related-Genre poster selection where locked manual posters remain authoritative and native portrait metadata wins before weaker preferred-artwork fallback;
- explicit on-demand server-side TMDB Trailer lookup through the Web Client API owner;
- privacy-enhanced `youtube-nocookie.com` playback with autoplay disabled, rendered inline below related Recording cards instead of in a modal overlay;
- real yaVDR/browser/mobile validation including Hero rendering, local cast search, metadata navigation, paused prewarm and inline Trailer playback.

See [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md) for the accepted PR/runtime identity and boundaries through PR #298 / merge `2b0d0990974244eac90e9be711843678c77e301d`.

### Native Recording editing

Post-Phase-66 native Recording marks/cutting is also merged and documented separately in [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md). VDR remains the canonical marks/cutter authority. Later marks-control hardening must be treated separately until it has its own accepted merged evidence.

### Live-TV lifecycle stabilization

The completed Phase-65 persistent playback architecture remains authoritative. A later browser regression showed that continuous Live/MSE playback could emit `HTMLMediaElement.ended`; the persistent shell incorrectly translated that into a hard stop and destroyed the still-owned MediaSession, producing the observed backend `client_closed` terminal reason.

The accepted stabilization removes `ended` as an implicit Live-TV STOP boundary while retaining the real boundaries: explicit stop, backend change, browser-session loss, replacement handoff and fatal player error. The focused lifecycle regression passed and the corrected frontend ran on the real yaVDR/browser path beyond the previous failure window without dropping.

The durable evidence lives in [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md).

## Phase 67 completed state

**Phase 67 is completed.** Teletext merged through PR #293 and HbbTV discovery/session/presentation-media runtime merged through PR #300 / `5fe2b73abaeb85f2b5c2cecf7c0c86753aef3d30`.

The accepted Broadcast Companion runtime provides normalized Teletext reads/rendering plus fenced HbbTV discovery, Suite-owned application sessions, normalized input, session-owned presentation/media integration and stale-context cleanup while preserving Phase-65 media ownership.

See [Phase 67 Closeout](phase-67-closeout.md) and [Phase 67 Teletext Closeout](phase-67-teletext-closeout.md).

## Forward ordering

```text
Phase 64 reliable Timer orchestration engine [COMPLETED]
  -> Phase 65 Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 Media Home and Browse Experience [COMPLETED]
  -> Phase 67 Broadcast Companion Services: Teletext and HbbTV [COMPLETED]
  -> Phase 68 Legacy OSD Compatibility Bridge [COMPLETED]
  -> Phase 69 Public API and Client Compatibility Hardening
  -> Phase 70 Recommendation and Content Knowledge Graph
```

ADR-0054 remains the binding completed Broadcast Companion architecture. ADR-0047 owns the active Phase-68 Legacy OSD boundary.

## Retained deferred boundaries

The following are not unfinished Phase 65/66 work:

- user-visible growing-Recording seek where unsupported capability remains truthful;
- Live-TV timeshift;
- Legacy OSD compatibility (Phase 68);
- stable public `/api/v1` client contract (Phase 69);
- recommendation/content graph runtime (Phase 70);
- broad Timer Product UI and account/backend access administration as cross-cutting product milestones.

## Related documents

- [Current State](../CURRENT.md)
- [Phase 68.E Viewer-Binding Closeout](phase-68e-viewer-bindings-closeout.md)
- [Phase 67 Teletext Closeout](phase-67-teletext-closeout.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Recording Detail Closeout](post-phase66-recording-detail-closeout.md)
- [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 65 Closeout](phase-65-closeout.md)
- [Phase 64 Closeout](phase-64-closeout.md)
- [Phase 62 Closeout](phase-62-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
