# Android, Android TV and Client API Feasibility Study

## Status

Architecture and feasibility study. This document is planning and design evidence; it does not implement an Android application, does not create a new numbered runtime phase and does not replace Phase 62.

Repository baseline verified on 2026-07-27:

```text
origin/main
cb77ff66e11dca7db2eafa36525762dcde35102d
Merge pull request #115 from hotzenplotz5/agent/configurable-remote-mapping
```

PR #114 is merged at `729ff190c532b9cd59c6b7054cd52062409af26c`. PR #115 is the first commit after that documentation baseline and is part of the evidence used here.

## Executive decision

The recommended production direction is:

```text
native Android Mobile application
  Kotlin + Jetpack Compose + Coroutines/Flow
  Android Media3 for playback
  adaptive phone/tablet layouts

shared non-UI client core
  domain models
  authenticated API client
  cache and synchronization
  media-session client
  operation/error handling
  design tokens and shared assets

separate Android TV application surface
  Compose-based ten-foot UI
  D-pad and focus-first navigation
  TV-specific playback and overlays
```

A PWA is useful as a browser enhancement and a low-cost installable shell. A WebView or Trusted Web Activity can be used for a strictly labelled, read-only transition prototype. Neither should become the final application architecture because VDR-Suite's production target includes native media playback, device lifecycle, secure credentials, Picture-in-Picture, Android TV focus navigation, notifications and a client-independent public API.

The application must not bind directly to today's unversioned routes as a permanent contract. A PoC may use a replaceable compatibility adapter over selected read-only routes, with explicit non-stability and no public SDK promise. The stable product client must consume `/api/v1` after the Phase 67 contract exists.

## Repository and architecture summary

### Runtime authority and ownership

VDR remains authoritative for tuners, native schedules, native timers, recordings, replay, OSD and plugin execution.

VDR-Suite owns the external platform boundary:

- stable Suite identities and backend scope;
- Backend Registry, availability and server-side read-only policy;
- persistent read models and query repositories;
- metadata, people, artwork references and Genres;
- search and client-facing orchestration;
- Suite-owned RemoteAction and LiveOverlay contracts;
- future actor identity, authorization and accountability;
- future MediaSession and public API contracts.

Private technologies remain adapters or providers:

- RESTfulAPI;
- SVDRP;
- Streamdev;
- TVScraper;
- SuiteBridge;
- Agent protocols;
- VDR-native data structures and local paths.

No independent client may receive permanent credentials or stable URLs for those private technologies.

### Current runtime layers

```text
browser
  -> VdrSuiteClientApi JavaScript wrappers
  -> unversioned Suite HTTP routes
  -> ApiRouter plus specialised API runtimes
  -> controllers
  -> services
  -> repositories / snapshot read services / backend registry
  -> Suite SQLite, snapshot cache and private VDR adapters
```

Specialised API runtimes are composed before the general router in `ApiRouter::handleClientGet()` and `handleClientPost()`:

- `LiveRemoteApiRuntime`;
- `GlobalSearchApiRuntime`;
- `GenreBrowserApiRuntime`;
- then the general `ApiRouter`.

The current route surface is a transition API. It contains aliases, implicit default-backend behaviour, backend-native identifiers, ad-hoc error bodies and speculative client fallbacks. These are implementation facts, not a stable public contract.

### Data holding

The daemon owns SQLite. Implemented persistent models include backend registry/configuration, EPG cache, recording cache, metadata bindings, people relations, Genre evidence and assignments, global-search read structures and other runtime state.

Normal Genre and global-search GET paths are query-only and do not synchronously contact TVScraper or another provider. Dedicated read connections use `PRAGMA query_only=ON` where configured.

### Security state

Implemented security foundations:

- explicit backend identity;
- backend capabilities and availability;
- server-enforced backend read-only mode;
- domain-specific validation, preview and readback foundations for selected mutations;
- allowlisted remote actions.

Not yet complete:

- production actor identity and session lifecycle;
- centralized RBAC and backend/resource scopes;
- request and correlation IDs across the public boundary;
- append-only accountability and transactional outbox;
- universal resource revisions and `If-Match`;
- durable idempotency and operation reconciliation;
- session revocation and bounded mobile credentials.

