# VDR-Suite Strict Roadmap

## Purpose

This file owns the strict forward execution order, phase boundaries and phase-completion gates for VDR-Suite.

It does **not** own volatile branch heads, pull-request tips or transient CI state. Canonical operational phase status belongs in [Current State](../CURRENT.md). Exact live repository state must be read from GitHub. Completed evidence belongs in historical closeouts. Compact numbering belongs in the [Phase Map](phase-map.md).

The roadmap translates accepted architecture into an implementation sequence. It does not replace ADRs:

```text
ADR
  -> owns stable architecture and invariants

Roadmap
  -> owns execution order, coherent verticals and completion gates

Current State
  -> owns volatile completed / active / next phase status

Closeout
  -> owns exact historical implementation and acceptance evidence
```

A roadmap entry is never automatic permission to implement the next possible diff. New runtime work requires:

1. a binding product or architecture requirement;
2. an accepted-code gap proven against current `main`;
3. the smallest **coherent** change that closes that gap;
4. the applicable safety, CI, packaging and real-system acceptance gates;
5. an explicit kickoff when a new numbered runtime phase begins.

---

## Execution governance

- A chat discussion becomes a project decision only when represented in the repository through the appropriate ADR, roadmap, current-state or workflow contract.
- A slice is the smallest coherent safety/product change, not the smallest mechanically possible diff.
- Avoid artificial intermediate states and unnecessarily long stacks unless a real safety, concurrency, compatibility, migration or acceptance boundary requires them.
- Technical CI and architecture guards remain mandatory. User-visible milestones additionally use [Golden User Journeys](golden-user-journeys.md).
- Provider reachability never creates authority. An active operation, Timer assignment, media route, broadcast application or compatibility session never silently changes provider.
- Accepted architecture does not equal implemented runtime. A gap closes only through implementation plus the appropriate regression and acceptance evidence.
- Completed phase history is not renumbered. Only not-yet-started future phases may be reordered when architecture/product sequencing is explicitly updated.

---

## Current phase position

```text
Latest completed numbered runtime phase:
Phase 66 - Media Home and Browse Experience

Current active numbered runtime phase:
none - Phase 67 has not started

Next strict numbered runtime phase:
Phase 67 - Broadcast Companion Services: Teletext and HbbTV

Current active runtime slice:
none - Phase 67 has not started
```

Phase 65 and Phase 66 are completed. Durable Media Home evidence lives in [Phase 66 Closeout](../development/phase-66-closeout.md). ADR-0054 defines the next Broadcast Companion architecture, but Phase 67 has not started and still requires a separate explicit runtime kickoff.

Future order: Phase 67 Broadcast Companion -> Phase 68 Legacy OSD -> Phase 69 Public API hardening -> Phase 70 Recommendation / Knowledge Graph.

## Phase 61 - Suite Metadata and Genre Platform

Status: **Completed.**

Established persistent backend-scoped Recording/EPG metadata, people relations, canonical Genre assignments, indexed query-only browse paths and frontend integration.

## Phase 62 — Identity, RBAC and Accountability Foundation

Status: **Completed.**

Established persistent identities, exact backend-scoped authorization, browser-session/CSRF protection and append-only accountability for protected mutations.

Important retained boundary:

- Legacy Basic compatibility remains transitional deployment compatibility.
- Generic account/role/backend administration product surfaces were not required for Phase 62 closeout and remain a cross-cutting product milestone.

## Phase 63 — Backend Agent and Secure Multi-Site Runtime

Status: **Completed.**

Established secure Agent enrollment and identity, backend/Agent generation and lease fencing, observation ingestion, durable command/result handling, fenced native execution, explicit local-provider ownership/selection and the generic protected-write safety contract.

Historical exact foundation marker retained for traceability: `Phase 63 - Backend Agent and Secure Multi-Site Runtime`.

## Phase 64 — Timer Intent and Multi-Backend Orchestration

Status: **Completed.**

Binding architecture: [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md).

Phase 64 established the durable separation:

```text
TimerIntent
  -> TimerAssignment
  -> NativeTimerBinding
```

and completed deterministic scheduling, managed native Timer fulfillment, authoritative readback, reconciliation and controlled reassignment/failover.

### Completed capability boundary

The accepted engine provides:

- durable backend-neutral TimerIntent identity and optimistic concurrency;
- TimerAssignment persistence, primary/replica roles and deterministic eligible-backend selection;
- NativeTimerBinding persistence and canonical observed-state evidence;
- managed native Timer create/update/toggle/delete execution through Control Plane -> Agent -> SuiteBridge;
- durable mutation-operation state, dispatch fencing and no-blind-retry semantics;
- authoritative PRESENT and complete-inventory ABSENCE verification;
- fail-closed handling of stale generations, providers, revisions, assignments, bindings, fingerprints and operation evidence;
- controlled replacement only before native dispatch or after exact verified absence;
- atomic old-owner supersession, replacement creation and durable reassignment evidence;
- exact replay without duplicate exclusive owners;
- real yaVDR acceptance on the exact final candidate.

### Phase-64 completion gate — satisfied

The engine gate required coherent proof of ADR-0044 lifecycle semantics, safe managed native mutation, durable unknown-outcome handling, authoritative readback, reconciliation, controlled reassignment and real-system acceptance. That gate is closed.

### Historical Slice 1-3 traceability anchors

The following strings describe historical Phase-64 intermediate boundaries only. They remain because early guards protect the original slice contracts; they are not current implementation authority:

- `Phase 64 Slice 1 — TimerIntent Domain Contract`
- `Phase 64 Slice 2 — TimerIntent Persistence and Repository Semantics`
- `Phase 64 Slice 3 — TimerAssignment Domain Contract`
- `Status: **Active; Slice 3 is the TimerAssignment domain contract.**`
- `No TimerAssignment persistence; no NativeTimerBinding; no scheduler or failover execution`
- `Account/backend access management is a hard prerequisite before broad Timer UI wiring`

