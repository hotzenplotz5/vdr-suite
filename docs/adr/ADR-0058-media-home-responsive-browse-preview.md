# ADR-0058: Media Home, Responsive Browse and Preview Experience

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Phase 66 Media Home and Browse Experience](../development/phase-66-media-home-browse-experience.md)
- [ADR-0030: Domain-First UI Over OSD Proxy](ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053: Client Playback Engine and Media Adaptation Strategy](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0054: Broadcast Companion Services — Teletext and HbbTV](ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0056: Playback Presentation, Timeline, Continuity and Failure Semantics](ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)

---

## Status

**Proposed**

Date: 2026-08-28

This ADR proposes the product and architecture boundary for a Media Home / Browse experience after completed Phase 65. It does not by itself start Phase 66 runtime work. If accepted, the Strict Roadmap and Phase Map must be reconciled explicitly before implementation begins.

---

## Context

Phase 65 completed the VDR-Suite media foundation needed for first-party Recording and Live-TV playback:

```text
Suite resource
  -> authorized MediaSession
  -> explicit provider / route ownership
  -> normalized MediaPlaybackContract
  -> persistent first-party playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

The current Web frontend exposes capable domain-specific views, but the product still lacks a single high-quality entry surface that combines the most common media tasks into a coherent browsing experience.

The desired product direction is a modern media-center home screen inspired by mature browsing patterns from premium media applications without copying their service model or introducing a new media architecture.

The initial product concept is:

```text
Media Home
  -> Live-TV hero browsing
  -> Continue Watching
  -> Newly Recorded
  -> Genre discovery
  -> Series / Recording folders
  -> Recently Watched
```

Live-TV browsing should feel immediate. Preview playback is secondary and must not make focus movement, keyboard navigation, remote-style browsing or touch swiping wait for stream startup.

The same information architecture must work in desktop browsers, tablets and mobile browsers, but a phone must not receive a merely scaled-down desktop layout.

Later native Windows, Android/Android TV or television clients may use a more immersive cinematic presentation while reusing the same Suite domain and media semantics. Native client implementation is not part of this ADR's initial Web runtime scope.

---

## Decision

VDR-Suite introduces **Media Home** as a first-party product composition over existing Suite domains.

The core product rule is:

> **Browse first, playback second.**

Browsing state is UI state. Playback/session state remains owned by the established Phase-65 media architecture.

The target composition is:

```text
existing Channel / EPG / Recording / Metadata domains
             |
             v
      Media Home projection
             |
      +------+------------------------------+
      |                                     |
 Live-TV hero                         content rails
      |                                     |
 deferred preview                    Continue Watching
      |                              Newly Recorded
 Phase-65 MediaSession               Genres
 / playback owner                    Folders / Series
                                     Recently Watched
```

Media Home is not a new source of truth for Channel, ProgramEvent, Recording, Genre, artwork, playback capability or MediaSession ownership.

---

# Product principles

## 1. Browse first, playback second

A focus, keyboard, pointer or swipe transition updates the browsing UI immediately.

Preview startup is deferred until the focus has remained stable for a bounded settle period.

Conceptually:

```text
focus change
  -> render focused content immediately
  -> cancel obsolete pending preview intent
  -> bounded settle period
  -> verify focus token still current
  -> request authorized preview MediaSession
  -> attach through existing playback ownership