Those gaps are Phase 62 and later prerequisites. Hiding a button in Android is never an authorization control.

### Roadmap position

```text
completed: Phase 61 and post-Phase-61 hardening/features
next:      Phase 62 Identity, RBAC and Accountability
then:      Phase 63 secure Backend Agents
           Phase 64 TimerIntent orchestration
           Phase 65 Streaming Gateway
           Phase 66 Legacy OSD bridge
           Phase 67 public API/client hardening
```

This study is cross-cutting design evidence for those phases. It does not insert an Android phase before Phase 62.

## Current browser frontend analysis

### Shell and navigation

`web/frontend/index.html` provides one responsive single-page shell. The main module navigation exposes:

- overview;
- Channels 2;
- Recordings 2;
- Genres;
- EPG;
- channel sorting;
- timers;
- SearchTimer.

The feature strip also launches global search and settings. Runtime code is split between eagerly loaded modules and deferred modules. `web/frontend/platform/deferred-runtime-loader.js` loads Recordings 2, metadata detail, Genres and EPG detail owners after shell startup.

The shell is responsive through CSS media queries, but it is still a browser document with one shared detail mount target. It is not a TV focus model and does not define a native application navigation stack.

### Current Client API boundary

The browser uses:

- `web/frontend/api/client-api.js`;
- `web/frontend/api/genre-client-api.js`;
- `web/frontend/api/live-remote-client-api.js`;
- `window.VdrSuiteClientApi`.

The positive boundary is real: most feature modules receive DOM-free functions instead of embedding provider calls.

Current limitations visible in the wrapper:

- all canonical routes are unversioned;
- route aliases are tried in the client;
- `requestJsonWithFallback*()` retries after any error, not only a proven `route_not_supported`;
- mutation fallbacks exist for SearchTimer operations;
- errors are collapsed into a JavaScript `Error` string;
- HTTP status, stable code, request ID, correlation ID and retry metadata are lost;
- backend query naming alternates between `backend` and `backendId`;
- no generated schema or compatibility version is negotiated.

`deferred-runtime-loader.js` also contains a legacy `window.fetch` interception for SearchTimer preview cache warmup. It is an internal browser compatibility mechanism, not a pattern for another client.

### Exact live image and streaming trace

The current browser frontend does **not** contain a `<video>` element and does not expose a Suite media playback route.

The implemented path is:

```text
remote dialog
  -> fetchClientLiveOverlay()
  -> GET /api/vdr/live/overlay?backend=<id>
  -> LiveRemoteApiRuntime
  -> LiveOverlayController / LiveOverlayService
  -> Snapshot read service + EPG cache
  -> private RestfulApiLiveChannelStateProvider
  -> structured JSON overlay

remote dialog
  -> createClientLiveUpdateSource()
  -> EventSource GET /api/vdr/live
  -> sequenced changed-domain notification
  -> refresh full overlay when liveOverlay changed
```

`web/frontend/modules/remote.js` renders only:

- current channel number/name or unavailable state;
- the current programme text;
- status and action feedback;
- the photorealistic remote image and hotspot controls.

It does not render video, a snapshot image or a Streamdev URL. The phrase “Live TV” in the shell is therefore not proof of live media playback.

`/api/vdr/live` is an SSE state-update transport. It is not the Media Plane. `LiveOverlay` is structured state. It is not a legacy OSD frame stream and not a `MediaSession`.

No direct Streamdev route was found in browser code. Streamdev appears only in architecture/source documentation as a future private provider. This is the correct boundary.

### Remote control

Current main after PR #115 uses:

```text
/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.png
```

The asset is 360×1220 with a transparent background. The existing 35 hotspot geometry remains in `web/frontend/modules/remote.js`.

Implemented behaviour includes:

- one pressed-state visual per key;
- a local busy guard against duplicate dispatch;
- allowlisted `RemoteAction` names;
- explicit backend selection;
- server-side backend availability/capability/read-only checks;
- direct channel switching by VDR channel ID;
- HOME mapping to the VDR menu;
- VDR-Suite logo navigation to overview;
- EPG navigation to the timeline;
- a help and assignment dialog;
- REC logic that inspects live overlay plus timers and may start recording or delete one running timer after confirmation.

Important safety limits:

- `operationId` is currently generated in browser memory and is not the universal durable idempotency contract;
- the REC stop path uses timer deletion and therefore is a real mutation;
- no independent mobile client should copy this workflow until actor authorization, revision/idempotency and outcome semantics are explicit;
- `actionInFlight`, pressed state, hotspot geometry and dialog scroll are UI-only state.

### EPG and channels

Implemented browser capabilities include:

- channel list and current programme enrichment;
- now/next;
- EPG search;
- cached time windows;
- channel-day and timeline views;
- EPG details;
- metadata/artwork detail integration;
- channel movement/sorting.

The current route families include `/api/vdr/channels`, `/api/epg/now-next`, `/api/epg/time-window`, `/api/epg/channel-window`, `/api/epg/cache/window` and `/api/epg/search`.

The public API should converge on stable Channel and ProgramEvent resources rather than expose cache operations or browser-specific time-window variants as independent long-term concepts.

### Recordings

Recordings 2 is the delivered browser owner for:

- lazy folder browsing;
- recording cards;
- detail view;
- metadata, people and artwork;
- Genre navigation;
- rename, move and trash workflows.

Current folder and metadata routes can require `backendNativeId` or path evidence. Those are acceptable internal transition details but not suitable public resource identities. Public clients need an opaque Suite Recording ID, backend scope, revision and media-session link.

No recording playback route is implemented in the Suite client boundary. Resume positions, tracks, marks and cut operations are therefore not a complete current client feature set.

### Search and Genres

Global search is implemented for one selected backend over persistent Recording and EPG titles, subtitles and people:

```text
fetchClientGlobalSearch()
  -> GET /api/search
  -> GlobalSearchApiRuntime
  -> GlobalSearchController
  -> GlobalSearchService
  -> GlobalSearchRepository
```

Genres are exposed through the separate DOM-free wrapper:

- `/api/metadata/genres`;
- `/api/metadata/genres/recordings`;
- `/api/metadata/genres/epg`.

Both are strong read-only candidates for future public resources, after identity, pagination, error and compatibility rules are normalized.

### Timers and SearchTimer

The browser has substantial Timer and SearchTimer foundations, including read, conflict, preview, validation and mutation paths.

They are not yet safe independent-client contracts because:

- aliases and mutation fallback remain;
- stable resource revisions are not universal;
- durable idempotency and operation outcomes are not universal;
- TimerIntent and multi-backend assignment remain Phase 64;
- actor/RBAC/accountability remain Phase 62.

A read-only Android PoC may display current timers and conflicts. It should not create, update or delete timers.

### Mobile state

The browser has mobile media queries, constrained remote width, touch/pointer handling, scroll preservation in selected dialogs, debounce/abort for search and visible loading/error states.

Missing native mobile behaviour includes:

- application lifecycle ownership;
- system back-stack integration;
- native rotation/window state restoration;
- native Picture-in-Picture;
- media controls and audio focus;
- secure device credential storage;
- push token lifecycle;
- Android App Links;
- native downloads;
- structured crash/network diagnostics;
- background synchronization policies.

### PWA state

The current frontend has:

- no web app manifest link;
- no service-worker registration;
- no offline shell;
- no explicit cache strategy;
- no install-prompt handling.

It may still be manually saved by a browser, but it does not currently meet the repository-level definition of an intentional PWA.

## Option comparison

### 1. WebView application

#### Reuse

Very high visual and functional reuse. The Android shell could load the existing frontend and add back-button, certificate, download and deep-link handling.

#### Advantages

- fastest route to a recognisable app;
- existing responsive UI and remote hotspots remain usable;
- one web deployment updates most UI immediately;
- suitable for an internal read-only evaluation package.

#### Risks

