# Phase 65 Recording Playback Closeout Readiness

## Purpose

This document records the accepted implementation and real-system evidence for the
Phase-65 existing-Recording playback vertical and identifies the one remaining
lifecycle hardening gap that still prevents treating 65.A as fully closed.

Phase 65 itself remains active. Live-TV playback, general seek/growing-Recording
semantics and Phase 66 remain outside this Recording-playback closeout-readiness
record.

## Accepted Recording playback path

The implemented first-party flow is:

```text
Recordings 2 detail
  -> authenticated/authorized Recording MediaSession request
  -> stable Suite Recording identity
  -> private local Recording source resolution
  -> source probing + client capability negotiation
  -> least-transformation presentation profile
  -> MediaSession / MediaRoute / ProviderStreamLease / MediaAccessGrant
  -> Suite-owned HLS/fMP4 Streaming Gateway path
  -> browser MediaSource playback
  -> explicit stop/pagehide cleanup
```

Provider-native Recording paths remain server-private. The browser receives Suite
MediaSession/Gateway semantics rather than a VDR filesystem path or provider URL.

## Merged implementation history

The Recording-playback vertical was built and hardened through these accepted
pull requests:

- PR #199 established the authenticated existing-Recording HLS playback vertical;
- PR #200 hardened the initial media/security boundary;
- PR #201 added safe interlaced playback, calibrated software-transcode policy,
  forward startup/rebuffer buffering and pagehide/ended cleanup;
- PR #202 stabilized copied-video HLS publication cadence and preserved terminal
  frontend playback errors;
- PR #203 added calibrated VAAPI UHD transcoding and fail-closed UHD auto policy
  when no measured implementation reaches the required real-time reserve.

The current `main` merge checkpoint after PR #203 is intentionally not treated as
a permanent status authority here. Read live GitHub state whenever the exact
repository head matters.

## Compatibility evidence

Real yaVDR acceptance has covered representative Recording media including:

- H.264 progressive direct/copy paths;
- H.264 interlaced 1080i material requiring deinterlace/transcode;
- HEVC/Main10 UHD material requiring browser-compatible H.264 adaptation;
- AAC, AC3, DTS and fallback AAC stereo audio paths;
- fMP4/HLS playback through the Suite Gateway;
- mobile/VPN playback where startup/rebuffer behavior is sensitive to delivery
  cadence and forward-buffer depth.

The accepted transformation rule remains:

```text
pass-through / copy when valid
  -> repackage/remux only where required
  -> transcode only where materially required
```

Copy HLS uses time-based segmentation for publication cadence. Transcoded HLS
uses forced four-second keyframes and independent segments.

## UHD performance acceptance

`/Drama/A_Star_Is_Born/2026-04-21.19.16.1-0.rec` exposed that software UHD
transcoding was not sustainable on the real yaVDR host.

The final version-4 calibration on the exact host measured:

```text
standard.superfast=2.370
standard.veryfast=1.790

deinterlace.superfast=1.970
deinterlace.veryfast=1.560

uhd-source.superfast=0.831
uhd-source.veryfast=0.680
uhd-source.faster=0.530
uhd-source.fast=0.459
uhd-source.vaapi=4.550
vaapi.device=/dev/dri/renderD128
```

The policy minimum is 1.25x measured real-time speed. The integrated worker
therefore selected the calibrated VAAPI path rather than starting a known-slow
software fallback.

The real integrated worker used hardware HEVC decode/output, VAAPI scaling and
`h264_vaapi` encoding to 1920x1080. Browser playback remained completely stable,
with picture and sound and without the repeated rebuffering seen on the previous
software path.

## HLS and frontend acceptance

The accepted integrated streams preserved the browser delivery contract:

- fMP4/HLS presentation;
- approximately four-second media segments;
- independent-segment signaling for transcoded video;
- 12-second startup and rebuffer-resume thresholds;
- bounded past-buffer trimming;
- terminal playback failures are not overwritten by later `waiting` events.

For the final UHD VAAPI acceptance, the live playlist contained repeated
`4.004s` segments with one observed `3.962292s` segment and remained stable.

## Graceful lifecycle acceptance

Normal client lifecycle cleanup is accepted.

The browser sends a keepalive stop on `pagehide`, normal stop/ended/error paths
terminate the active MediaSession, and the server stops the owned FFmpeg worker
and removes the session workspace.

Real yaVDR acceptance after closing playback showed:

```text
PASS: kein FFmpeg-Worker mehr
MEDIA-SESSIONS: empty
```

Daemon shutdown also stops all runtime-owned Recording workers, and startup
recovery resolves persisted non-terminal bundles.

## Remaining 65.A lifecycle gap

The Recording vertical is not yet fully closeout-ready for an ungraceful client
disappearance.

Current runtime facts on `main` are:

- Recording MediaSessions are issued with a six-hour absolute lifetime;
- MediaAccessGrant authentication applies a 300-second idle timeout and updates
  `lastSeenAt` during successful media access;
- an idle/expired grant therefore stops authorizing subsequent Gateway requests;
- the active FFmpeg worker is owned separately by `RecordingMediaSessionRuntime`;
- that runtime stops a worker on explicit session stop, provisioning failure,
  daemon destruction/shutdown, or restart recovery;
- there is no server-side loop that observes an idle/expired active grant and
  proactively ends the associated active MediaSession/worker.

Therefore this failure mode remains possible:

```text
browser process killed / network disappears
  -> no pagehide/stop request reaches the server
  -> MediaAccessGrant eventually becomes idle-expired
  -> future media requests are denied
  -> but the already-running FFmpeg worker can remain owned until another
     lifecycle event or the much longer absolute session lifetime boundary
```

This is a resource-lifecycle gap, not a playback/codec/HLS gap.

## Required final 65.A hardening

Before declaring the Recording-playback vertical fully closed, add a bounded
server-owned lifecycle reaper/watchdog that:

1. identifies active Recording MediaSessions whose access grant is expired,
   revoked or idle-expired;
2. maps the stale session to the runtime-owned worker without trusting a client
   stop request;
3. terminates the worker and workspace deterministically;
4. persists one terminal session/route/lease/grant outcome with an explicit
   reason such as idle/disconnect expiry;
5. is idempotent and safe against races with normal pagehide/stop/ended cleanup;
6. does not kill a session whose grant has been touched within the accepted idle
   window;
7. proves the hard-disconnect case on real yaVDR.

The reaper must use existing MediaSession/MediaAccessGrant lifecycle evidence;
it must not infer liveness from FFmpeg process state alone.

## Closeout decision

Recording playback is functionally and compatibility accepted, including the
real UHD hardware path. The remaining blocker for full 65.A closeout is the
server-side ungraceful-disconnect lifecycle reaper described above.

After that hardening passes CI and real yaVDR acceptance, the next authorized
Phase-65 product vertical is 65.B Live-TV playback. Phase 66 remains blocked until
the complete Phase-65 gate is satisfied.