```

No browsing action waits for MediaSession creation, FFmpeg startup, buffering or decoder readiness.

The exact settle duration is a UX tuning parameter, not a public architecture constant.

## 2. Preview is not lifecycle authority

The preview surface may request, attach, relinquish and replace playback through the existing canonical owner/session contracts.

It does not create:

- a second MediaSession owner;
- a second cleanup path;
- a second restart architecture;
- a provider-native stream path;
- a hidden background player with independent authority.

DOM presence, focus, animation completion and timeout expiry are not MediaSession authority.

## 3. Existing domains remain authoritative

Media Home reuses existing domain truth:

- Channel identity and ordering;
- current/next ProgramEvent data;
- Recording identity and hierarchy;
- Suite metadata and Genre assignments;
- artwork/cache contracts;
- normalized playback capability and continuity semantics;
- existing authorization and backend scope.

No Home-specific metadata database is introduced.

## 4. Responsive recomposition, not desktop shrinking

The Web product has one semantic Home experience with device-appropriate composition.

### Desktop

- large Live-TV hero;
- focused channel/programme in the visual center;
- direct neighboring channels visibly de-emphasized;
- optional further neighbors only as subtle context;
- delayed preview in a secondary area, typically upper right;
- one dominant next row visible immediately, initially `Continue Watching`;
- additional rails below the fold.

### Tablet

- hero consumes most horizontal space;
- direct neighbor peeks remain discoverable;
- preview may be integrated into the hero or use a smaller secondary pane according to orientation;
- touch and keyboard navigation remain first-class.

### Mobile portrait

- one dominant hero card;
- only narrow previous/next peeks;
- horizontal swipe for channel browsing;
- compact `Now` / `Next` information below the hero;
- primary actions such as `Watch Live` and `EPG` remain thumb-sized;
- Live preview is rendered inside the hero rather than as a persistent floating mini-player;
- bottom primary navigation is preferred for Home / Live / Recordings / Search / More;
- secondary functions such as Timer, EPG and Settings remain reachable without occupying permanent primary-navigation space.

### Mobile landscape

- may approach tablet composition;
- preview may become more prominent when space permits;
- no layout assumption may depend solely on device user-agent strings.

Responsive behavior is capability/layout driven through CSS and explicit frontend state, not browser-brand routing.

## 5. Visual hierarchy

The Home surface intentionally avoids showing every possible product section at once.

Initial hierarchy:

```text
Live-TV hero
Continue Watching
Newly Recorded
Genres
Series / Recording folders
Recently Watched
```

On initial desktop view, the hero plus one prominent next rail is preferred over a dense dashboard containing all sections simultaneously.

## 6. Live-TV hero semantics

The hero represents browsing selection, not automatically the currently playing channel.

It may show:

- Channel logo/name;
- current ProgramEvent;
- start/end time;
- current event progress;
- Genre/category where available;
- short description;
- next ProgramEvent;
- existing authenticated artwork/backdrop where available.

Default browsing may favor the user's configured/familiar channel favorites when such a stable existing concept is available. The implementation must not invent a new opaque recommendation system merely to order channels.

## 7. Explicit playback intent

`Browse focus`, `preview playback` and `Watch Live` are distinct intents.

- Browse focus changes immediately and may never create a session if focus keeps moving.
- Preview is delayed and disposable.
- `Watch Live` is an explicit user playback action and enters the established full playback experience.

Moving away from a focused hero cancels an obsolete pending preview. If a preview is already active, ownership is relinquished through the canonical playback lifecycle.

## 8. Continue Watching and Recently Watched are distinct

`Continue Watching` means unfinished content for which a truthful resume position is available.

`Recently Watched` is history/order information and may include completed items.

The implementation must first reuse accepted persisted state if one exists. It must not silently promote browser-local storage into server-authoritative cross-client resume/history truth.

If durable history requires a new server contract, that contract must be introduced as a bounded slice with explicit identity, actor scope, privacy and retention semantics.

## 9. Content rails are projections, not new collections of authority

Rails such as:

- Newly Recorded;
- Genre rows;
- Series;
- Recording folders;
- Recently Watched;

must be projections over existing query/domain contracts wherever possible.

Opening an item routes into the established owning detail/list experience rather than duplicating Recording actions inside a parallel Home-specific domain.

---

# Interaction model

## Directional navigation

The layout must preserve predictable navigation suitable for keyboard and future remote/D-pad clients:

```text
left / right
  -> move within current horizontal rail / hero carousel

up / down
  -> move between semantic Home sections

enter / OK
  -> activate focused item or primary action

back
  -> return to previous navigation level without losing canonical playback ownership
```

Touch uses swipe/scroll equivalents without making pointer/touch behavior the semantic authority.

## Focus visibility

The currently focusable/selected item must be visually unambiguous.

Focus treatment must remain readable under:

- dark artwork;
- bright artwork;
- missing artwork fallback;
- keyboard navigation;
- reduced-motion mode;
- desktop and mobile layouts.

## Motion

Transitions may use bounded fade/scale/slide effects, but motion never controls:

- MediaSession ownership;
- Channel identity;
- Recording identity;
- playback completion;
- provider choice;
- cleanup correctness.

Reduced-motion preferences must remain supported.

---

# Client presentation strategy

The Web Home experience and later native clients share design language and semantic contracts but do not require identical composition.

```text
Web browser
  -> information-rich responsive Home
  -> desktop/tablet/mobile recomposition

Later native desktop/mobile/TV clients
  -> may use more immersive cinematic/full-screen composition
  -> reuse the same Suite identities, playback contracts and domain projections