- cookies, same-origin sessions and certificate trust become WebView-specific operational concerns;
- JavaScript/native bridges enlarge the attack surface;
- file access and arbitrary navigation must be tightly disabled or allowlisted;
- native media, PiP, MediaSession, downloads and notifications require increasing bridge code;
- lifecycle and rotation state remain web-document concerns;
- TV focus and D-pad behaviour are not solved by responsive CSS;
- debugging spans WebView, JavaScript, HTTP and native bridge layers;
- long-term product value is weak if the app is only a packaged website.

#### Decision

Acceptable only as a labelled transition PoC or diagnostic shell. Not recommended as the production Android or Android TV architecture.

### 2. PWA

#### Reuse

Maximum reuse of the current web code. Required first steps are HTTPS, manifest, icons, service worker and an explicit offline/cache design.

#### Advantages

- installable from supported browsers;
- one web release path;
- no native store release required for every UI change;
- useful for browser-mobile and desktop users;
- an offline shell can preserve navigation, cached metadata and clear unavailable states.

#### Limits

- it cannot create Phase 65 streaming or Phase 62 security by itself;
- browser media and background behaviour vary by browser/device;
- native TV discovery, D-pad focus and ten-foot UI remain separate work;
- native Credential Manager, PiP polish, downloads and system media integration are weaker or browser-dependent;
- cache invalidation is security-sensitive for multi-user and multi-backend data;
- the current frontend is not yet a PWA.

#### Decision

Recommended as a parallel browser enhancement, not as the only Android product.

### 3. Hybrid application

A hybrid application can share selected web surfaces while using native navigation, credentials, notifications and Media3 playback.

#### Advantages

- more reuse than a full native rewrite;
- native media and platform APIs can be added;
- gradual migration of screens is possible;
- one application package can host both native and web destinations.

#### Risks

- every boundary between web and native requires typed messages, lifecycle ownership and error handling;
- two navigation/state systems can drift;
- media, authentication and deep links must not be duplicated across JavaScript and native code;
- framework upgrades become a permanent dependency;
- TV focus navigation remains difficult when web views are embedded;
- test matrices cover web, native and bridge combinations.

#### Decision

Viable as a migration architecture when a business deadline requires early delivery. It is not the preferred long-term endpoint for this project.

### 4. Native Android application

#### Recommended technologies

- Kotlin;
- Jetpack Compose;
- Coroutines and Flow;
- lifecycle-aware ViewModels and unidirectional state flow;
- Android Media3/ExoPlayer for playback;
- an isolated HTTP/JSON client module;
- local database/cache only for client-owned cached representations;
- Android Credential Manager for supported login UX and Android Keystore-backed protection for app-held secrets/keys;
- WorkManager only for deferrable background work;
- Android App Links for verified links;
- notification delivery only after server-side notification resources and user consent exist.

#### Advantages

- strongest media playback, PiP, audio focus, subtitle and track integration;
- correct phone, tablet and TV lifecycle ownership;
- best accessibility, haptics and system back behaviour;
- clear typed client state and error models;
- secure per-installation credential handling;
- testable module boundaries;
- TV UI can share core code without sharing touch UI.

#### Costs

- low direct UI code reuse;
- initial project/build/release setup;
- duplicate implementation of visual components unless design tokens are extracted;
- public API contracts become a hard dependency;
- separate store and signing lifecycle.

#### Decision

Recommended production architecture.

## Weighted decision matrix

Scores use 1 (poor) through 5 (strong). Weights total 100 and deliberately prioritize media, user experience, security and long-term architecture over short-term reuse.

