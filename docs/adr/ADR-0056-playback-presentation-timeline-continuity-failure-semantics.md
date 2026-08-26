# ADR-0056: Playback Presentation, Timeline, Continuity and Failure Semantics

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053: Client Playback Engine and Media Adaptation Strategy](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0055: Media Transcode Backend Selection and Hardware Acceleration Policy](ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [Frontend Playback Integration Contract](../development/frontend-playback-integration-contract.md)
- [Phase 65.D Playback Semantics Consolidation Contract](../development/phase-65d-playback-semantics-consolidation.md)

---

## Status

**Accepted**

Date: 2026-08-26

Accepted during active Phase 65.D after the persistent browser playback owner, completed-Recording seek/restart semantics, normalized track selection, browser-local Volume/Mute, continuous-fMP4 forward-buffer control and the exact HLS Recording resume follow-up had exposed the remaining semantic coupling between transport details and client behavior.

This ADR consolidates those demonstrated boundaries. It does **not** start Phase 66, add Live-TV timeshift, broaden growing-Recording seek, create a universal decoder/player core or authorize unrelated runtime work.

---

## Context

ADR-0046 defines the server-side MediaSession/Gateway/provider boundary. ADR-0053 defines the client playback and least-transformation adaptation direction. ADR-0055 separates media adaptation from encoder-backend policy.

Phase 65.D implementation has now proved that those layers need one additional stable semantic boundary above the internal presentation/worker plan.

The current internal `MediaPresentationProfile` is intentionally rich in execution detail. It can describe:

- protocol and container;
- adaptation class;
- selected source streams;
- copy/transcode/omit decisions;
- target codecs, geometry and audio channels;
- deinterlacing/workload class;
- encoder backend/preset/hardware-device policy.

That is appropriate for server-side media planning. It is not an appropriate public client semantic contract.

At the same time, the browser implementation has accumulated legitimate but distributed decisions based on combinations of:

- `presentationProfileId`;
- completed versus growing Recording state;
- indexed timeline availability;
- progressive-fMP4 versus HLS transport;
- restart versus in-session reposition;
- active owner/session identity;
- track-selection readiness;
- browser-local media-element state.

Real Phase-65.D runtime work demonstrated why those distinctions must become explicit rather than remain implicit conventions.

### Continuous-fMP4 forward buffering

PR #219 closed a browser MSE forward-buffer/backpressure defect without changing MediaSession/provider ownership. This proved that browser transport buffering is an adapter concern beneath the stable presentation owner, not a reason to create another playback lifecycle.

### HLS timeline ownership

PR #220 fixed a real Android compatibility-mode timeline drag race. While the user owned the range input, active `timeupdate` events could overwrite the selected target before `change`, causing the wrong absolute restart-seek target to be committed.

This proved that UI preview position, transport-local playback position and canonical Recording position are distinct state domains and must have explicit ownership.

### Exact HLS Recording resume

The stacked PR #221 isolated a separate non-zero HLS restart/resume synchronization defect. Captured startup evidence showed audio beginning at 0 while the first decodable video random-access point appeared later. The unsafe worker used input `-ss` with copied A/V streams. Arbitrary exact stream-copy cuts did not provide a sync-safe random-access start.

The accepted fix keeps ordinary HLS start-at-zero on the least-transformation copy/remux path, but for an exact non-zero video Recording resume it promotes implemented H.264/AAC copy tracks to transcode through the existing ADR-0055 backend-scoped policy and fails closed when no implemented exact-resume path exists.

This proves that a presentation identifier alone does not fully describe the semantics of a playback operation. The same HLS delivery family may have different valid adaptation requirements depending on the requested operation and timeline boundary.

---

## Decision

VDR-Suite introduces a stable **playback semantic contract** above internal presentation/worker planning and below client-specific UI rendering.

The architecture becomes:

```text
source + client capabilities + Suite policy
  -> internal MediaPresentationProfile
  -> MediaSession / provider / route ownership
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

`MediaPresentationProfile` remains an internal server-side adaptation/execution plan.

`MediaPlaybackContract` is the provider-free semantic contract consumed by first-party playback clients.

The introduction of this contract does not create a second MediaSession, route, provider lease, player, decoder or lifecycle authority.

---

## MediaPlaybackContract

The normalized contract is versioned and expresses client-relevant truth without leaking internal provider or encoder implementation.

The exact wire representation may evolve during the Phase-65.D implementation slice, but it must be able to represent at least the following semantic domains:

```text
contractVersion
resourceMode
presentationProfileId

playback
  position
  duration
  pause/resume capability

seek
  supported
  mode
  preparing
  window

tracks
  audioSelection
  subtitleSelection
  subtitleOff

continuity
  generation
  state

failure
  classified semantic state
```

The contract may reference the selected public `presentationProfileId` for diagnostics/traceability, but client capability must not be derived only from hard-coded profile-name tests once the normalized semantic field exists.

Internal fields such as raw FFmpeg arguments, private filesystem paths, provider URLs, provider credentials, source-native PIDs, DRM device paths, encoder implementation details or private socket paths are not part of this contract.

---

## Canonical Recording timeline

A completed Recording has one canonical Suite-visible absolute timeline derived from authoritative Recording/index evidence.

Transport-local presentation time is not automatically the same coordinate system.

The semantic relationship is:

```text
absoluteRecordingPosition
  = presentationBasePosition + transportLocalPosition
```

where `presentationBasePosition` may be zero.

Examples:

### Ordinary start

```text
presentationBasePosition = 0
transportLocalPosition   = 120
absoluteRecordingPosition = 120
```

### HLS restart-seek at 47:12

A fresh HLS presentation may begin its local media timeline at or near zero while representing an absolute Recording position of 47:12.

```text
presentationBasePosition = 2832
transportLocalPosition   = 0
absoluteRecordingPosition = 2832
```

The client UI must display and commit the canonical absolute Recording position. It must not infer the absolute position from raw `HTMLMediaElement.currentTime` alone when a presentation base is non-zero.

### UI preview ownership

While a user owns a seek/timeline interaction, the requested preview target is separate from active playback position. Playback `timeupdate` or equivalent transport events must not overwrite the user-owned target before the interaction is committed or cancelled.

This is a general owner rule, not an Android-specific workaround.

---

## Seek semantic modes

A client must be able to distinguish how a supported absolute seek is fulfilled without understanding provider internals.

The normalized semantic model must distinguish at least:

```text
unsupported
in-session reposition
replacement-session restart
```

Additional modes require an explicit contract extension rather than client inference.

### In-session reposition

The same MediaSession remains authoritative while its underlying presentation/worker is repositioned. A transport/discontinuity generation may change even though `mediaSessionId` does not.

### Replacement-session restart

The old presentation/session is stopped or relinquished through the canonical owner and a fresh authorized MediaSession is created at the requested absolute Recording position.

HLS completed-Recording restart-seek is the accepted example.

A client must not implement its own stop/create/retry sequence merely because it owns a timeline widget or track selector. Commands remain delegated to the canonical playback owner.

---

## Exact non-zero HLS Recording resume

For a non-zero exact Recording resume with video, the selected HLS worker plan must provide a synchronized random-access start for all mapped A/V streams.

The accepted invariant is:

```text
ordinary HLS start at 0
  -> retain least-transformation copy/remux when valid

exact non-zero HLS video resume
  -> copied H.264 video is promoted to implemented H.264 transcode
  -> copied AAC audio is promoted to implemented AAC transcode when mapped
  -> video transcode backend resolves through ADR-0055 policy
  -> unsupported exact-resume codec path fails closed
  -> a lower worker-plan guard rejects remaining copied A/V in unsafe non-zero video resume
```

Audio-only HLS resume may remain copy-safe when the exact implemented path supports it.

This rule is operation-specific adaptation. It does not redefine all HLS as transcoding and does not weaken the global least-transformation preference.

---

## Continuity and discontinuity

VDR-Suite distinguishes three different identities/fences:

```text
MediaSession identity
MediaRoute / routeEpoch
playback presentation generation
```

They are not interchangeable.

`routeEpoch` remains a server routing/fencing concept.

`mediaSessionId` remains the public session identity and is not a bearer credential.

A playback presentation generation identifies the decoder-significant incarnation of media delivered within the current client presentation lifecycle.

The existing internal Recording runtime `streamGeneration` proves the need for such a concept, but that internal implementation field is not automatically the public contract. The normalized semantic generation is defined by client-visible continuity, not by exposing private runtime storage.

A generation/discontinuity transition may be caused by a demonstrated operation such as:

- in-session seek/reposition;
- replacement-session restart;
- selected-track reconfiguration that restarts media delivery;
- transport replacement;
- source/timestamp discontinuity;
- another explicitly modeled decoder-invalidating transition.

Clients must never infer continuity only because the MediaSession ID stayed the same.

Conversely, a DOM reparent of the exact same owned media element does not by itself create a media discontinuity.

---

## Persistent client playback owner lifecycle

The accepted Phase-65.D.1 rule remains one persistent first-party playback owner for one presentation lifecycle.

No new `PlaybackController` or parallel player architecture is introduced by this ADR.

The canonical owner shall gain a small explicit lifecycle observation contract during Phase 65.D semantic consolidation. The target shape is conceptually:

```text
snapshot()
subscribe(callback)
```

Exact JavaScript naming is implementation detail, but the semantics are binding.

The published owner state/events must be sufficient for session-bound extensions to observe:

- idle;
- session started;
- active session replaced;
- transport presentation replaced;
- play/pause state where required;
- seek/reposition started/completed where required;
- stopped;
- relinquished/replaced;
- destroyed.

Until this subscription exists, the current bounded `sessionId()`/`state()` observation remains allowed. After an explicit lifecycle subscription is available, new session-bound features must not introduce primary lifecycle truth based only on timer polling, DOM mutation or exported-method interception.

DOM observation may still be used for transport-local facts such as discovering the currently owned `<video>` element or handling native media events. It is not session authority.

---

## Classified playback failures

Existing detailed `reasonCode` values remain valuable and are retained.

Above them, the playback semantic contract defines a stable classification with at least:

```text
category
origin
stage
terminal
recoveryClass
reasonCode
```

Initial stable categories should cover the demonstrated domains:

```text
authorization
source
provider
adaptation
transport
timeline
decoder
buffer
client-platform
```

Origins distinguish where the failure became authoritative, for example:

```text
control-plane
gateway
media-worker
client-transport
platform-player
```

`recoveryClass` describes what kind of recovery may be allowed by policy. It is **not** an instruction to recover automatically.

The following ADR-0053 rules remain binding:

- no silent provider switch;
- no hidden unsafe retry;
- no browser-brand/user-agent fallback architecture;
- no fabricated success after an ambiguous transition;
- another presentation/session may be attempted only through the canonical owner and an independently truthful authorized contract.

---

## Read-only media diagnostics

A future read-only media-pipeline diagnostic projection is allowed and recommended after the semantic contract is established.

It may expose sanitized evidence such as:

```text
mediaSessionId
resource kind/id
presentation profile
absolute start/base position
playback generation
worker lifecycle state
selected normalized tracks
classified terminal reason
sanitized timestamp/startup evidence
```

It must not expose:

- access credentials or cookies;
- private provider URLs/socket paths;
- arbitrary filesystem paths;
- raw FFmpeg command lines as a public client contract;
- writable hardware-device configuration.

Diagnostics are observational only:

```text
diagnostics != lifecycle authority
diagnostics != provider authority
diagnostics != capability authority
```

---

## Shared fMP4/MSE primitives

The browser code currently contains some duplicated fMP4 box/MIME/SourceBuffer primitives between continuous-fMP4 and HLS/fMP4 transports.

A small shared helper library may later consolidate transport-neutral primitives such as init inspection, MIME derivation and safe SourceBuffer operations.

This is technical-debt cleanup, not an architectural completion gate. Continuous streaming and manifest/segment HLS remain different transport lifecycles and must not be collapsed into one universal player implementation merely to remove duplicate helper functions.

---

## Non-goals

This ADR does not authorize or require:

- Phase 66 implementation;
- Teletext/HbbTV runtime;
- Live-TV timeshift;
- user-visible growing-Recording arbitrary seek;
- broader VDR-index functionality unrelated to accepted completed-Recording playback;
- a public third-party `/api/v1` playback contract;
- one universal Suite-owned decoder/rendering engine;
- extraction or vendorization of Kodi VideoPlayer;
- a second MediaSession/player lifecycle;
- a second provider-selection architecture;
- silent compatibility/provider fallback;
- mandatory MSE helper refactoring before semantic work.

---

## Implementation sequencing

The architecture is implemented as bounded Phase-65.D slices rather than one cross-cutting rewrite:

```text
1. normalized MediaPlaybackContract
2. canonical owner lifecycle snapshot/subscription
3. timeline + continuity/discontinuity semantics
4. classified playback failure semantics
5. read-only media diagnostics
```

Shared fMP4/MSE helper consolidation remains separate technical debt and may be performed only when it reduces a demonstrated maintenance risk without mixing unrelated playback behavior.

Each implementation slice must preserve accepted playback behavior and use the shortest relevant regression/real-system gate.

---

## Acceptance invariants

Implementation conforming to this ADR must prove, as applicable:

1. one canonical MediaSession/player owner remains authoritative;
2. public client semantics do not expose provider-native topology or private encoder/device details;
3. absolute Recording position remains correct across transport-local timeline resets;
4. user-owned seek preview cannot be overwritten by unrelated playback-position updates before commit/cancel;
5. in-session reposition and replacement-session restart are distinguishable without profile-name guessing;
6. decoder-significant presentation changes produce explicit continuity/discontinuity evidence;
7. exact non-zero HLS video resume is sync-safe or fails closed;
8. ordinary start-at-zero retains least-transformation selection;
9. classified failures preserve detailed reason codes without enabling silent recovery;
10. lifecycle extensions prove the real production owner/action path required by the Frontend Playback Integration Contract;
11. no Phase-66 or unrelated media scope is introduced.

---

## Consequences

Positive consequences:

- future browser/Android/TV clients consume stable playback semantics instead of transport conventions;
- Recording absolute timeline becomes explicit across restart/reposition paths;
- discontinuity can be implemented without conflating route epoch, session identity and worker generation;
- track/seek/subtitle extensions can observe one canonical lifecycle rather than proliferate timers/wrappers;
- playback failures become actionable and testable without hidden fallback;
- exact HLS resume safety becomes a durable architecture invariant rather than a one-off FFmpeg fix.

Costs:

- current API/controller code must eventually consolidate duplicated profile-dependent capability assembly;
- the frontend owner needs a small explicit lifecycle publication surface;
- existing adapters must migrate incrementally to normalized semantics;
- tests must cover semantic transitions at the production composition root, not only isolated helpers.

These costs are preferred to allowing more Phase-65.D behavior to accumulate as implicit `profileId`, DOM or transport-specific conventions.

---

## Relationship to other ADRs

- ADR-0046 remains authoritative for MediaSession/Gateway/route/provider ownership and security boundaries.
- ADR-0053 remains authoritative for client playback architecture, platform-engine reuse, capability negotiation, least-transformation adaptation and no-silent-fallback behavior.
- ADR-0055 remains authoritative for encoder-backend selection when a video transcode action is required.
- ADR-0056 adds the normalized client semantic layer for timeline, continuity, lifecycle observation and failure classification.

No earlier accepted ADR is superseded.

---

## Back

- [Back to ADR Index](index.md)
- [Back to Current State](../CURRENT.md)
- [Back to Strict Roadmap](../planning/roadmap.md)
