# Phase 65.D Playback Semantics Consolidation Contract

Status: **Binding Phase-65.D implementation plan under accepted ADR-0056.**

This document converts ADR-0056 into bounded implementation slices. It is intentionally a development contract, not a new playback architecture and not a runtime closeout.

## Navigation

- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0046 Streaming Gateway](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053 Client Playback Engine](../adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0055 Media Transcode Backend Selection](../adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [ADR-0056 Playback Presentation, Timeline, Continuity and Failure Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [Frontend Playback Integration Contract](frontend-playback-integration-contract.md)
- [Current Architecture State](current-architecture-state.md)

---

## Purpose

Phase 65.D has accepted the major product behaviors needed to expose Recording and Live playback through one Suite-owned MediaSession architecture. The remaining risk is no longer lack of individual controls. It is semantic drift between:

- internal `MediaPresentationProfile` execution details;
- MediaSession lifecycle and route/provider ownership;
- Recording absolute timeline;
- transport-local MSE/HLS/HTMLMediaElement time;
- persistent browser owner state;
- transport replacement/restart behavior;
- detailed backend/browser error codes.

This contract consolidates those semantics without rewriting the accepted playback stack.

The target is:

```text
internal media planning
  -> normalized playback semantics
  -> one persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

---

## Verified baseline at contract creation

The contract is based on current merged Phase-65.D behavior, not on speculative future design.

### Continuous-fMP4 forward-buffer control

PR #219 merged the bounded continuous-fMP4 MSE forward-buffer/backpressure fix into `main` as merge commit:

```text
5a5789e09bbf79d0e87a25f09ebadce72918f68b
```

That work bounds browser MSE read/append-ahead behavior while preserving the existing MediaSession/player owner.

### Compatibility timeline and exact HLS resume

PR #220 merged into `main` as:

```text
5f9ecc3b1c0af831d8017204da9da63c9ad62610
```

PR #220 contains the compatibility timeline-drag ownership correction and incorporates stacked PR #221 for exact non-zero HLS Recording resume synchronization.

The accepted stacked runtime head was:

```text
870609e7b9a00949448aca1ea1c4db97399bf949
```

PR #221 merged that head into the PR-#220 branch as:

```text
0d237855188ca2483649a72e5a6bb93c4c4b86f1
```

GitHub comparison records that `0d237855...` is one merge commit ahead of `870609e7...` with no file differences, so the accepted runtime tree is preserved by the merge wrapper.

VDR-Suite CI for `0d237855...`:

```text
workflow=VDR-Suite CI
run_number=8256
run_id=32929643924
result=success
```

PR #221 records the runtime startup evidence that motivated the exact-resume rule:

```text
audio first PTS = 0.000 s
video first PTS = 1.350 s
first video keyframe = 1.350 s
audio lead before first decodable video random-access point = 1.350 s
```

The same source remained aligned away from the non-zero resume startup boundary, and ordinary playback from the beginning was synchronized. The accepted implementation therefore keeps start-at-zero copy/remux unchanged while making non-zero exact video HLS resume decode/transcode the mapped A/V tracks or fail closed.

These facts are evidence for ADR-0056. They do not mean every future playback issue is a timeline/discontinuity issue; new defects still require observation -> evidence -> conclusion.

---

## Non-negotiable preserved architecture

Every slice in this contract must preserve:

- one Suite MediaSession/Gateway authorization boundary;
- explicit MediaRoute/provider ownership and route fencing;
- no provider-native URL/path/credential leakage;
- one persistent first-party playback owner per presentation lifecycle;
- platform-native playback engines rather than a Suite-owned universal decoder core;
- least-transformation adaptation except where a demonstrated operation requires stronger transformation for correctness;
- ADR-0055 encoder policy when video transcoding is required;
- no silent provider switch or hidden unsafe recovery;
- truthful unsupported capability rather than fabricated seek/track/timeline behavior;
- Phase 66 remains outside this workstream.

---

# Slice 1 — Normalized MediaPlaybackContract

## Goal

Introduce one provider-free semantic representation for client-relevant playback capability/state without changing existing accepted user behavior.

## Required semantic coverage

The normalized contract must represent at least:

```text
contract version
resource mode
public presentation profile id
absolute position
duration
presentation base position when required
pause/resume capability
seek supported/preparing/window/mode
normalized audio selection capability
normalized subtitle selection/OFF capability
continuity generation/state
classified failure state when present
```

Exact JSON/C++ shape is an implementation decision, but it must have one canonical construction path rather than independent controller-specific assembly for create/status/track responses.

## Hard boundary

`MediaPresentationProfile` remains internal execution/adaptation state.

The normalized contract must not expose:

- raw source-native stream/PID identity as client contract;
- raw FFmpeg command arguments;
- private workspace/filesystem paths;
- provider URL/socket topology;
- encoder hardware-device path;
- access credentials.

## Required regression proof

At minimum:

1. completed progressive-fMP4 session exposes truthful in-session seek semantics;
2. completed HLS compatibility session exposes truthful replacement-session restart semantics;
3. growing/unsupported source does not gain fake seek capability;
4. audio/subtitle capability remains identical to the accepted normalized track behavior;
5. exact non-zero HLS video resume retains the sync-safe transcode/fail-closed requirement;
6. no client capability requires parsing an internal encoder/backend field.

## Completion boundary

Slice 1 closes only when existing browser behavior consumes or can consume the normalized semantic fields without changing MediaSession/provider ownership.

---

# Slice 2 — Canonical Playback Owner Lifecycle Publication

## Goal

Publish lifecycle state from the already accepted persistent playback owner so session-bound extensions stop reconstructing ownership through unrelated mechanisms.

## Target semantic interface

Conceptually:

```text
snapshot()
subscribe(callback)
```

Exact names are not fixed by this document.

The publication must represent enough state/events to observe:

- idle;
- MediaSession started;
- active MediaSession replaced;
- transport presentation replaced;
- play/pause where needed by owner-level features;
- seek/reposition transition where needed;
- stop;
- relinquish/replacement;
- destroy/page teardown.

## Migration rule

Current bounded polling of `sessionId()`/`state()` remains valid until the canonical subscription exists.

After publication exists:

- new session-bound features use it as primary lifecycle truth;
- existing decorators may migrate incrementally in bounded changes;
- DOM observation remains allowed for transport-local/media-element facts;
- DOM mutation, timer expiry or exported-method wrapping alone is not session authority.

## Production integration proof

Tests must use the real production composition root required by `frontend-playback-integration-contract.md` and prove at least:

```text
user-style action
  -> canonical owner transition
  -> subscription state/event
  -> expected Suite request/state change
```

Replacement tests must include progressive -> HLS ownership where the real product can perform it.

## Completion boundary

No second player/controller/session owner may be introduced.

---

# Slice 3 — Timeline and Continuity/Discontinuity Semantics

## Goal

Make canonical absolute position and decoder-significant presentation transitions explicit across accepted seek/restart paths.

## Timeline requirements

The Suite-visible Recording position is absolute.

A transport may have a local presentation clock with a non-zero base relationship:

```text
absoluteRecordingPosition
  = presentationBasePosition + transportLocalPosition
```

The contract must preserve this distinction through:

- ordinary start;
- progressive in-session reposition;
- HLS replacement-session restart;
- track change that restarts a presentation;
- transport replacement;
- stop/resume.

User-owned seek preview remains separate from active playback position until commit/cancel. Playback `timeupdate` must not overwrite a user-owned target.

## Continuity requirements

Do not conflate:

```text
mediaSessionId
routeEpoch
playback presentation generation
```

The implementation must expose a normalized generation/continuity state sufficient for the client to recognize decoder-significant replacement even when `mediaSessionId` is unchanged.

The existing internal `streamGeneration` may inform implementation but is not automatically the public field.

## Initial discontinuity causes

Only causes demonstrated by implemented behavior are required initially:

- in-session seek/reposition;
- replacement-session restart;
- media-restarting track selection;
- transport replacement;
- explicit source/timestamp discontinuity when later demonstrated.

Do not invent a generic recovery mechanism for unobserved cases.

## Required regression proof

At minimum:

1. progressive in-session seek preserves absolute Recording position across worker generation change;
2. HLS restart-seek begins a new authorized session but preserves the requested absolute position through the presentation base;
3. timeline drag ownership survives active `timeupdate` until commit;
4. track restart preserves canonical absolute position;
5. a same-element DOM reparent does not create a fake discontinuity;
6. route epoch and playback generation remain independent in tests/contracts.

## Real-system gate

Because timeline/discontinuity semantics are product-visible and decoder-sensitive, the final slice requires real yaVDR/browser acceptance on every changed supported path.

---

# Slice 4 — Classified Playback Failure Semantics

## Goal

Normalize playback failures above detailed implementation `reasonCode` values without enabling silent recovery.

## Required shape

The stable semantic failure object must include at least:

```text
category
origin
stage
terminal
recoveryClass
reasonCode
```

Initial categories:

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

Initial origins may include:

```text
control-plane
gateway
media-worker
client-transport
platform-player
```

The implementation may add a category only with a demonstrated distinct semantic requirement.

## Recovery rule

`recoveryClass` is descriptive policy evidence, not an imperative.

No implementation may infer:

```text
failure -> silently switch provider
failure -> silently choose unrelated profile
failure -> blindly recreate session
```

Any replacement path remains a canonical owner action based on a truthful newly authorized contract.

## Required regression proof

At minimum classify and preserve detailed reason codes for representative:

- authorization/access failure;
- unsupported/invalid source or exact-resume profile;
- worker/adaptation failure;
- browser transport/buffer failure;
- platform media/decoder failure where the browser can report it.

Tests must prove that classification itself does not trigger hidden fallback.

---

# Slice 5 — Read-only Media Pipeline Diagnostics

## Goal

Provide bounded diagnostic evidence for media-session debugging without creating lifecycle or provider authority.

This slice is sequenced after the semantic contract because diagnostics should project stable semantics rather than define them accidentally.

## Allowed evidence

A sanitized read-only projection may include:

```text
mediaSessionId
resource identity
public presentation profile
absolute start/base position
playback generation
worker lifecycle state
selected normalized tracks
classified terminal reason
sanitized startup/timestamp evidence
```

## Forbidden evidence/controls

Do not expose as public diagnostics:

- cookies or access credentials;
- provider-native URLs/socket paths;
- arbitrary filesystem paths;
- arbitrary FFmpeg command execution;
- writable DRM device path;
- a mutation surface that can steer provider/session ownership.

## Authority rule

```text
diagnostics != lifecycle authority
diagnostics != provider authority
diagnostics != capability authority
```

## Gate

This slice is recommended but must not block earlier semantic correctness merely because diagnostics are not yet implemented.

---

# Separate Technical Debt — Shared fMP4/MSE Primitives

Current continuous-fMP4 and HLS/fMP4 browser transports contain overlapping MP4-box/MIME/SourceBuffer helper logic.

A later bounded cleanup may share transport-neutral primitives such as:

- init-segment inspection;
- codec/MIME derivation;
- safe SourceBuffer append/remove operations;
- buffered-range calculations.

It must **not** merge continuous streaming and HLS manifest/segment lifecycles into one universal transport owner.

This cleanup is not a Phase-65.D architecture gate and must not be mixed into a semantic slice without a concrete maintenance/test reason.

---

## Validation strategy

Follow the shortest gate that can invalidate the current slice.

### Docs-only architecture changes

- ADR index validation;
- development/roadmap link validation;
- documentation/static guards;
- `git diff --check` equivalent in CI.

### Frontend semantic changes

- focused production-composition frontend regression;
- replacement-owner regression where applicable;
- `frontend-regression-test`;
- packaging/install staging if frontend asset wiring changes;
- real browser/yaVDR acceptance for changed runtime semantics.

### Server semantic changes

- focused media/domain/API tests;
- directly affected fast-regression/architecture checks;
- packaging/install staging when runtime files/wiring change;
- real yaVDR acceptance for changed media behavior.

Full repository CI remains the final head gate for review/merge/Phase-65 closeout, not a substitute for focused evidence.

---

## Explicitly outside this contract

- Phase 66 Teletext/HbbTV;
- Legacy OSD;
- Live-TV timeshift;
- broad growing-Recording seek;
- public third-party API stabilization;
- universal decoder/player replacement;
- unrelated media codec expansion;
- automatic provider switching;
- speculative recovery behavior.

---

## Phase-65.D exit relationship

This consolidation does not itself close Phase 65.D. It defines how the remaining semantic requirements are completed coherently.

The complete Phase-65.D gate still requires:

- accepted existing playback behavior remains regression-free;
- normalized playback semantic contract is implemented;
- canonical owner lifecycle publication is implemented for new session-bound work;
- required timeline/continuity/discontinuity semantics are implemented;
- classified playback failures are implemented;
- all required supported paths pass exact-head CI and real-system acceptance where applicable.

Phase 66 remains blocked until Phase 65 is fully closed and explicitly started.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current State](../CURRENT.md)
- [Back to Strict Roadmap](../planning/roadmap.md)
