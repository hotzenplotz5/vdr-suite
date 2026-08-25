# Frontend Playback Integration Contract

Status: **Binding for Phase 65.D playback frontend work and later playback UI changes.**

This contract prevents a recurring integration failure class: a feature can be locally correct, installed, and covered by isolated tests while the real browser production lifecycle never enters the new code. Playback controls, track selection, seek/restart helpers and later subtitle controls must therefore prove their integration at the actual playback ownership boundary, not only at an isolated adapter or DOM fragment.

The contract supplements ADR-0046, ADR-0053, ADR-0055 and the accepted Phase-65.D.1 persistent browser playback shell. It does not create a second playback architecture or change MediaSession authority.

## Production composition root

For Recording playback the relevant first-party production path is:

```text
Recordings 2 module
  -> recordings2-browser-view
  -> VdrSuiteRecordings2Playback.createPanel(...)
  -> one persistent Recording playback owner
  -> replaceable transport presentation
       -> progressive-fMP4
       -> HLS compatibility fallback
  -> Suite MediaSession API
```

Cross-cutting playback features attach to the persistent playback owner. They must not attach their correctness to a replaceable progressive or HLS DOM node.

Audio and subtitle controls share this same track-control lifecycle boundary. Subtitle work must extend the established track owner rather than create a separate subtitle player, restart path or MediaSession lifecycle.

## Binding integration invariants

### 1. Production path first

Before implementing a new playback-facing feature, identify and test the exact production composition root that creates the owner used by the user-visible view. A direct unit call to a helper, decorator or factory is not proof that the real Recordings 2 view reaches that code.

### 2. One public playback owner

There is one first-party Recording playback owner for a presentation. New controls consume that owner or an explicitly published child transport owner. They must not create a competing player, MediaSession owner, seek implementation, HLS replacement implementation or provider-specific client contract.

### 3. Method interception is not a lifecycle contract

A decorator must not assume that replacing or wrapping an exported method such as `start()`, `stop()`, `seekAbsolute()` or `selectAudioTrack()` observes every real user action.

An owner may bind its own DOM control directly to an internal closure before an outer decorator exists. In that case the browser action can legitimately bypass the later wrapper method while still changing the canonical owner state.

Session-bound extensions must therefore derive lifecycle truth from the canonical owner state (`sessionId()`, `state()`, documented ownership events, or a future explicit lifecycle subscription), not solely from interception of exported methods.

### 4. Observe the whole owner lifetime

A session-bound extension is active from owner creation until owner destruction. It must correctly handle:

- idle -> first MediaSession;
- progressive-fMP4 -> HLS fallback replacement;
- stop -> no active MediaSession;
- restart/resume -> new active MediaSession;
- seek/reposition where the existing contract changes stream state;
- explicit replacement/relinquish;
- destroy/page teardown.

A bounded startup timer that expires before the user presses Play is not a valid lifecycle observer.

Local observation of `sessionId()` is allowed when no explicit lifecycle event exists. Such observation must not generate server traffic by itself; server state is refreshed only when the canonical active session identity appears or changes.

### 5. Stable presentation owner

Controls that must survive a transport switch live outside the replaceable transport DOM. A progressive player may be replaced by HLS presentation without removing audio/subtitle controls, restart controls or another owner-level control that remains valid for the same presentation.

### 6. Commands reuse existing owners

After capability/state discovery, commands go through the existing public owner boundary. For example, HLS audio selection reuses the Phase-65.D.2 replacement/restart owner and its position-preserving semantics. A track control must not duplicate stop/create/resume sequencing merely because it owns the selector UI.

### 7. Session state decides control availability

A control is visible/enabled only when the active MediaSession and active transport both support the operation. Browser UI must not infer support merely because source metadata contains multiple tracks or because a helper function exists.

Unknown language, role, default state or subtitle format remains unknown. Provider IDs/PIDs are not promoted to the public client contract.

### 8. Real action-to-request proof

For every session-bound browser feature, at least one integration test must prove the real chain:

```text
production-style user action
  -> canonical playback owner changes state/session
  -> extension observes that state/session
  -> expected Suite API request/state transition occurs
  -> resulting UI is owned by the same presentation
```

For features whose owner binds controls directly to internal closures, tests must exercise that internal-button path or an equivalent production entry. Calling only a decorator's `wrapped.start()` is explicitly insufficient.

### 9. Replacement paths are first-class test cases

When the real browser can replace one transport with another, the integration test must cover that replacement through the same owner topology as production. Tests that instantiate an already-final HLS owner directly are useful unit tests but are not sufficient integration coverage for a progressive-to-HLS browser path.

### 10. Installation is not runtime integration

An installed file checksum, expected bundle content and JavaScript syntax check prove deployment identity only. They do not prove that an already-persistent browser owner was recreated or that the production lifecycle reached the new code. Real-runtime acceptance must recreate the relevant owner and verify the expected action/request/state transition.

## Required test shape for track controls

The Recording track-control regression must include a case where:

1. the track decorator creates its stable owner shell;
2. the inner playback owner is still idle;
3. a production-style owner-internal Start control activates the MediaSession without calling the decorator's exported `start()` wrapper;
4. the track owner observes the newly active session and requests `track-status`;
5. a progressive-to-HLS replacement changes the canonical session ID while preserving the outer track owner;
6. the track owner requests fresh status for the HLS session;
7. a supported normalized audio selection delegates to the existing HLS replacement owner;
8. the replacement session is verified before the UI reports success.

This test is the minimum lifecycle regression for both audio and the subsequent subtitle-selection slice.

## Subtitle pre-implementation gate

Before adding selectable subtitle UI, verify all of the following against this same owner path:

- the current active MediaSession can be observed from idle through fallback/restart;
- subtitle metadata is returned by the normalized track contract without PID/provider leakage;
- the active profile truthfully reports whether selection and OFF are supported;
- any server-side restart uses the existing D.2 owner and preserves position when the profile supports it;
- browser-native text-track rendering is used when the selected delivery format supports it;
- bitmap DVB subtitles are not mislabeled as browser-native text tracks;
- the production integration test starts from the real owner lifecycle rather than directly invoking the subtitle adapter.

If any of these cannot be proven, subtitle UI remains hidden/unsupported rather than exposing a fake control.

## Acceptance boundary

A playback frontend change is not considered integrated merely because its unit tests and API tests pass. For runtime-sensitive changes the final acceptance evidence must identify:

- exact installed candidate;
- active production profile;
- real browser action;
- resulting owner/session identity transition;
- relevant Suite API operation;
- preserved playback invariants such as position, pause state and deterministic old-stream cleanup.

This contract stays inside Phase 65.D. It does not authorize Phase 66, Live-TV timeshift or unrelated discontinuity work.
