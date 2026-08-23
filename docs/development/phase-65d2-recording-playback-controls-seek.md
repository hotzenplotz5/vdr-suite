# Phase 65.D.2 — Recording Playback Controls and Seek

Status: **Planned next bounded Phase-65.D slice. Runtime implementation not started by this document.**

## Position

Phase 65.D is the active Client playback abstraction vertical under ADR-0053.

Phase 65.D.1 — Persistent Browser Playback Shell is accepted and closed through PR #210. The next product gap is no longer persistent ownership of one running browser player; it is a truthful semantic control surface for Recording playback.

This plan refines ADR-0053. It does not create a second media architecture and does not change the provider, MediaSession, Gateway or transcode-policy boundaries.

## Product goal

A Recording should behave like a mature VDR playback surface while remaining portable to browser, Android/Android TV, Kodi, desktop and television clients.

The user-facing semantic target is:

```text
open Recording
  -> play / pause / stop
  -> read current position and duration
  -> scrub on a timeline where seek is supported
  -> jump to an absolute time
  -> skip backward / forward by relative time
  -> use fast-forward / rewind semantics
  -> optionally navigate normalized Recording marks
  -> continue through the same Suite MediaSession semantics
  -> close with deterministic cleanup
```

The client contract describes intent. Each platform adapter may implement the intent with the native facilities appropriate to its playback engine.

## 65.D.2 required controls

The first accepted control set should include:

- play;
- pause/resume;
- explicit stop;
- current playback position;
- known duration/readable extent;
- seekable window/capability;
- timeline/scrubber when the selected source/profile supports it;
- absolute time jump, for example `00:42:30`;
- relative backward/forward skip with product defaults such as 10 seconds and 60 seconds;
- jump to beginning;
- repeated seek without leaking or constructing provider-native URLs;
- clear `seeking`, `buffering`, `playing`, `paused`, `ended` and classified error state where those states affect the user-visible result.

Exact button layout and shortcut mapping remain client UX. The semantic operations must be reusable by remote-control and television clients rather than being encoded only as browser mouse behavior.

## Seek truthfulness

Seek is a capability, not a visual promise.

The Suite must not implement a timeline by merely assigning browser `currentTime` when the selected MediaSession profile cannot actually satisfy arbitrary seek.

For every active Recording presentation, the Suite/client boundary must truthfully distinguish at least:

```text
seek unsupported
seek supported within explicit window
completed immutable Recording
still-growing Recording
known duration / readable extent
unknown or changing end
```

A completed Recording may use a different internal seek mechanism from a growing Recording. That implementation difference must remain behind Suite MediaSession semantics.

Continuous fMP4 must not acquire fake HTTP byte-range or immutable-length claims merely to make browser controls appear enabled. If the existing delivery profile cannot seek truthfully, the coherent implementation may add a server-owned seek/reposition contract, choose another already-authorized seek-capable presentation, or report seek unsupported until the capability exists.

## Absolute and relative jumps

The semantic controller should support both forms explicitly:

```text
seekTo(position)
skipBy(delta)
```

Examples:

- `skipBy(-10s)`;
- `skipBy(+10s)`;
- `skipBy(-60s)`;
- `skipBy(+60s)`;
- `seekTo(42m30s)`.

Clients may expose different default skip intervals, but the Suite contract must use normalized media time rather than provider offsets, file byte positions or player-private identifiers.

## Fast-forward and rewind

Fast-forward/rewind are user semantics, not a requirement that every platform support negative or arbitrary playback rates.

A platform adapter may implement trick-play using native playback rates when reliable, controlled repeated time jumps when native reverse playback is unavailable, or another platform-appropriate mechanism. The observable state and final playback position must remain coherent.

Do not expose a capability such as `reversePlayback=true` unless the actual selected adapter/profile supports it. A browser limitation must not become a false universal Suite limitation, and a browser-only implementation must not become the public contract.

## VDR Recording marks

VDR-style previous/next-mark navigation is a desirable Recording capability, but provider-native mark files, raw VDR index positions and filesystem paths must not become client contracts.

If mark navigation is implemented in this slice, it must use normalized Suite semantics such as:

```text
RecordingMark
  stable recording scope
  normalized media time
  optional kind/label
  source revision/fingerprint where required

previousMark()
nextMark()
seekToMark(markId)
```

The implementation must first prove how VDR marks and index timing map safely to the selected Recording source. If this requires a larger VDR-index mapping contract than the core time-seek vertical, mark navigation remains a coherent follow-up rather than forcing unsafe or provider-coupled semantics into 65.D.2.