| Criterion | Weight | WebView | PWA | Hybrid | Native |
| --- | ---: | ---: | ---: | ---: | ---: |
| Existing web UI reuse | 6 | 5 | 5 | 4 | 1 |
| Initial development effort | 7 | 5 | 4 | 3 | 1 |
| Long-term maintenance | 7 | 2 | 4 | 3 | 4 |
| Runtime performance | 7 | 2 | 3 | 3 | 5 |
| Media playback and PiP | 13 | 2 | 2 | 4 | 5 |
| Mobile interaction quality | 8 | 2 | 3 | 4 | 5 |
| Android platform integration | 7 | 2 | 2 | 4 | 5 |
| Android TV suitability | 8 | 1 | 2 | 3 | 5 |
| Accessibility | 4 | 2 | 3 | 4 | 5 |
| Offline/lifecycle behaviour | 5 | 2 | 3 | 4 | 5 |
| Security and credentials | 7 | 2 | 3 | 4 | 5 |
| API contract fit | 4 | 2 | 3 | 4 | 5 |
| Testability/diagnostics | 5 | 2 | 3 | 4 | 5 |
| Distribution/release | 3 | 4 | 3 | 4 | 5 |
| Third-party framework risk | 3 | 3 | 5 | 2 | 4 |
| Long-term multi-client fit | 6 | 2 | 3 | 4 | 5 |
| **Weighted result / 5.00** | **100** | **2.40** | **3.04** | **3.65** | **4.38** |

The result does not prohibit a PWA or transition WebView. It identifies the native application as the production target.

## Shared Android and Android TV architecture

### Share

Recommended shared modules:

```text
core-model
  Suite IDs, backend scope, channels, programme events, recordings,
  metadata, people, Genres, operations, errors, capabilities

core-api
  HTTP transport abstraction
  JSON serialization
  authentication/session interceptor
  request/correlation IDs
  revision and idempotency support
  pagination and SSE client

core-auth
  installation identity
  login/pairing flow state
  token renewal and revocation handling
  secure credential storage abstraction

core-cache
  backend-scoped cached reads
  stale/offline markers
  migration and eviction policy

core-sync
  refresh policy
  event cursor/SSE reconciliation
  full-resync rules

core-media
  MediaSession API client
  Media3 player factory/configuration
  track, subtitle, resume and reconnect models

core-design
  VDR-Suite design tokens
  typography, spacing, shapes, icons and semantic colours

core-test
  contract fixtures
  fake API/media servers
  deterministic clocks and operation outcomes
```

### Do not share as one UI

#### Android Mobile

- touch-first navigation;
- compact and adaptive portrait/landscape layouts;
- list-detail on tablets;
- video above controls in portrait and beside controls in expanded landscape;
- gestures only as supplements to visible controls;
- compact remote;
- PiP;
- fast switching among live, EPG, recordings and search;
- haptics for remote acknowledgement, not as proof of backend success.

#### Android TV / Google TV

- D-pad and focus-first navigation;
- visible focused, pressed and disabled states;
- ten-foot typography and large targets;
- no touch assumptions;
- full-screen playback as the primary surface;
- overlay and back navigation designed for a TV remote;
- deterministic focus restoration after dialogs and playback;
- separate evaluation before home-screen recommendations or channel integration.

The two applications may use one Gradle project and shared modules, but should have separate navigation graphs, screen components and acceptance tests.

## Target client and API architecture

```text
Web / Android / Android TV / Desktop / Integrations
  -> client-specific wrapper or SDK
  -> authenticated public /api/v1
  -> request context, authorization and policy
  -> Suite domain services and repositories
  -> backend-scoped read models and orchestration
  -> private Agent / VDR / plugin / provider adapters

Media3 player
  -> create MediaSession through /api/v1
  -> receive short-lived MediaAccessGrant
  -> VDR-Suite Streaming Gateway
  -> private Agent/provider route
  -> media bytes
```

Contract separation remains mandatory:

```text
public HTTP API
  != JavaScript web wrapper
  != Android SDK
  != Backend Agent protocol
  != Media Plane protocol
  != Legacy OSD frame protocol
  != plugin-local schema
```

### HTTP and update mechanisms

Recommended public-client defaults:

- JSON/HTTP for resource reads and command submission;
- conditional requests with ETags where appropriate;
- SSE for one-way sequenced change notifications when the client can reconnect with a cursor;
- polling fallback for constrained clients;
- WebSocket only where a true bidirectional session protocol is required, such as a later OSD plane, not merely because the client is native;
- media bytes through the Media Plane, not normal JSON handlers.

The current SSE feed can inform the design but is not yet a stable authenticated public event subscription.

## Authentication and pairing study

Phase 62 must define actor/session semantics before a final login technology is selected.

