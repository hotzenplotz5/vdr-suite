# Phase 66 — Media Home and Browse Experience

Status: **Runtime active — Slice 66.3 Deferred Live Preview. Slices 66.1 and 66.2 are completed; Slice 66.4+ is not authorized.**

This document defines the bounded implementation plan for a new VDR-Suite Media Home after completed Phase 65. It is intentionally a product-composition phase, not a replacement for existing Channel, EPG, Recording, Metadata or MediaSession architecture.

## Navigation

- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [ADR-0058 Media Home, Responsive Browse and Preview Experience](../adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR-0046 Streaming Gateway](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053 Client Playback Engine](../adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0056 Playback Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [Frontend Playback Integration Contract](frontend-playback-integration-contract.md)

---

# Purpose

Phase 65 established reliable first-party Live-TV and Recording playback. Phase 66 turns those capabilities into a high-quality primary product entry surface.

The target experience is:

```text
open VDR-Suite
  -> Media Home
       -> browse Live-TV immediately
       -> optional delayed Live preview
       -> Continue Watching
       -> Newly Recorded
       -> Genres
       -> Series / Recording folders
       -> Recently Watched
```

The product principle is:

> **Browse first, playback second.**

A slow preview, MediaSession startup, image request or backend must never make channel/content focus movement feel blocked.

---

# Preserved architecture

Phase 66 must preserve all accepted ownership boundaries from previous phases.

## Existing domain authority

Media Home consumes, rather than replaces:

- Channel and channel-order/favorite truth;
- ProgramEvent / EPG truth;
- Recording identity and hierarchy;
- metadata, Genre and artwork truth;
- authorization and backend scope;
- Phase-65 MediaSession / MediaRoute / provider ownership;
- normalized `MediaPlaybackContract`;
- canonical playback-owner lifecycle;
- playback continuity, generation and failure semantics.

## Forbidden parallel architecture

Phase 66 must not introduce:

- a second media/session owner;
- a second restart/cleanup engine;
- provider-native stream URLs;
- a Home-only media protocol;
- a Home metadata database;
- a separate Recording identity;
- a universal decoder/player;
- hidden provider/profile fallback;
- background autoplay of multiple streams.

The existing playback owner remains canonical whenever Home requests preview or explicit playback.

---

# Visual product direction

The Web visual reference is a reduced premium media-center layout rather than a dense information dashboard.

## Desktop

Initial viewport should emphasize only:

```text
primary navigation
Live-TV hero
Now / Next + primary actions
Continue Watching
```

Additional sections belong below the first viewport.

The Live-TV hero uses:

- one dominant focused channel/programme;
- one direct previous/next neighbor clearly discoverable;
- optional outer neighbors only as subtle faded context;
- strong artwork/branding fallback hierarchy;
- current-event progress;
- compact metadata;
- secondary delayed preview, normally upper/right where space permits.

The page must not show all rails at once merely because the screen is large.

## Tablet

- hero dominates;
- neighboring items remain partially visible;
- touch swipe and keyboard navigation are equivalent navigation modes;
- preview placement may switch between a side pane and hero-integrated mode depending on available width/orientation.

## Mobile portrait

Mobile is recomposed, not scaled down.

Target:

```text
VDR-Suite                         Search

LIVE-TV · Favorites

      previous peek
          [ focused hero ]
                         next peek

Now                      Next
current programme        next programme
progress / time

[ Watch Live ] [ EPG ]

CONTINUE WATCHING       See all >
[card] [card] [card] ...

---------------------------------------
Home | Live | Recordings | Search | More
```

Rules:

- no persistent floating mini-player stealing display area;
- delayed preview replaces/fades into the hero media area;
- swiping to another channel relinquishes/cancels obsolete preview work;
- primary touch targets remain comfortably tappable;
- secondary product sections live below the fold.

## Mobile landscape

May use a tablet-like composition when space permits. No user-agent-based breakpoint logic is required or desired.

---

# Information hierarchy

Initial fixed order:

1. Live-TV hero / favorites
2. Continue Watching
3. Newly Recorded
4. Genres
5. Series / Recording folders
6. Recently Watched

Personalized rail reordering is explicitly deferred. Version 1 uses a deterministic product order.

---

# Slice 66.1 — Home Shell and Responsive Information Architecture

Status: **Completed through PR #231.**

## Goal

Create the new Home composition and responsive navigation without introducing Live preview or new history persistence.

## Required work

- define Home as the primary first-party landing view;
- retain direct navigation to existing Live-TV, Recordings, Search, Timer/EPG and Settings surfaces;
- desktop top/side navigation compatible with current shell;
- mobile bottom primary navigation:

```text
Home
Live
Recordings
Search
More
```

- `More` keeps secondary functions reachable, including Timer, EPG, Settings and other appropriate existing surfaces;
- define reusable Home section/rail composition without duplicating owning domain logic;
- responsive desktop/tablet/mobile composition;
- keyboard, pointer and touch interaction foundation;
- loading, empty, degraded and missing-artwork states.

## Hard boundary

No preview MediaSession is started in Slice 66.1 unless absolutely required to prove an existing shell integration. Slice 66.1 is primarily layout/composition/navigation.

## Regression proof

At minimum:

1. Home is reachable as the first-party landing route;
2. existing primary pages remain reachable;
3. desktop and mobile layouts expose the same semantic sections without horizontal page overflow;
4. keyboard focus order is deterministic;
5. mobile primary navigation does not make Timer/EPG/Settings unreachable;
6. existing persistent playback owner survives Home navigation according to its accepted contract.

---

# Slice 66.2 — Live-TV Hero Carousel

Status: **Completed through PR #232 plus the real-browser keyboard-focus correction in PR #233.**

## Goal

Provide immediate, visually strong Live-TV browsing independent of stream startup.

## Required semantics

Hero selection state is separate from playback state.

Conceptually:

```text
heroSelection
  -> channelId
  -> current ProgramEvent
  -> next ProgramEvent
  -> artwork/logo/metadata projection
```

Moving selection must not implicitly mean `Watch Live`.

## Interaction

- left/right changes the channel within the active channel set;
- touch swipe provides equivalent movement;
- direct neighbors remain visible as context;
- vertical movement transitions to/from other Home rails;
- explicit `Watch Live` enters normal playback;
- explicit `EPG` opens the existing EPG/domain flow.

## Channel set

Prefer an existing stable favorites/channel-order concept when available.

Do not create recommendation ranking as a substitute for missing favorites.

## Regression proof

At minimum:

1. rapid channel browsing makes no MediaSession request;
2. selection follows the canonical channel order/set;
3. current/next EPG changes with focus without mutating playback;
4. same focused channel renders consistently on desktop and mobile;
5. missing artwork/logo degrades without layout collapse.

---

# Slice 66.3 — Deferred Live Preview

Status: **Active runtime slice. Draft PR #234. Not complete until exact-head hosted CI and mandatory real yaVDR browser acceptance pass.**

## Goal

Add optional Live preview without degrading browse responsiveness or creating another player lifecycle.

## Canonical flow

```text
focus changes
  -> increment / replace focus token
  -> render immediately
  -> cancel prior pending preview intent
  -> bounded settle timer
  -> verify focus token and Home ownership
  -> request preview through existing authorized media path
  -> attach through canonical playback owner
```

If focus changes while preview startup is in flight, stale work must not attach as if it were current.

If an active preview is superseded, relinquish it through the canonical owner lifecycle.

## Priority rule

Browsing always has higher priority than preview.

A user may navigate across many channels without waiting for preview teardown/startup between each move.

## Desktop presentation

Preview may appear as a secondary small player in the upper-right/secondary hero area.

## Mobile presentation

Preview appears inside the focused hero media area. No permanent floating mini-player is required.

## Failure behavior

Preview failure is non-blocking UI evidence. It must not silently:

- switch provider;
- invent another profile;
- seize explicit full-playback ownership;
- prevent further browsing.

The user may still use explicit playback actions if the normalized capability permits them.

## Regression proof

At minimum:

1. focus changes faster than the settle threshold create zero preview sessions;
2. settled focus creates at most one current preview session;
3. stale in-flight preview cannot attach after focus moved;
4. active preview relinquishes deterministically when browsing moves away;
5. explicit full playback remains distinguishable from preview ownership;
6. production composition uses the existing owner/session lifecycle rather than a test-only player.

## Real-system gate

Real yaVDR browser acceptance is mandatory because this slice changes actual Live media behavior.

---

# Slice 66.4 — Continue Watching

## Goal

Expose unfinished Recording playback prominently and resume through accepted playback semantics.

## Required semantics

A Continue Watching item needs truthful evidence for:

```text
recordingId
resume position
recording duration / playable extent where known
last relevant activity ordering
playback capability
```

The implementation must first identify the accepted existing source of resume truth.

Browser-local storage must not silently become cross-client server truth.

If no durable actor-scoped resume/history contract exists, the missing contract must be proven and introduced as the smallest coherent domain addition before presenting cross-device persistence claims.

## UI

Each card should provide:

- artwork/fallback;
- title and optional episode/subtitle;
- progress indicator;
- truthful position/duration when known;
- primary `Continue` action;
- secondary `From beginning` where supported.

## Regression proof

At minimum:

1. only resumable unfinished items appear;
2. resume uses canonical absolute Recording position;
3. finished/non-resumable items do not receive fake progress;
4. `Continue` and `From beginning` use established playback ownership;
5. mobile and desktop use the same semantic item identity.

---

# Slice 66.5 — Recording Discovery Rails

## Goal

Turn existing Recording/metadata queries into browseable Home rails without creating a second content catalog.

## Initial rails

- Newly Recorded;
- Genres;
- Series where supported by existing metadata/hierarchy;
- Recording folders / library hierarchy.

## Rules

- Genre uses canonical Suite Genre assignments;
- folder navigation uses existing Recording hierarchy;
- opening a Recording enters the owning Recording detail/playback flow;
- opening a folder/genre enters an established or bounded browse route;
- artwork remains authenticated/bounded according to current cache contracts;
- rail loading below the fold is lazy/bounded.

## Regression proof

At minimum:

1. rail membership agrees with the owning query/domain result;
2. no duplicate Home-only Recording identity is introduced;
3. backend scope remains explicit;
4. stale/failed one-rail loading does not block the entire Home page;
5. opening an item preserves existing domain routes/actions.

---

# Slice 66.6 — Recently Watched / History

## Goal

Expose recent viewing activity separately from Continue Watching.

## Required semantics

History must be actor-scoped and distinguish at least:

```text
content identity
activity time/order
completion/resume relevance where known
source of evidence
```

Do not infer global user history from transient DOM state.

## Boundary

This slice may require a new bounded persistence contract only if current accepted data cannot support the product truthfully.

If introduced, persistence must define:

- actor scope;
- supported content identity;
- retention/pruning;
- update semantics;
- privacy/access;
- no hidden recommendation authority.

## Regression proof

- Recently Watched and Continue Watching can differ;
- completed content may remain in Recently Watched;
- actor isolation is enforced if persistence is server-side;
- history failure does not break Live/Recording browsing.

---

# Slice 66.7 — Visual Polish and Accessibility

## Goal

Apply the premium Web presentation after information architecture and lifecycle correctness are stable.

## Visual direction

- dark cinematic VDR-Suite identity;
- large artwork with bounded gradients;
- clear focused item;
- subtle de-emphasis of neighboring hero cards;
- restrained depth/transparency;
- smooth but bounded transitions;
- progress indicators readable without overpowering artwork;
- fallback artwork that still looks intentional.

The Web target is the cleaner reduced premium concept, not the more immersive native-app cinematic concept.

## Required accessibility/performance

- visible keyboard focus;
- reduced motion;
- semantic labels;
- practical touch targets;
- contrast over dynamic artwork;
- image lazy loading/cancellation;
- no layout shift from missing artwork dimensions;
- responsive checks at mobile, tablet, 1080p and 4K-class widths.

## Boundary

No unrelated global design-system rewrite.

---

# Slice 66.8 — Golden User Journey and Real-System Acceptance

## Goal

Close Phase 66 only after the complete product path works on real supported browser layouts.

## Golden Web journey

```text
open VDR-Suite
  -> Home is useful before preview is ready
  -> browse Live-TV quickly
  -> pause focus
  -> delayed preview appears
  -> continue browsing without blocking
  -> explicitly Watch Live
  -> return to Home
  -> Continue Watching Recording
  -> browse Newly Recorded / Genre
  -> open a Recording folder
```

## Golden mobile-browser journey

```text
open VDR-Suite on phone
  -> one dominant Live hero
  -> swipe between channels
  -> neighbor peeks make horizontal browsing discoverable
  -> preview appears inside hero only after focus settles
  -> Watch Live / EPG touch actions work
  -> Continue Watching horizontal rail works
  -> bottom navigation reaches Home / Live / Recordings / Search / More
  -> Timer / EPG / Settings remain reachable through secondary navigation
```

## Real acceptance matrix

At minimum for changed supported surfaces:

- desktop Chromium-family browser on real yaVDR;
- mobile browser on a real phone or equivalent actual supported device;
- portrait and landscape mobile behavior where materially different;
- 1080p desktop layout;
- 4K-class desktop layout or equivalent viewport validation;
- real Live preview start/relinquish;
- real Recording continue path if Slice 66.4 is implemented with durable truth.

## Phase-66 completion gate

Phase 66 closes only when:

1. Media Home is the accepted first-party landing experience;
2. desktop/tablet/mobile composition is responsive rather than desktop-scaled;
3. Live-TV browsing remains responsive independent of preview startup;
4. deferred preview uses canonical Phase-65 ownership and deterministic cleanup;
5. Continue Watching is truthful and resumes canonical absolute position;
6. Recording discovery rails consume existing domain truth;
7. Recently Watched, if implemented, uses explicit actor/history semantics;
8. keyboard/touch/focus/accessibility gates pass;
9. Golden desktop and mobile journeys pass on real supported environments;
10. full final-head CI and packaging/install regression pass;
11. rollback remains documented and no provider/session/privacy boundary is weakened.

---

# Explicitly deferred beyond Phase 66

- Broadcast Companion / Teletext / HbbTV runtime;
- Legacy OSD runtime;
- public third-party API stabilization;
- recommendation/knowledge-graph ranking;
- AI recommendations;
- personalized Home rail ordering;
- Live-TV timeshift;
- native Windows application;
- native Android / Android TV application;
- native Apple / television application;
- premium full-screen native-app cinematic UX;
- universal media decoder/player;
- unrelated broad design-system rewrite.

---

# Native-client follow-up direction

The future native application family may deliberately use a more immersive cinematic layout than Web:

```text
shared VDR-Suite design language
+ shared domain/media contracts
+ platform-native player/runtime
+ client-specific composition
```

A Windows/Android/TV application may therefore use large backdrop-driven, remote-friendly or full-screen layouts without making that layout a server contract or a Phase-66 Web requirement.

---

# Accepted future phase sequence

Under accepted ADR-0058, the numbered sequence is:

```text
Phase 66 - Media Home and Browse Experience
Phase 67 - Broadcast Companion Services: Teletext and HbbTV
Phase 68 - Legacy OSD Compatibility Bridge
Phase 69 - Public API and Client Compatibility Hardening
Phase 70 - Recommendation and Content Knowledge Graph
```

ADR-0054/0047/0048 retain their architecture. Only not-yet-started future numbering changes.

---

# Current runtime boundary

Phase 66 runtime is active. Slice 66.1 and Slice 66.2 are completed; Slice 66.3 is the only currently authorized runtime scope.

Before every Slice-66.3 implementation or status mutation:

1. re-read current GitHub `main` and the Slice branch;
2. verify there is no competing work that would be overwritten;
3. preserve ADR-0058 browse-first and canonical playback-owner boundaries;
4. keep Slice 66.4+ and Phase 67+ closed;
5. require exact-head hosted CI plus real yaVDR browser acceptance before Slice 66.3 may be marked completed or merged.