```

The more immersive cinematic presentation discussed for future Windows/Android/TV applications is intentionally not a Phase-66 Web completion requirement.

---

# Accessibility and performance rules

The initial implementation must account for:

- keyboard navigation;
- visible focus;
- semantic labels and useful screen-reader order;
- minimum practical touch targets;
- reduced motion;
- high contrast over artwork;
- bounded image loading;
- lazy loading below the first visible rails;
- cancellation of obsolete artwork/data/preview work;
- no unbounded simultaneous preview sessions;
- no full-page blocking on slow preview startup.

Artwork failure must degrade to deterministic branded/metadata fallbacks rather than broken layout.

---

# Proposed numbered-phase reconciliation

Phase 65 is completed. No future phase from the current 66+ sequence has started runtime implementation.

If this ADR is accepted, the proposed sequence becomes:

```text
Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

This change modifies only the future execution sequence.

It does **not** supersede the domain architecture of ADR-0054, ADR-0047 or ADR-0048.

Specifically:

- ADR-0054 remains the binding Broadcast Companion architecture, but its old `Phase 66` sequencing statement would move to Phase 67;
- ADR-0047 remains the Legacy OSD architecture, moving to Phase 68;
- ADR-0048 remains the Public API hardening architecture, moving to Phase 69;
- Recommendation / Knowledge Graph moves to Phase 70 and still requires its own accepted runtime ADR before implementation.

Completed history is never renumbered.

---

# Proposed Phase-66 slices

The implementation contract is maintained in [Phase 66 Media Home and Browse Experience](../development/phase-66-media-home-browse-experience.md).

Proposed order:

```text
66.1 Home Shell and Responsive Information Architecture
66.2 Live-TV Hero Carousel
66.3 Deferred Live Preview
66.4 Continue Watching
66.5 Recording Discovery Rails
66.6 Recently Watched / History
66.7 Visual Polish and Accessibility
66.8 Golden User Journey and Real-System Acceptance
```

Each slice must remain coherent and must not pull later native-client or recommendation work forward merely because the Home layout could display it.

---

# Explicit non-goals

This ADR does not authorize:

- Recommendation AI or knowledge-graph ranking;
- `Because you watched ...` personalization;
- Live-TV timeshift;
- Teletext/HbbTV runtime;
- Legacy OSD runtime;
- Public API stabilization;
- a new universal player/decoder;
- a second MediaSession or playback owner;
- provider-native URL access;
- a Home-specific metadata database;
- a general design-system rewrite unrelated to Home requirements;
- native Windows, Android, Android TV, Apple TV or television applications;
- hidden provider/profile switching;
- autoplay of multiple preview streams;
- browser-brand/user-agent playback routing.

---

# Consequences

## Positive

- Phase-65 media capability becomes accessible through a first-class product entry experience.
- Live-TV and Recording discovery gain one coherent visual hierarchy.
- Preview startup cost is prevented from degrading browsing responsiveness.
- Responsive mobile behavior is designed explicitly rather than patched after desktop implementation.
- Existing identity, metadata and playback boundaries remain reusable across future native clients.
- Future cinematic native clients can evolve presentation without redefining server/media semantics.

## Costs / risks

- Home combines several existing domains, so production composition tests must prove routing and owner integration rather than isolated cards only.
- Deferred preview introduces cancellation and stale-focus races that require explicit tests.
- Continue Watching / Recently Watched may expose a missing durable history contract; implementation must not fabricate one.
- Artwork-rich presentation can regress load performance unless loading and cancellation remain bounded.
- Numbered future phases require coordinated documentation reconciliation upon ADR acceptance.

---

# Acceptance requirements for this ADR

Before this ADR becomes **Accepted**:

1. Phase 65 remains verified closed on current `main`.
2. Phase 66 Broadcast Companion runtime has not started.
3. The Strict Roadmap and Phase Map can be reconciled without renumbering completed history.
4. ADR-0054/0047/0048 architecture remains intact while only their future phase numbers move.
5. The proposed Phase-66 implementation contract contains bounded slices and explicit non-goals.
6. Golden User Journey coverage is defined for desktop and mobile Home browsing.
7. No runtime/frontend code is included in the architecture-acceptance change.

After acceptance, Phase 66 still requires a separate explicit runtime kickoff before implementation starts.

---

## Decision relationship

```text
ADR-0058
  -> proposes Media Home architecture and future sequencing reconciliation
  -> does not start Phase 66 runtime
  -> preserves Phase-65 media ownership
  -> preserves ADR-0054 Broadcast Companion architecture at later phase number
```