Candidate user flows:

### Account login

- user authenticates to one VDR-Suite installation;
- server issues a bounded, revocable session;
- app stores only the minimum required secret material;
- server remains authoritative for roles and backend/resource scopes.

Best for remote access and multiple named users.

### Local pairing

- an authenticated administrator displays a one-time code or QR payload;
- the mobile device submits an enrollment request;
- administrator explicitly approves the device and actor binding;
- the resulting device/session credential is revocable.

Best for household setup without typing long credentials on TV.

### Device-code flow

- TV displays a short code and verification address;
- user authorizes from another signed-in device;
- TV receives a bounded session after approval.

Best fit for Android TV because typing credentials with a D-pad is poor.

### Multiple installations

The client should model installations as explicit connection profiles:

- stable installation ID;
- user-visible label;
- verified origin;
- actor/session state;
- last known API capabilities;
- no shared bearer token across unrelated installations.

### Local network versus remote access

Local reachability is not trust. Both paths require authenticated transport. Self-signed/private PKI support, if offered, needs an explicit enrollment and certificate pin/trust UX; it must not use a global “accept all certificates” switch.

## Mutation safety for applications

Every production mutation must eventually carry or resolve:

- actor identity;
- explicit backend scope;
- stable Suite resource identity;
- backend generation where required;
- current resource revision and `If-Match`;
- one logical `Idempotency-Key`;
- durable operation identity;
- deterministic operation state;
- request and correlation IDs;
- audit/accountability evidence.

A mobile timeout after dispatch is not proof of failure. The client must query the existing operation or wait for reconciliation. It must not retry through another alias or generate a new operation automatically.

The Android SDK should therefore expose mutation results as a sealed state model such as:

```text
Accepted(operation)
Rejected(problem)
Conflict(problem, currentRevision)
InProgress(operation)
Succeeded(result)
FailedVerified(problem)
OutcomeUnknown(operation)
```

It should not return a Boolean for a destructive action.

## Media architecture

### Domain API

Public JSON resources should cover:

- available playback profiles;
- authorization to play a Channel or Recording;
- MediaSession creation and lifecycle;
- selected audio/subtitle tracks;
- resume position and progress updates;
- session revocation/expiry;
- classified capacity and route errors.

### Media Plane

The Streaming Gateway owns:

- byte delivery;
- range and seek;
- reconnect;
- growing recordings;
- pass-through;
- optional governed remux/transcode after pass-through proof;
- grant validation and revocation;
- hiding provider URLs and credentials.

### Current versus future

| Level | Reality |
| --- | --- |
| Current browser | Structured LiveOverlay plus remote actions; no Suite video player. |
| Honest local PoC | May display only the implemented overlay, or use an explicitly non-product private lab adapter that never becomes a public contract. |
| Product architecture | MediaSession plus short-lived grant and Streaming Gateway. |
| Missing runtime | Phase 65 implementation, route admission, provider leases, playback profiles and full media tests. |

A PoC that directly embeds a permanent Streamdev URL may be useful for isolated codec/player experiments, but it must live outside the public client contract, use no production credential model and be removed before product acceptance.

## Proof of concept

### Goal

Validate the native client architecture and read-model suitability without freezing the transition API.

### Included

- configure one local VDR-Suite connection profile;
- explicit compatibility warning and server-origin verification;
- capability discovery through a replaceable PoC adapter;
- backend list and selection;
- backend availability/read-only state;
- channel list;
- now/next;
- EPG detail;
- backend-scoped global search;
- Recording folder/list and metadata detail;
- Genre browsing;
- current LiveOverlay presentation;
- structured offline, timeout and unsupported-capability states;
- phone portrait, phone landscape and one tablet list-detail layout;
- shared model/API modules prepared for a future TV shell.

### Excluded

- remote actions;
- timer or SearchTimer mutation;
- recording rename/move/trash;
- administrative cache refresh;
- production push notifications;
- direct Streamdev URLs;
- recording or live playback unless Phase 65 exists;
- legacy OSD;
- public SDK stability promise.

### PoC adapter rule

