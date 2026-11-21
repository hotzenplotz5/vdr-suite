# Phase 65.D Recording Audio Track Selection Acceptance

Status: **Runtime accepted on real yaVDR for Recording/HLS audio-track selection.**

This document records the immutable acceptance evidence for the Recording audio-track slice of Phase 65.D. It does not close the complete audio/subtitle PR and does not authorize Phase 66 or Timeshift work.

## Accepted candidate

- Branch: `work/phase65d-audio-subtitle-selection`
- Accepted product head: `f497960b44b969c3f7fbe42887f85f817a107af9`
- Pull request: `#216` (`Phase 65.D: normalized audio and subtitle selection`)
- PR state at acceptance: open, Draft, not merged
- Base: `main`
- Base head: `30a25417926726ddf26052f3c6cd30faef255b09`

## CI evidence

GitHub Actions `VDR-Suite CI #8182`, Run ID `32810410115`, completed PASS for the accepted product head.

Passing jobs included:

- `frontend-regression-test`
- `fast-regression-test` including daemon build
- `packaging-regression-test`
- `architecture-check`
- `make-test-audit`
- `docs-check`

The frontend regression explicitly covered:

- the binding playback frontend integration contract;
- production-style owner-internal Start rather than only `wrapped.start()`;
- progressive-to-HLS transport replacement;
- the outer Fast owner reporting `fallback` while the active HLS owner reports `playing`/`paused`;
- HLS normalized audio replacement preserving position and pause state.

## Real source evidence

Acceptance recording:

- Recording ID: `12`
- Title: `Cloverfield`
- Real recording directory: `/srv/vdr/video.00/Mystery/Cloverfield/2026-05-09.07.36.1-0.rec`

The exact product-style concat ffprobe against the active MediaSession workspace reported two audio streams:

1. AAC, stereo, language `ger`
2. AAC, stereo, language `eng`

The normalized client contract exposed these as session-local Suite IDs rather than provider-native identifiers:

- `audio-1` -> Deutsch · AAC · stereo
- `audio-2` -> Englisch · AAC · stereo

No provider PID or ffmpeg source index is part of the public selector identity.

## Real browser/runtime evidence

The accepted path was tested on the real yaVDR from an Android browser after the normal progressive-fMP4 attempt had fallen back to the HLS compatibility path.

Observed playback contract:

- HLS compatibility mode active;
- Recording remained completed / non-growing;
- indexed time-jump controls remained available;
- audio selector remained mounted below the playback controls after progressive -> HLS replacement;
- selector displayed both `Deutsch · AAC · stereo` and `Englisch · AAC · stereo`.

Runtime acceptance sequence:

1. Start Cloverfield with German selected.
2. While playback is running, select English.
3. HLS stream is replaced through the existing Phase-65.D.2 owner at the current absolute Recording position.
4. Playback continues at approximately the previous position rather than restarting at zero.
5. The audible soundtrack is English.
6. Existing time-jump controls remain functional.
7. Select German again.
8. Playback position remains preserved.
9. The audible soundtrack returns to German.

The user explicitly confirmed both directions work correctly and that the audible soundtrack matches the selected language. A runtime screenshot also shows active `Englisch · AAC · stereo` while HLS playback continues at `00:01:51 / 01:24:39`.

## Integration defects found and closed during acceptance

This slice exposed a recurring frontend integration failure class and therefore produced the binding `frontend-playback-integration-contract.md` plus AGENTS rules.

The concrete audio defects were:

1. The track decorator originally depended on its exported `wrapped.start()`, while the production Fast owner binds the visible Start button directly to an internal `startPlayback` closure. The browser therefore created a real MediaSession without ever triggering track-status discovery.
2. The progressive DOM node is replaced by the HLS fallback DOM. Controls attached to that replaceable node disappeared on fallback. Track controls now live in a stable outer owner shell.
3. The outer Fast owner truthfully reports state `fallback` after transport replacement. The audio selector originally rejected this before delegating to HLS because it allowed only `playing`/`paused`. HLS selection now resolves active playback state from the published HLS owner.

Regression tests now model these production boundaries explicitly.

## Accepted invariants

The following Recording audio behavior is accepted and must not be reopened by the subsequent subtitle slice without direct evidence:

- normalized session-local audio IDs;
- no PID/provider-index leak in the browser API;
- truthful language/codec/layout labels;
- stable track-control ownership across progressive -> HLS replacement;
- delayed/internal Start is discovered through canonical session ownership;
- HLS audio selection reuses the existing D.2 stop/resume replacement owner;
- absolute playback position is preserved across HLS audio replacement;
- paused playback remains paused across HLS audio replacement;
- old HLS owner cleanup occurs before replacement;
- selected audio preference is carried into the replacement MediaSession and ffmpeg mapping;
- seek/jump controls remain functional after an audio switch;
- failure remains fail-closed rather than advertising an unverified selection.

## Subtitle boundary

This acceptance does **not** claim working selectable subtitles. Current workers still omit subtitles from the media output where `-sn` applies. Subtitle implementation must begin from the binding subtitle pre-implementation gate in `frontend-playback-integration-contract.md` and reuse the same persistent track owner proven here.

In particular, the subtitle slice must not regress or duplicate the accepted audio/session/HLS replacement lifecycle.
