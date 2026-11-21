# Post-Phase-69 Recording Playback Truthfulness and Fullscreen Hardening

Status: **non-numbered correctness hardening between completed Phase 69 and not-yet-started Phase 70.**

## Reported regression

The Recording playback surface could show:

```text
Aufnahme läuft · Index konnte nicht für Seek bereitgestellt werden.
```

while the browser had not yet confirmed actual media playback. The same surface also lacked a Recording fullscreen action.

## Root cause

Two independent frontend facts were conflated:

1. the Recording MediaSession/seek contract may finish preparing without a usable seek window;
2. actual browser playback is confirmed only by the canonical media element's `playing` event.

The lazy index/seek poll wrote `Aufnahme läuft` directly and marked missing seek as an error. It therefore could overwrite the truthful startup/waiting presentation even though seek capability is optional for basic playback.

The canonical owner also treated the earlier `play` event as `playing`. `play` only establishes playback intent; the first `playing` event is the existing first-media boundary.

For fullscreen, Recording playback had no control on its stable outer owner. Putting the action inside the replaceable progressive transport would lose it when playback falls back to HLS.

## Bounded fix

- the Recording owner remains `starting` until the first real `playing` event;
- the seek/index poll derives its wording from the current playback activity instead of fabricating `Aufnahme läuft`;
- unavailable seek is shown as the non-fatal user-facing limitation `Spulen/Springen derzeit nicht verfügbar.`;
- internal index terminology is no longer used as the user-facing playback error;
- Recording gets a `Vollbild` action on the existing stable browser-local owner shell;
- fullscreen targets the currently active canonical Recording video and therefore follows progressive-to-HLS replacement without creating another player or MediaSession;
- Live-TV keeps its established dedicated fullscreen control and does not receive a duplicate from the Recording owner decorator.

## Architecture retained

This hardening does not:

- create a second media element or playback owner;
- change MediaSession, provider or public-v1 contracts;
- turn missing seek capability into a playback failure;
- alter VDR/SuiteBridge runtime;
- start Phase 70.

Real browser acceptance remains required for the original Recording symptom because hosted CI can verify lifecycle/control ownership but cannot prove moving video pixels.