```text
UI and domain modules
  -> PoC ClientRepository interface
  -> UnversionedCompatibilityAdapter
```

Only the adapter knows current route names. No route string appears in UI or domain modules. The adapter is deleted or replaced when `/api/v1` is available.

### Acceptance

- no provider/private URL in logs or UI;
- no client-side authorization decision;
- backend scope shown on every backend-owned object;
- rotation and process recreation preserve only safe client state;
- no stale response overwrites newer selection/search state;
- offline cache is marked stale and read-only;
- contract fixtures cover malformed JSON, 401, 403, 404, 409, 429, 5xx and timeout;
- accessibility checks for content descriptions, focus order, text scaling and target sizes;
- no mutating HTTP request in the PoC package.

## Roadmap integration

### Phase 62

Provide the actor/session/RBAC/accountability model required by every production app. Client study outputs should be used as acceptance consumers, not as a reason to change the phase order.

### Phase 63

Secure remote-site access without exposing VDR-internal ports. Android connection profiles must discover only Control Plane origins.

### Phase 64

Introduce TimerIntent and safe multi-backend orchestration before exposing full timer automation in mobile clients.

### Phase 65

Implement MediaSession and Streaming Gateway. This is the gate for product live TV and Recording playback.

### Phase 66

Add a separately permissioned Legacy OSD session. Do not merge OSD frames into LiveOverlay or the ordinary media stream.

### Phase 67

Stabilize `/api/v1`, errors, revisions, pagination, deprecation and compatibility tests. Publish an Android SDK only from this contract.

### Work that can proceed before Phase 67 without violating order

- maintain this study and the capability/API matrices;
- extract shared design tokens and stable icons;
- build API contract fixtures and mock server responses;
- prototype native navigation and adaptive layouts using fakes;
- prototype Media3 with non-production test media;
- build the read-only compatibility-adapter PoC;
- add PWA manifest/offline-shell work as a separate browser enhancement after a cache/security design.

## Final recommendation

1. Treat native Kotlin/Compose as the production Android direction.
2. Share domain, API, auth, cache, synchronization, media and test modules between Mobile and TV.
3. Build distinct touch and D-pad/focus UIs.
4. Keep the current Web frontend as an independent client and improve it toward a PWA where useful.
5. Permit a read-only WebView/TWA or native compatibility PoC only when clearly temporary.
6. Do not expose current aliases as a public API and do not publish an Android SDK before `/api/v1`.
7. Do not claim live video until Phase 65 implements the Streaming Gateway.
8. Keep Phase 62 as the next runtime phase.

## Technical source baseline

Current Android decisions were checked against official Android/Google upstream documentation on 2026-07-27:

- [Android app architecture](https://developer.android.com/topic/architecture)
- [Lifecycle-aware coroutines and Flow](https://developer.android.com/topic/libraries/architecture/coroutines)
- [Jetpack Compose](https://developer.android.com/develop/ui/compose/documentation)
- [Adaptive apps](https://developer.android.com/develop/ui/compose/layouts/adaptive/get-started-with-adaptive-apps)
- [Android TV navigation](https://developer.android.com/training/tv/get-started/navigation)
- [Media3](https://developer.android.com/media/media3)
- [Media3 Android TV](https://developer.android.com/media/media3/ui/androidtv)
- [Picture-in-Picture](https://developer.android.com/develop/ui/views/picture-in-picture)
- [Credential Manager](https://developer.android.com/identity/credential-manager)
- [Android App Links](https://developer.android.com/training/app-links)
- [WebView file and JavaScript security](https://developer.android.com/privacy-and-security/risks/webview-unsafe-file-inclusion)
- [PWA installability](https://web.dev/articles/install-criteria)
- [Trusted Web Activity](https://developer.chrome.com/docs/android/trusted-web-activity)
- [Firebase Cloud Messaging for Android](https://firebase.google.com/docs/cloud-messaging/android/receive-messages)
- [Notification permission](https://developer.android.com/develop/ui/compose/notifications/notification-permission)

Library versions must be selected from stable release channels at implementation time. This study deliberately does not freeze a future Android build to a 2026 point release.
