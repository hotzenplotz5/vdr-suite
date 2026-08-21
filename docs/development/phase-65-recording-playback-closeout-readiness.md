# Phase 65 Recording Playback Closeout

## Purpose

This document records the accepted implementation and real-system evidence for the Phase-65 existing-Recording playback vertical.

It is historical closeout evidence for **65.A**. Later Phase-65 sequencing is owned by [Current State](../CURRENT.md) and the [Strict Roadmap](../planning/roadmap.md); do not interpret the historical “next” wording from this closeout as current authorization.

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
  -> graceful stop or server-owned stale-session reaping
```

Provider-native Recording paths remain server-private. The browser receives Suite MediaSession/Gateway semantics rather than a VDR filesystem path or provider URL.

## Merged implementation history

The Recording-playback vertical was built and hardened through these accepted pull requests:

- PR #199 established the authenticated existing-Recording HLS playback vertical;
- PR #200 hardened the initial media/security boundary;
- PR #201 added safe interlaced playback, calibrated software-transcode policy, forward startup/rebuffer buffering and pagehide/ended cleanup;
- PR #202 stabilized copied-video HLS publication cadence and preserved terminal frontend playback errors;
- PR #203 added calibrated VAAPI UHD transcoding and fail-closed UHD auto policy when no measured implementation reaches the required real-time reserve;
- PR #204 closes the remaining ungraceful-client-disconnect lifecycle gap by reaping stale active Recording sessions from existing MediaAccessGrant evidence.

Exact live `main` and PR heads are intentionally not permanent status authority in this document. Read live GitHub state whenever the exact repository checkpoint matters.

## Compatibility evidence

Real yaVDR acceptance has covered representative Recording media including:

- H.264 progressive direct/copy paths;
- H.264 interlaced 1080i material requiring deinterlace/transcode;
- HEVC/Main10 UHD material requiring browser-compatible H.264 adaptation;
- AAC, AC3, DTS and fallback AAC stereo audio paths;
- fMP4/HLS playback through the Suite Gateway;
- mobile/VPN playback where startup/rebuffer behavior is sensitive to delivery cadence and forward-buffer depth.

The accepted transformation rule remains:

```text
pass-through / copy when valid
  -> repackage/remux only where required
  -> transcode only where materially required
```

Copy HLS uses time-based segmentation for publication cadence. Transcoded HLS uses forced four-second keyframes and independent segments.

## UHD performance acceptance

`/Drama/A_Star_Is_Born/2026-04-21.19.16.1-0.rec` exposed that software UHD transcoding was not sustainable on the real yaVDR host.

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

The policy minimum is 1.25x measured real-time speed. The integrated worker therefore selected the calibrated VAAPI path rather than starting a known-slow software fallback.

The real integrated worker used hardware HEVC decode/output, VAAPI scaling and `h264_vaapi` encoding. Browser playback remained completely stable, with picture and sound and without the repeated rebuffering seen on the previous software path.

## HLS and frontend acceptance

The accepted integrated streams preserve the browser delivery contract:

- fMP4/HLS presentation;
- approximately four-second media segments;
- independent-segment signaling for transcoded video;
- 12-second startup and rebuffer-resume thresholds;
- bounded past-buffer trimming;
- terminal playback failures are not overwritten by later `waiting` events.

For the final UHD VAAPI acceptance, the live playlist contained repeated `4.004s` segments with one observed `3.962292s` segment and remained stable.

## Graceful lifecycle acceptance

Normal client lifecycle cleanup is accepted.

The browser sends a keepalive stop on `pagehide`, normal stop/ended/error paths terminate the active MediaSession, and the server stops the owned FFmpeg worker and removes the session workspace.

Real yaVDR acceptance after closing playback showed:

```text
PASS: kein FFmpeg-Worker mehr
MEDIA-SESSIONS: empty
```

Daemon shutdown also stops all runtime-owned Recording workers, and startup recovery resolves persisted non-terminal bundles.

## Ungraceful-disconnect lifecycle hardening

PR #204 closes the remaining Recording lifecycle gap for a killed browser process or lost network that cannot deliver a normal stop request.

The server now:

1. retains the issued MediaAccessGrant id with the runtime-owned Recording worker;
2. evaluates the persisted grant against the same 300-second idle semantics used by Gateway authentication;
3. invokes the check from the existing HTTP listener tick every five seconds;
4. stops the owned worker through the existing `RecordingMediaSessionRuntime::stop()` path when the grant is revoked, absolutely expired or idle-expired;
5. persists deterministic terminal session/route/lease/grant state;
6. removes the MediaSession workspace through the normal runtime ownership path;
7. remains safe for active playback because successful media requests continue to refresh `lastSeenAt`.

No additional background thread is introduced, and process existence alone is not used as liveness authority.

## Real hard-disconnect acceptance

The final real yaVDR acceptance used MediaSession `ms_3b84162b43effb98a8dea5beee1efbac` and MediaAccessGrant `mg_a713748f63d63d0070025c68befb065b`.

First, normal active playback remained stable with the VAAPI FFmpeg worker and workspace present. After the client disappeared without a graceful stop, `last_seen_at` stopped advancing:

```text
2026-08-18 19:36:42
2026-08-18 19:36:42
```

Only that grant was then aged beyond the accepted 300-second idle boundary. On the next reaper cycle the server reached the required deterministic terminal state:

```text
FFMPEG: PASS: kein FFmpeg-Worker mehr
WORKSPACE: PASS: Workspace entfernt
SESSION: ended | media_access_idle_expired | 2026-08-18 19:38:34
GRANT: active=0 | revoked_at=2026-08-18 19:38:34
LEASE: ended | 2026-08-18 19:38:34
ROUTE: ended
```

This proves both required sides of the lifecycle contract: actively consumed media is not reaped, while an ungraceful client disappearance eventually releases worker, workspace and persisted MediaSession ownership.

Hosted CI run `32176309565` on the accepted implementation candidate `485c990c9f5692f00aa0e2e087967b236676c154` passed docs, architecture, Make/test audit, fast regressions including daemon build, packaging and frontend regressions.

## Closeout decision

**Phase 65.A existing-Recording playback is accepted and closed for its bounded scope.**

The accepted scope includes browser picture/sound playback, least-transformation adaptation, interlace handling, calibrated software and VAAPI transcode policy, stable HLS publication/buffering, graceful lifecycle cleanup, daemon recovery and server-owned idle cleanup after an ungraceful client disappearance.

### Later sequencing supersedes the original follow-on note

After this 65.A closeout, Phase 65.B Live-TV was completed. The later implementation history then defined and closed:

- **65.C — Recording startup / progressive-direct** through PR #206;
- **65.D — Media-transcode backend policy and output settings** through PR #208 and ADR-0055.

The earlier idea of making `Recording seek and growing-recording semantics` the 65.C product label is therefore obsolete. Its safety intent remains: Range/seek/growing capability must be truthful, and unsupported advanced seek must not be fabricated. Full arbitrary VOD time-seek, VDR-index mapping and user-visible growing-Recording seek remain deferred until a coherent demonstrated gap promotes them.

The current next Phase-65 vertical is maintained only in [Current State](../CURRENT.md) and the [Strict Roadmap](../planning/roadmap.md). Phase 66 remains blocked until the complete Phase-65 gate is satisfied.