## Growing Recordings

Growing Recording playback must stay truthful.

Until demonstrated and implemented, a growing Recording may expose a restricted or unsupported seek window. It must never be treated as a complete immutable file solely to enable controls.

When growing seek is implemented, current readable extent, seek window and end behavior must be explicit and must tolerate the source extending while playback is active.

## Live-TV boundary

These Recording controls do **not** create Live-TV timeshift.

Normal Live playback may use play/pause only where the active platform/source semantics truthfully support it, but backward seek, rewind, arbitrary timeline navigation or pause-with-buffer for Live TV require the separate explicit timeshift architecture described by ADR-0053.

No implicit global timeshift buffer is introduced by Phase 65.D.2.

## Follow-on Phase-65.D product blocks

The intended continuation after the control/seek vertical is:

### Track and playback-state semantics

- normalized audio-track selection;
- normalized subtitle-track selection and off state;
- default/selected track state;
- buffering/seeking/discontinuity state;
- classified playback failures and platform-adapter reporting.

### Durable Recording resume/progress

- durable resume position bound to stable Suite Recording identity and actor scope;
- `Fortsetzen bei …`;
- `Von vorne starten`;
- watched/unwatched or completed semantics;
- progress indicator on Recording surfaces;
- explicit progress reset;
- no persistence keyed by provider URL, filesystem path or browser-private player identity.

### Recording discovery UX

The existing global search already covers Recording results. A later Recordings-specific UX block may add local search/filter/sort without inventing a second search architecture, for example:

- title/subtitle/description;
- people;
- Genre;
- folder;
- watched/unwatched/in-progress;
- date, title, duration and progress sorting.

This discovery work is product UX and is not part of the playback-engine architecture unless a demonstrated backend/query gap requires a separate contract.

## Explicit non-goals for 65.D.2

- no Live-TV timeshift;
- no universal Suite-owned decoder/rendering engine;
- no provider-native URLs, VDR filesystem paths or provider credentials in clients;
- no browser-brand or user-agent routing;
- no fake Range/seek advertisement;
- no forced transcode merely to normalize controls;
- no durable resume/progress unless explicitly promoted into its own coherent follow-on block;
- no Phase 66 runtime.

## Source-review requirement before implementation

Before changing runtime code, inspect current `main` for:

- the accepted 65.D.1 persistent playback owner and exact media-element lifecycle;
- Recording playback/session creation and current profile selection;
- current `progressive-direct`, `progressive-fmp4` and HLS behavior;
- any existing Range or seek-capability fields in MediaSession descriptors;
- browser continuous-fMP4/MSE buffering and SourceBuffer retention behavior;
- VDR Recording index/marks evidence already exposed behind Suite boundaries;
- current Recording duration/source-fingerprint/completed-vs-growing evidence;
- existing tests that intentionally guard against fake seek/range semantics.

Do not choose a seek mechanism before that source review proves the current capability gap.

## Acceptance target

A 65.D.2 runtime candidate is not accepted from UI tests alone. On the real yaVDR plus first-party Android/browser client, for at least one suitable completed Recording, acceptance should prove:

```text
RECORDING_PICTURE_SOUND=PASS
PLAY_PAUSE_RESUME=PASS
POSITION_DURATION_STATE=PASS
RELATIVE_SKIP_BACK=PASS
RELATIVE_SKIP_FORWARD=PASS
ABSOLUTE_TIME_SEEK=PASS
TIMELINE_SEEK=PASS
REPEATED_SEEK_STABILITY=PASS
STOP_CLEANUP=PASS
PROVIDER_PRIVACY=PASS
FALSE_SEEK_ADVERTISEMENT=absent
```

If VDR mark navigation is included in the accepted candidate, add real previous/next-mark acceptance on a Recording with known marks.

The exact source head must pass the full required GitHub CI before real-system installation. Installed runtime identity must be verified before acceptance, following `AGENTS.md`.

## Relationship to Phase 65 closeout

Phase 65.D.2 advances the semantic Client playback abstraction but does not automatically close Phase 65.D or Phase 65.

After 65.D.2, the repository must re-evaluate the remaining ADR-0053 and Phase-65 acceptance gaps rather than assuming all planned follow-ons are mandatory. Truthful unsupported capability remains preferable to fabricated support.

Phase 66 remains blocked until Phase 65 is fully closed and separately authorized.