Later accepted Phase-64 work superseded those implementation limitations without rewriting historical slice documents.

---

# Forward numbered runtime roadmap

## Phase 65 — Streaming Gateway and Media Sessions

Status: **Completed.**

### Binding architecture

Primary ADRs:

- [ADR-0046: Streaming Gateway and Media Session Boundary](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053: Client Playback Engine and Media Adaptation Strategy](../adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0055: Media Transcode Backend Selection and Hardware Acceleration](../adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [ADR-0056: Playback Presentation, Timeline, Continuity and Failure Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)

Required foundations:

- ADR-0014 Recording identity;
- ADR-0017 Live transport boundary;
- ADR-0039 Control Plane / Backend Agent boundary;
- ADR-0040 backend lifecycle/generation/lease;
- ADR-0041 authentication and Agent trust;
- ADR-0042 safe mutation/revision/idempotency where media lifecycle changes state;
- ADR-0049 accountability/security events where security-sensitive lifecycle evidence is required.

### Phase goal

Create one authenticated VDR-Suite Media Plane that can deliver Recording and Live-TV media to first-party clients without exposing private backend/provider topology.

The target flow is:

```text
Recording or Live Channel
  -> authenticated/authorized MediaSession request
  -> capability and policy evaluation
  -> MediaRoute + routeEpoch
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> least-transformation media adaptation / internal MediaPresentationProfile
  -> Streaming Gateway
  -> short-lived MediaAccessGrant
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

The server owns authorization, resource identity, route/provider selection, internal delivery/adaptation selection and cleanup policy. The normalized playback contract owns first-party client semantic truth for timeline/capability/continuity/failure without exposing provider or encoder implementation details.

The client owns platform-appropriate decode/render execution. VDR-Suite does not build one universal decoder/rendering engine.

### Hard invariants

- Clients never receive or construct permanent Streamdev, SuiteBridge, filesystem or site-local provider URLs.
- Provider availability does not grant provider authority.
- An active route never silently switches provider.
- `mediaSessionId` is an identifier, not a bearer credential.
- Backend generation, Agent identity, provider identity/generation and route epoch are explicit fences.
- MediaSession identity, route epoch and playback presentation generation are distinct concepts and are not interchangeable.
- Slow or disconnected clients must not block VDR callbacks or retain unbounded receiver/resources.
- VDR locks/pointers never cross media-network or adaptation work.
- Transformation preference is strictly:

```text
pass-through
  -> remux / repackage only when needed
  -> transcode only when materially required
```

- Operation-specific correctness may require stronger transformation only when demonstrated and implemented fail-closed under the existing policy. Exact non-zero HLS video Recording resume is the accepted current example; ordinary start-at-zero remains copy/remux when valid.
- HLS, fragmented MP4 or any other protocol is a negotiated delivery profile, not a universal architecture requirement.
- Timeshift is not silently implied by ordinary Live TV.
- Growing Recordings report truthful readable extent and seek capability; unsupported seek is reported as unsupported rather than fabricated.
- User-owned seek preview, transport-local presentation time and canonical absolute Recording position remain separate state domains.
- Media failures remain classified; no hidden provider/profile switch or unsafe retry is allowed.

### Coherent implementation verticals

#### 65.A — Recording playback vertical — CLOSED

Accepted first product proof:

```text
Recordings 2 detail
  -> authorized MediaSession
  -> explicit MediaRoute / ProviderStreamLease
  -> Gateway
  -> selected least-transformation profile
  -> browser playback adapter
  -> real picture + sound
  -> stop
  -> deterministic cleanup
```

Accepted proof includes provider privacy, least-transformation selection, real picture/sound, deterministic stop/disconnect/revocation cleanup and real yaVDR acceptance. Durable evidence lives in [Phase 65 Recording Playback Closeout](../development/phase-65-recording-playback-closeout-readiness.md).

#### 65.B — Live-TV playback vertical — CLOSED

Accepted product flow:

```text
channel / EPG selection
  -> authorized MediaSession
  -> live provider lease
  -> Gateway
  -> first-party browser playback
  -> picture + sound
  -> channel change
  -> old route / receiver / lease released
```

The accepted hot path uses one continuous FFmpeg consumer on the conditioned SuiteBridge replay, with explicit channel replacement, bounded receiver ownership and real repeated-zap/stability acceptance. Durable evidence lives in [Phase 65 Live-TV Playback Closeout](../development/phase-65-live-tv-closeout.md).

#### 65.C — Recording delivery performance and media output/transcode settings — CLOSED

Phase 65.C was delivered in two successive coherent blocks under the same bounded authorization.

**Recording startup / progressive delivery — PR #206**

- narrow descriptor reuse only while exact completed-source fingerprints remain valid;
- `progressive-direct` for completed native MPEG-TS sources when typed client capabilities include compatible container/codecs and truthful byte ranges;
- `progressive-fmp4` as the low-latency completed-Recording browser path when fMP4 adaptation is needed;
- HLS retained as compatibility fallback rather than a mandatory startup pipeline;
- no `-re` pacing on the completed-Recording continuous fMP4 path;
- no fake `Accept-Ranges`, `Content-Range`, immutable `Content-Length` or browser time-seek semantics on continuous fMP4;
- growing/changed sources fail closed out of completed-only immutable fast paths;
- MediaSession/Gateway authorization, provider privacy and deterministic cleanup remain unchanged.

Durable startup scope is recorded in [Phase 65.C Recording Startup / Progressive Direct](../development/phase-65c-recording-startup-progressive-direct.md).

**Media-transcode backend policy and output settings — PR #208**

- backend-scoped output modes `auto`, `software` and `vaapi`;
- authenticated Web/REST settings with backend scope, CSRF, permissions and accountability;
- session-stable policy resolution for new MediaSessions;
- calibrated Auto selection with a minimum 1.25x real-time threshold;
- measured quality-first x264 preset selection;
- hard execution-host VAAPI capability and exact-transform checks;
- forced VAAPI fail-closed behavior with no silent x264 fallback;
- QSV/NVENC modeled but unavailable/fail-closed and no VDPAU introduction;
- sanitized diagnostics rather than arbitrary FFmpeg arguments or browser-editable DRM paths;
- progressive-fMP4 slow-reader/backpressure hardening;
- deterministic terminal persistence for post-issuance Live policy rejection.

Real yaVDR acceptance covered Auto/forced Software/forced VAAPI Recording and Live paths, settings persistence/restart, active-session stability and fail-closed unsupported Live VAAPI transformation.

Durable output-policy scope is recorded in [Phase 65 Media Transcode Performance / Output Policy](../development/phase-65-media-transcode-performance-policy.md).

The old separate `65.D - Compatibility escalation` planning block is superseded because its demonstrated remux/transcode/hardware-policy work was completed inside 65.C rather than started as a distinct 65.D vertical.

#### Retained seek/growing-recording truthfulness boundary

The earlier `65.C - Recording seek and growing-recording semantics` heading is obsolete, but its safety intent remains mandatory across Phase 65:

- advertise range/seek only when truly supported by the selected source/profile;
- model completed versus growing source state explicitly wherever it changes capability;
- expose current readable extent truthfully when implemented;
- never treat a still-growing source as immutable merely to enable a fast path;
- normalize public media/track identity independently of provider-native paths/PIDs where required.

Phase 65.D.2 and accepted follow-ups provide arbitrary time-seek and stop/resume semantics for supported completed-Recording progressive-fMP4 and HLS restart-seek paths. Compatibility timeline interactions commit canonical absolute Recording positions rather than transport-local guesses. Exact non-zero HLS video resume provides a synchronized random-access start through the implemented adaptation path or fails closed. User-visible growing-Recording seek and broader VDR-index mapping beyond what those accepted paths require remain deferred until a demonstrated product gap justifies a coherent implementation block. Truthful non-support satisfies the safety contract; fabricated support does not.

#### 65.D — Client playback abstraction — CLOSED

Provide a small semantic first-party abstraction around platform playback engines:

```text
open session
play / pause / stop
seek where supported
select audio/subtitle track
read canonical position/state
observe continuity/discontinuity
report classified failure
close
```

Browser is the initial product-validation client. Android/Android TV, Kodi, desktop and television adapters reuse Suite MediaSession semantics but keep their mature platform-native player engines.

The abstraction must consume typed Suite media capabilities and normalized MediaSession playback semantics. It must not introduce browser-brand/user-agent routing, bypass ADR-0055 output policy, expose provider-native URLs or vendor one universal Suite decoder core.

**65.D.1 — Persistent Browser Playback Shell — CLOSED**

- one persistent browser playback owner across first-party internal navigation;
- the same HTML media element moves between the active presentation owner and the persistent shell;
- native browser/Android Picture-in-Picture remains tied to the same element/session;
- no second MediaSession or universal player core is introduced.

Durable evidence: [Phase 65.D.1 Persistent Browser Playback Shell Closeout](../development/phase-65d1-persistent-browser-playback-shell-closeout.md).

**65.D.2 — Recording Playback Controls and Seek — CLOSED**

- Play/Pause/Stop and truthful completed-Recording position/duration;
- relative, timeline and direct absolute seek;
- progressive-fMP4 MediaSession worker repositioning without fake HTTP byte ranges;
- HLS/transcoding restart-seek through a fresh authorized MediaSession with `startPositionSeconds`;
- stop/resume versus start-from-beginning choice;
- Android-friendly direct-time entry;
- real yaVDR/browser acceptance for the supported progressive-fMP4 and HLS fallback paths.

Durable evidence: [Phase 65.D.2 Recording Playback Controls and Seek Closeout](../development/phase-65d2-recording-playback-controls-seek-closeout.md).

**Normalized Recording audio/subtitle selection — CLOSED**

- normalized public audio/subtitle track identities without provider/PID leakage;
- explicit Recording audio-track selection integrated with the existing progressive/HLS owner lifecycle;
- browser-selectable Recording SRT sidecars delivered as WebVTT with normalized `subtitle-N` / `off` semantics;
- unsupported DVB bitmap/Teletext subtitle formats remain truthfully unavailable to the browser selector;
- real yaVDR/browser acceptance completed for the supported audio and SRT paths.

Operational exact PR/CI/acceptance evidence is maintained in [Current State](../CURRENT.md).

**Browser-local Volume/Mute — CLOSED**

- one Suite-owned 0–100 Volume/Mute control layer shared by Recording and Live playback owners;
- active `HTMLMediaElement.volume` / `.muted` remain the source of truth;
- state survives replaceable Recording transport ownership changes and persistent presentation reparenting without creating another media element;
- no Suite MediaSession mutation, server volume API, VDR system-volume command, browser-brand routing or provider/PID exposure;
- platform write limitations fail safely by reading back the actual media-element state;
- real browser/yaVDR acceptance passed for the bounded slice.

Durable evidence: [Phase 65.D Browser-local Volume/Mute Closeout](../development/phase-65d-browser-volume-mute-closeout.md).

**Continuous-fMP4 browser MSE forward-buffer/backpressure — CLOSED**

PR #219 closed the demonstrated progressive-fMP4 browser `SourceBuffer is full` defect. The continuous browser MSE pump now uses a bounded 12-second forward high-water mark, stops pulling while the buffer is full enough, resumes after playback/seeking creates room and releases the pending wait on transport destruction.

This remains one MediaSession, one Gateway stream and one HTML media owner. It is distinct from the earlier Phase-65.C HTTP slow-reader/backpressure boundary.

Durable exact candidate/CI/real-system evidence is maintained in [Current State](../CURRENT.md).

**Compatibility timeline ownership + exact HLS Recording resume — CLOSED**

PR #220/#221 established two separate accepted boundaries:

1. user-owned HLS fallback timeline drag preview is not overwritten by active playback `timeupdate` before commit/cancel;
2. exact non-zero HLS video Recording resume must provide synchronized random-access startup. Ordinary start-at-zero retains least-transformation copy/remux; exact non-zero HLS video resume promotes supported copied H.264/AAC A/V to the implemented transcode path under ADR-0055 policy or fails closed, with a lower worker-plan guard rejecting unsafe copied A/V.

The durable timeline model is:

```text
canonical absolute Recording position
  = presentation base position + transport-local position
```

UI preview position remains separately user-owned during an active seek interaction.

Durable exact candidate/CI/merge evidence is maintained in [Current State](../CURRENT.md).

**ADR-0056 Playback semantic consolidation — CLOSED**

ADR-0056 introduces no new player or MediaSession owner. It normalizes the semantic layer above internal `MediaPresentationProfile` execution detail:

```text
internal MediaPresentationProfile
  -> MediaSession / provider / route ownership
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

The bounded implementation sequence is:

```text
1. normalized MediaPlaybackContract
2. canonical playback-owner lifecycle snapshot/subscription
3. timeline + continuity/discontinuity semantics
4. classified playback failure semantics
5. read-only media diagnostics after semantic correctness
```

Mandatory Phase-65.D work was 1-4 and is completed. Read-only diagnostics are sequenced afterward and remain observational only. Shared fMP4/MSE helper extraction is separate technical debt and is not a Phase-65.D completion gate.

Binding implementation contract: [Phase 65.D Playback Semantics Consolidation](../development/phase-65d-playback-semantics-consolidation.md).

Binding production-owner/testing rules: [Frontend Playback Integration Contract](../development/frontend-playback-integration-contract.md).

### Phase-65 acceptance gate

Phase 65 closes only when all required supported paths prove:

1. real Recording picture + sound through Suite MediaSession/Gateway contracts;
2. real Live-TV picture + sound through the same public media architecture;
3. deterministic stop/disconnect/revocation cleanup;
4. no public provider URL/credential leakage;
5. explicit route/provider/generation fencing;
6. pass-through chosen when valid;
7. remux/transcode not selected unnecessarily, except operation-specific stronger adaptation proven necessary for correctness such as exact non-zero HLS resume;
8. truthful range/seek/growing-recording capability advertisement for implemented Recording playback, including accepted completed-Recording seek and explicit non-support where growing/other advanced seek is not implemented;
9. normalized `MediaPlaybackContract` semantics are implemented for supported first-party playback paths;
10. decoder-significant continuity/discontinuity is explicit and does not conflate MediaSession identity, route epoch or presentation generation;
11. browser/client classified failure behavior preserves detailed reasons and does not trigger silent provider/profile/session recovery;
12. real yaVDR acceptance of the first native live source and Recording source plus every later runtime-sensitive supported-path change required by the active semantic slice;
13. Golden User Journeys 1, 2 and the media portion of Journey 5 pass for the implemented scope;
14. complete repository CI, packaging/install regression and rollback documentation pass on the exact accepted candidate.

### Explicitly deferred / not required to close Phase 65

- broad polished Timer UI;
- Legacy OSD compatibility;
- Teletext/HbbTV application-domain runtime;
- public third-party `/api/v1` stabilization;
- universal transcoding support beyond demonstrated supported workloads/backends;
- user-visible growing-Recording seek when unsupported capability is reported truthfully;
- broader VDR-index time mapping beyond the accepted completed-Recording seek paths when not required by a supported profile;
- global timeshift architecture unless explicitly promoted by a later accepted contract;
- extraction/vendorization of Kodi VideoPlayer;
- one universal Suite-owned decoder/rendering core;
- shared fMP4/MSE helper deduplication;
- read-only media-pipeline diagnostics beyond what is required to prove semantic correctness.

Phase 65 is completed. Phase 65.A through 65.D are closed for their bounded accepted scopes. See [Phase 65 Closeout](../development/phase-65-closeout.md). Phase 66 is also completed; see [Phase 66 Closeout](../development/phase-66-closeout.md).

---

## Phase 66 — Media Home and Browse Experience

Status: **Completed.**

Binding architecture: [ADR-0058: Media Home, Responsive Browse and Preview Experience](../adr/ADR-0058-media-home-responsive-browse-preview.md).

Binding implementation contract: [Phase 66 Media Home and Browse Experience](../development/phase-66-media-home-browse-experience.md).

Durable completion evidence: [Phase 66 Closeout](../development/phase-66-closeout.md).

### Phase goal

Turn completed Phase-65 media and existing Channel/EPG/Recording/Metadata domains into one responsive first-party landing/browse experience without creating a second source of truth or playback lifecycle.

Core rule: **Browse first, playback second.** Browse focus updates immediately; optional Live preview is deferred, cancelable and attached only through the existing Phase-65 MediaSession / canonical playback owner.

### Coherent implementation sequence

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

Slices 66.1 through 66.8 are completed for their accepted bounded scopes.

### Hard invariants

- Home projects existing Channel, ProgramEvent, Recording, Metadata, Genre and artwork truth.
- No Home-specific media identity, metadata database, MediaSession owner, restart path or cleanup engine.
- Browse focus, preview intent and explicit `Watch Live` remain distinct.
- Rapid focus movement creates no preview session; stale preview startup cannot attach after focus moves.
- Desktop/tablet/mobile use responsive recomposition, not a scaled desktop page.
- Continue Watching requires truthful resume evidence; Recently Watched is separate history semantics.
- Browser-local state is not silently promoted to cross-client truth.
- Teletext/HbbTV, Legacy OSD, public API hardening, recommendations, native apps and Live timeshift are outside Phase 66.

### Phase-66 acceptance gate — satisfied

Phase 66 required Home to be the accepted first-party landing experience, responsive desktop/mobile browse without waiting for preview, deferred preview through canonical Phase-65 ownership/cleanup, truthful Continue Watching and Recording rails, explicit history semantics, real supported-environment Golden Home journeys, and exact final CI/packaging/rollback gates. Those gates passed before PR #264 merged the closeout.

---

## Phase 67 — Broadcast Companion Services: Teletext and HbbTV

Status: **Next; not started.**

Binding architecture: [ADR-0054: Broadcast Companion Services — Teletext and HbbTV](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md).

ADR-0054 establishes the domain-first Teletext/HbbTV boundary and the future sequencing. Runtime implementation follows completed Phase 66 Media Home and remains blocked until Phase 67 is explicitly started.

### Why this phase exists before Legacy OSD

Teletext and HbbTV are normal television product capabilities, not merely legacy OSD screens.

- Teletext data may currently be rendered by `vdr-plugin-osdteletext`, but VDR-Suite should model the underlying Teletext service/page/subpage data rather than make the VDR OSD the product contract.
- HbbTV is a broadcast-associated application lifecycle using application discovery plus a browser/application runtime. It must not be reduced to an OSD snapshot/control tunnel.

This follows ADR-0030's domain-first rule: model the underlying capability first; reserve Legacy OSD for functions that genuinely remain opaque native compatibility surfaces.

### Phase goal

Provide backend-neutral broadcast companion services associated with a Live Channel/ProgramEvent:

```text
Live Channel / ProgramEvent
  +--> Teletext service/pages/subpages
  +--> HbbTV application discovery/session
```

Both reuse Phase-62/63 identity, authorization, Agent and generation boundaries and Phase-65 media semantics where video playback is required.

### 67.A — Teletext data plane

Target domain concepts:

```text
TeletextServiceRef
TeletextPageRef
TeletextPage
TeletextSubpage
TeletextSnapshot / revision
TeletextCapability
```

Required rules:

- extract/normalize Teletext data behind an explicit local provider capability;
- do not expose `osdteletext` cache files or internal plugin paths as the public model;
- page and subpage identity are backend/channel/service scoped;
- page freshness/revision is explicit;
- color/control/text rendering information is preserved through a versioned normalized contract where required;
- page caches are bounded and provider-owned internals remain private;
- no VDR OSD frame is required for normal Teletext viewing;
- multi-backend/channel identity is explicit;
- Teletext navigation is a client/domain action, not raw remote-key replay.

Initial product flow:

```text
Live TV
  -> Teletext available
  -> open Teletext
  -> page 100 / number entry
  -> page/subpage navigation
  -> color-key navigation where represented
  -> close back to Live TV
```

Teletext subtitles may reuse decoded Teletext data but require explicit subtitle/timing semantics before being advertised as a media subtitle track.

### 67.B — HbbTV application discovery

Target domain concepts:

```text
BroadcastApplicationRef
BroadcastApplicationDescriptor
BroadcastApplicationCapability
BroadcastApplicationSession
BroadcastApplicationRoute
```

Discovery may use VDR-local AIT/DSM-CC or another proven local provider, but provider-specific parser/cache/process details remain private.

Required rules:

- application identity is tied to backend generation plus broadcast service/application identity, not only a URL;
- discovery provenance and freshness are retained;
- red-button/autostart semantics are explicit facts, not hidden UI heuristics;
- browser/application launch never exposes arbitrary local plugin command channels;
- raw `URL`, `JS`, `KEY`, `ATTACH`, `DETACH` style plugin control is not a public VDR-Suite API;
- HbbTV network access executes in an explicitly bounded application/browser runtime;
- origin/navigation/security policy is explicit;
- app lifecycle and media playback remain separately identifiable.

### 67.C — HbbTV client/application runtime

Initial first-party direction:

```text
Live Channel
  -> discovered BroadcastApplication
  -> authorized BroadcastApplicationSession
  -> sandboxed HbbTV-capable browser/application adapter
  -> normalized remote/color-key input
  -> Phase-65 MediaSession when Suite media is required
  -> deterministic close/cleanup
```

The HbbTV application runtime is not the general VDR-Suite Web frontend and not the Legacy OSD renderer.

A client may need HbbTV compatibility/polyfill facilities, but the public Suite contract remains application/session oriented rather than exposing one implementation library.

### Phase-67 safety invariants

- no arbitrary broadcaster/plugin URL or JavaScript execution endpoint becomes a general public API;
- no direct browser access to local VDR/plugin ports;
- backend generation and application-discovery revision are fenced;
- application network/browser execution is isolated from VDR locks/callbacks;
- application media requests do not bypass MediaSession authorization where Suite media resources are involved;
- normalized remote input is bounded and application-scoped;
- closing/changing channel invalidates stale application sessions;
- Teletext and HbbTV remain distinct capabilities even when one client presents them together.

### Phase-67 acceptance gate

Phase 67 closes only when:

1. one real Teletext-capable broadcast service can be discovered and browsed through Suite domain contracts without OSD proxying;
2. page/subpage navigation and bounded caching behave deterministically;
3. one real HbbTV application can be discovered from broadcast signaling through a backend-local provider;
4. the first supported HbbTV client/runtime can launch and close that application through a Suite-owned session boundary;
5. no raw plugin/browser command API is exposed publicly;
6. channel change/backend restart invalidates stale Teletext/HbbTV context correctly;
7. required Phase-65 media integration is preserved rather than bypassed;
8. representative real yaVDR/broadcast acceptance and rollback pass;
9. Golden Teletext and HbbTV user journeys pass for the supported deployment profile.

### Explicitly deferred / not required to close Phase 67

- pixel-perfect support for every broadcaster/HbbTV profile;
- unrestricted arbitrary-web browsing;
- using Legacy OSD as the primary Teletext/HbbTV surface;
- universal DSM-CC or browser implementation mandated as public contract;
- every historical `osdteletext` display option;
- every proprietary HbbTV extension.

---

---

## Phase 68 — Legacy OSD Compatibility Bridge

Status: **Planned after Phase 67.**

Binding architecture: [ADR-0047: Legacy OSD Compatibility Bridge](../adr/ADR-0047-legacy-osd-compatibility-bridge.md).

Planning note: the accepted ADR-0047 architecture remains authoritative. This roadmap intentionally places its not-yet-started runtime after the Broadcast Companion phase. Accepted ADR-0054 supersedes only the older future-phase numbering statement; it does not weaken ADR-0047's architecture or safety constraints.

### Phase goal

Provide a bounded compatibility route for native VDR/plugin functions that still cannot be represented as normal Suite domains.

```text
Client
  -> authenticated Suite API
  -> LegacyOsdSession
  -> viewer binding
  -> sequenced immutable OSD frame/delta
  -> optional exclusive controller lease
  -> allowlisted native input
```

The bridge remains visibly legacy/compatibility functionality and never becomes the primary VDR-Suite application model.

### Coherent implementation verticals

#### 68.A — Read-only OSD observation

- OsdSurfaceRef identity and OSD epoch;
- immutable full-frame contract;
- bounded native copying;
- sequence/freshness metadata;
- no input.

#### 68.B — Sequence, delta and resynchronization

- full-frame authority;
- deltas only against exact base sequence;
- gap detection;
- `resync_required` instead of guessed state;
- bounded queues/backpressure.

#### 68.C — Agent transport and viewer sessions

- authenticated Agent path;
- `osd.view` authorization;
- multiple bounded viewers;
- backend-generation fencing;
- privacy/stale-state handling.

#### 68.D — Controller lease

- separate `osd.control` permission;
- exactly one active Suite controller per native surface scope;
- lease epoch, expiry and revocation;
- read-only backend denial;
- no native input yet until the lease boundary passes.

#### 68.E — Allowlisted input

- normalized safe key vocabulary;
- generation/OSD-epoch/lease fencing;
- bounded repeat/rate/deadline;
- no raw SVDRP, shell, plugin-service or arbitrary key-code tunnel;
- no delayed replay after Agent disconnect.

### Phase-68 acceptance gate

- domain-first product surfaces remain primary;
- view and control permissions are independent;
- read-only backends cannot obtain OSD control;
- sequence loss is detectable and recoverable from a full frame;
- stale generation/OSD epoch/controller lease commands fail closed;
- several viewers can observe within limits;
- exactly one Suite controller lease exists per surface scope;
- no arbitrary command tunnel exists;
- VDR locks/callbacks remain bounded and non-blocking;
- sensitive frame payloads do not enter normal logs/audit;
- local and multi-site real VDR acceptance passes;
- direct legacy endpoint migration/rollback is documented.

---

---

## Phase 69 — Public API and Client Compatibility Hardening

Status: **Planned after Phase 68.**

Binding architecture: [ADR-0048: Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-and-compatibility-contract.md).

Planning note: ADR-0048 architecture remains accepted. The strict roadmap now places its not-yet-started runtime after Streaming, Broadcast Companion and Legacy OSD so the stable public API can describe mature implementations instead of prematurely freezing internal transition shapes.

### Phase goal

Stabilize an independent-client platform contract below:

```text
/api/v1
```

without confusing it with:

- Agent protocol versions;
- Media Plane connections;
- Legacy OSD frame/input transport;
- plugin-local schemas;
- internal C++ services.

### Coherent implementation verticals

#### 69.A — Public resource and route inventory

- classify existing routes as public v1, internal/transition, deprecated alias or private;
- define stable Suite identities for every public representation;
- prevent backend/provider implementation details from leaking into public contracts.

#### 69.B — Common request/response metadata and errors

- request/correlation IDs;
- stable problem/error codes;
- correct HTTP status mapping;
- retry/deprecation metadata;
- no machine logic based on human error strings.

#### 69.C — Revision/precondition/idempotency exposure

- resource-specific revisions;
- ETag/conditional requests where appropriate;
- explicit idempotency for mutation submission;
- no unsafe client fallback after ambiguous mutation errors.

#### 69.D — Collections, pagination and partial results

- stable ordering;
- pagination/cursors;
- bounded limits;
- multi-backend partial-result semantics;
- source failure is explicit rather than silently omitted.

#### 69.E — Compatibility and deprecation policy

- additive versus breaking schema rules;
- versioned capability negotiation;
- alias retirement policy;
- deprecation headers/metadata;
- compatibility matrix and contract tests.

#### 69.F — First-party and third-party client hardening

- common client error representation;
- no fallback probing after arbitrary errors;
- client wrappers consume stable v1 contracts;
- browser, TV, mobile, desktop and Kodi integrations receive a documented stable boundary for supported domains.

### Phase-69 acceptance gate

- `/api/v1` exists for the declared stable domain set;
- stable error and HTTP semantics are verified;
- revisions/preconditions/idempotency behave correctly across restart and concurrency cases;
- collection/pagination/partial-result contracts are deterministic;
- deprecation policy is testable;
- clients do not depend on private Agent/provider/plugin shapes;
- schema/compatibility tests prevent accidental breaking changes;
- migration from supported aliases is documented and rollback-safe;
- media/OSD/broadcast data planes remain separately versioned where appropriate.

---

---

## Phase 70 — Recommendation and Content Knowledge Graph

Status: **Later vision; no runtime authorization.**

A dedicated accepted ADR is required before implementation.

### Prerequisites

- stable Recording/ProgramEvent/MetadataEntity identity and provenance;
- mature people/genre/metadata graph foundations;
- actor privacy/preferences and authorization;
- accountability boundaries;
- stable public resource semantics from Phase 69;
- explicit correction and explainability behavior.

### Direction

Potential domain flow:

```text
stable content identities
  -> provenance-aware facts and relations
  -> actor-scoped preferences/history
  -> deterministic baseline ranking
  -> explainable recommendation
  -> optional provider-neutral AI enrichment
  -> user correction / feedback
```

### Hard boundary

Recommendation logic never becomes hidden authority for:

- Timer creation;
- metadata relationship changes;
- access policy;
- provider selection;
- destructive Recording actions.

Any later automation using recommendation evidence must enter the owning domain contract explicitly.

---

# Cross-cutting product milestones

These milestones are intentionally **not inserted as numbered runtime phases**. They may progress when their prerequisites are satisfied without blocking unrelated numbered work.

## Milestone A — Account and Backend Access Administration

Status: **Planned; prerequisite for broad Timer mutation UI.**

Phase 62 established the underlying identity/RBAC model but intentionally deferred generic administration product surfaces.

Required product capability:

- list/manage users or supported actor identities;
- inspect backend-scoped access grants;
- grant/revoke supported backend permissions according to policy;
- Admin and Read-only semantics remain fixed and server-enforced;
- credential/session management exposes only safe administrative metadata;
- CSRF, accountability and backend scope remain mandatory;
- no secret material is returned after issuance where the credential contract forbids it;
- operator recovery/migration is documented.

This milestone may proceed alongside numbered runtime work when implemented as a coherent security/admin product slice.

## Milestone B — Broad Timer Product UI

Status: **Planned; Phase-64 engine complete, UI still gated on access administration.**

Prerequisites:

```text
Phase 62 identity/RBAC foundation [DONE]
+ Phase 64 Timer engine [DONE]
+ required account/backend access administration [OPEN]
```

The broad Timer UI must be intent-first, not a return to native VDR Timer ownership.

### Product surface

- TimerIntent list and detail;
- create one-off TimerIntent from EPG and explicit manual form;
- edit desired schedule/recording options through intent revision semantics;
- enable/disable/cancel/delete according to the TimerIntent lifecycle contract;
- show current primary assignment and deliberate replicas;
- explain selected backend and relevant eligibility/conflict evidence in user-appropriate form;
- show fulfillment state separately from intent state;
- display reconciliation/failover state without collapsing `outcome_unknown` into generic failure;
- expose read-only/permission denial clearly;
- preserve backend-neutral default behavior while allowing only policy-level backend preferences that the Timer model explicitly supports;
- provide advanced diagnostics for NativeTimerBinding/operation evidence only to appropriately authorized users.

### UI safety rules

- browser code never directly calls private SuiteBridge/SVDRP Timer commands;
- UI optimistic updates never invent verified native success;
- stale revisions produce conflict/reload behavior rather than overwrite;
- an ambiguous dispatch does not present a safe Retry button that can duplicate mutation;
- replicas are explicit user/policy intent, not duplicate detection heuristics;
- failover status reflects actual durable handover evidence.

### Product acceptance

The user-facing portion of Golden Journey 3 must pass from real EPG interaction through TimerIntent, assignment, native readback and visible final state. Permission/read-only/conflict and at least one reconciliation/failover presentation path must also be demonstrated.

The milestone is independent of the current numbered runtime status and does not silently advance it.

## Milestone C — Audit, Security and Operations Product Surfaces

Status: **Deferred product layer over completed accountability foundation.**

Phase 62/ADR-0049 established accountability foundations. Remaining product work may include:

- protected audit reader;
- filtering and backend/actor/resource correlation;
- redaction and retention policy;
- export/integration;
- security-event presentation;
- operation/job/reconciliation diagnostics.

Do not reopen Phase 62 to implement these surfaces.

## Milestone D — Legacy Basic Retirement

Status: **Deferred deployment migration.**

Retire transitional Legacy Basic compatibility only after:

- packaged defaults and operator configuration use the enforced identity model;
- recovery/admin paths are proven;
- upgrade/migration documentation exists;
- real deployment rollback is tested.

This is a deployment compatibility milestone, not a prerequisite for Streaming unless a concrete security requirement later makes it one.

## Milestone E — First-party client family rollout

Status: **Progressive.**

- Browser is the first Phase-65 playback validator.
- Browser/TV surfaces should reuse the same Suite media and broadcast companion semantics.
- Android/Android TV should use a mature platform engine such as Media3/ExoPlayer behind the Suite playback abstraction.
- Kodi integration should obtain authorized Suite resources and delegate playback to Kodi's own player; Kodi VideoPlayer is not vendored as the Suite player core.
- Desktop/Apple/native clients select mature platform-appropriate engines.
- Independent/third-party client compatibility becomes a formal Phase-69 contract.

---

# Cross-cutting completion gates

Every numbered phase and milestone applies the relevant subset of these gates.

## Identity and ownership

- stable Suite identity exists;
- native/provider identity is an explicit binding, never the Suite resource itself;
- actor/backend/site/provider identities remain distinct.

## Authorization and policy

- authentication does not imply permission;
- exact backend/resource scope is enforced server-side;
- read-only policy remains independent and authoritative;
- UI visibility is never the enforcement boundary.

## Provider and generation fencing

- provider facts carry provenance and version/generation evidence;
- reachability does not grant authority;
- active work never silently switches provider;
- stale backend/Agent/provider generation fails closed.

## Mutation and side effects

Where side effects exist:

- expected revision/preconditions;
- idempotency scope;
- durable pre-dispatch/starting evidence where required;
- exact dispatch boundary;
- no blind retry after possible dispatch;
- authoritative readback/verification;
- bounded reconciliation;
- accountability evidence.

## Native VDR boundary

- no raw VDR pointer, lock guard or native iterator crosses async/network/database work;
- callbacks remain bounded;
- expensive serialization, media processing and client waits occur outside VDR locks;
- shutdown/rollback removes callbacks, listeners, receivers, leases and temporary resources deterministically.

## Client boundary

- clients consume Suite-owned resources/sessions;
- provider URLs, credentials, local paths and plugin command channels remain private;
- client capabilities are negotiation facts, not authority to select private providers;
- first-party playback consumes normalized ADR-0056 semantics rather than deriving new capability only from provider/profile/DOM implementation details.

## Acceptance

As applicable:

```text
domain/value tests
  -> repository/migration tests
  -> service/controller tests
  -> architecture/static guards
  -> frontend/client contract tests
  -> aggregate regression
  -> production build
  -> packaging/install validation
  -> real yaVDR/native acceptance
  -> Golden User Journey acceptance
  -> rollback verification
```

A user-visible milestone is not complete from component CI alone.

---

# Revised forward sequence

```text
Phase 64 - Timer Intent and Multi-Backend Orchestration [COMPLETED]
  -> Phase 65 - Streaming Gateway and Media Sessions [COMPLETED]
  -> Phase 66 - Media Home and Browse Experience [COMPLETED]
  -> Phase 67 - Broadcast Companion Services: Teletext and HbbTV [NEXT; NOT STARTED]
  -> Phase 68 - Legacy OSD Compatibility Bridge
  -> Phase 69 - Public API and Client Compatibility Hardening
  -> Phase 70 - Recommendation and Content Knowledge Graph
```

Cross-cutting, non-numbered product milestones:

```text
Account / Backend Access Administration
  -> enables Broad Timer Product UI

Audit / Security / Operations product surfaces
Legacy Basic retirement
First-party client family rollout
```

This ordering intentionally places Teletext/HbbTV **before** Legacy OSD because they are ordinary television-domain capabilities and should be modeled domain-first. Legacy OSD remains the compatibility fallback for functions that still lack a proper Suite domain.

---

## Next authorization boundary

Phase 65 and Phase 66 are completed for their accepted bounded scopes. Phase-66 durable completion evidence is in [Phase 66 Closeout](../development/phase-66-closeout.md).

**Phase 67 - Broadcast Companion Services: Teletext and HbbTV is next but has not started.** ADR-0054 is accepted architecture only. Runtime implementation still requires a separate explicit Phase-67 kickoff.

Before any Phase-67 runtime block, read live `main`, `CURRENT.md`, ADR-0054, the strict Phase-67 roadmap section and the existing Channel/EPG/Agent/media ownership paths, then choose the smallest coherent domain-first change without crossing into Legacy OSD or public-API work.

Completed-Recording arbitrary time-seek and stop/resume are accepted for the supported progressive-fMP4 and HLS restart-seek profiles. Growing-Recording seek, Live-TV timeshift and broader VDR-index mapping not required by those accepted paths remain deferred and must stay explicit/fail-safe until separately justified.

Bounded post-Phase-66 performance/correctness hardening does not reopen Phase 66 and does not authorize Phase 67.

---

## Related documents

- [Current State](../CURRENT.md)
- [Phase Map](phase-map.md)
- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [Implementation Dependency Map](implementation-dependency-map.md)
- [Golden User Journeys](golden-user-journeys.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Phase 64 Closeout](../development/phase-64-closeout.md)
- [Phase 65 Closeout](../development/phase-65-closeout.md)
- [Phase 65 Recording Playback Closeout](../development/phase-65-recording-playback-closeout-readiness.md)
- [Phase 65 Live-TV Playback Closeout](../development/phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](../development/phase-65c-recording-startup-progressive-direct.md)
- [Phase 65 Media Transcode Performance / Output Policy](../development/phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](../development/phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](../development/phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 65.D Browser-local Volume/Mute Closeout](../development/phase-65d-browser-volume-mute-closeout.md)
- [Phase 65.D Playback Semantics Consolidation](../development/phase-65d-playback-semantics-consolidation.md)
- [Phase 66 Media Home and Browse Experience](../development/phase-66-media-home-browse-experience.md)
- [Phase 66 Closeout](../development/phase-66-closeout.md)
- [Post-Phase-66 Home Performance Hardening](../development/post-phase-66-home-performance-hardening.md)
- [Frontend Playback Integration Contract](../development/frontend-playback-integration-contract.md)
- [ADR-0030 Domain-First UI](../adr/ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0044 Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0046 Streaming Gateway](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053 Playback/Adaptation](../adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0054 Broadcast Companion Services](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0055 Media Transcode Backend Selection](../adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [ADR-0058 Media Home, Responsive Browse and Preview Experience](../adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR-0047 Legacy OSD](../adr/ADR-0047-legacy-osd-compatibility-bridge.md)
- [ADR-0048 Public API](../adr/ADR-0048-public-api-versioning-error-and-compatibility-contract.md)
- [ADR-0049 Audit/Security](../adr/ADR-0049-audit-security-event-model.md)
