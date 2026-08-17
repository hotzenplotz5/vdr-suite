# Phase 65 Media Capability Negotiation Contract

Status: Active implementation contract for Phase 65.

Binding architecture:

- ADR-0046 — Streaming Gateway and Media Session Boundary
- ADR-0053 — Client Playback Engine and Media Adaptation Strategy

This document narrows those accepted decisions for the first Phase-65 vertical. It does not create a new architecture decision and does not change the later Live-TV, growing-recording, Broadcast Companion, Legacy OSD or public-API phases.

## Goal of the first vertical

The first product proof is one existing VDR Recording played through a VDR-Suite-controlled MediaSession in the browser with real picture and sound, followed by deterministic cleanup.

An HTTP 200 response, a provider URL, a raw filesystem path or a raw MPEG-TS byte dump alone is not acceptance.

## Source and client facts

The server derives a `MediaSourceDescriptor` from trusted backend/provider evidence. The client submits `ClientMediaCapabilities` as untrusted capability claims.

The initial typed source facts cover:

- resource kind: recording, live channel or growing recording;
- source container;
- video codec, dimensions, frame rate and HDR fact;
- audio codec, channel count and language;
- seekability and growing state.

The initial typed client facts cover:

- supported delivery protocols;
- supported containers;
- supported video and audio codecs;
- byte-range capability;
- maximum video dimensions.

Client capability values never become provider URLs, filesystem paths, shell fragments or raw FFmpeg arguments.

## Selection order

The selector uses the least transformation that satisfies both source facts and client capabilities:

1. `progressive-direct` when protocol, container, codecs and seek semantics are directly supported;
2. `hls-fmp4` as the preferred interoperable HLS packaging profile when supported;
3. `hls-ts` as an HLS compatibility packaging profile when needed;
4. selective track transcoding only for incompatible tracks;
5. no route when no allowlisted profile can satisfy the request.

Packaging and codec transformation are separate decisions. A typical browser path may therefore be:

```text
VDR Recording: MPEG-TS + H.264 + AC3
  -> HLS/fMP4 packaging
  -> H.264 copied
  -> AC3 transcoded to AAC
```

The video is not re-encoded merely because the audio track is incompatible.

Unknown source codecs fail closed instead of silently selecting a costly generic transcode.

## Multi-client rule

The browser is the first product validator, not the architecture authority.

The same source/session model must remain usable by native clients such as Android/Android TV, Apple platforms, Windows, Kodi-style clients and television runtimes. Platform-specific assumptions belong in client capability adapters, not in the provider or Recording model.

No server rule is keyed to a browser brand, operating-system name, television vendor or user-agent string when an explicit media capability can express the requirement.

## Lessons from existing VDR implementations

### vdr-live

`vdr-live` demonstrates two useful implementation facts:

- VDR recordings can be traversed segment-by-segment without materializing the entire recording in memory;
- HLS plus FFmpeg can provide practical browser playback.

VDR-Suite does not copy its public architecture. In particular, Phase 65 does not bind playback to a web-session temp directory, a shell-built FFmpeg command, a Streamdev URL, a native recording path or a browser-specific player contract.

### Streamdev

Streamdev remains a possible private `StreamProvider`. Its URL, credentials and protocol details are never the public VDR-Suite playback contract.

### Kodi/VNSI

Kodi/VNSI demonstrates the useful separation between a seekable server-side Recording stream/session and client-owned playback/demux. VDR-Suite keeps that separation while using its own MediaSession, route, permission and gateway contracts rather than exposing VNSI as the platform API.

## Media access and credential rule

ADR-0046 remains binding: the initial contract does not use a URL-only bearer credential.

Browser media delivery uses a same-origin protected media context with a short-lived, scoped credential such as a Secure/HttpOnly media cookie. Native clients use an authorization header or equivalent protected protocol metadata.

A `mediaSessionId`, provider URL, recording path, backend credential or user login session identifier is never a public media credential.

If a future television or external-player runtime proves that it cannot use headers or scoped cookies, URL-only compatibility requires a separate explicit architecture decision and leakage/replay analysis. It is not introduced implicitly for convenience.

## Adaptation worker rule

When remuxing or transcoding is required, the initial implementation may use FFmpeg/ffprobe as private workers, but only under these rules:

- no `/bin/sh -c`;
- no client-controlled raw command arguments;
- fixed allowlisted argv construction from typed profiles;
- deterministic child-process ownership by the MediaSession/ProviderStreamLease;
- bounded private working directories;
- explicit stop, reap and cleanup;
- provider credentials and internal paths remain private;
- capacity and later resource policy can deny expensive transformations.

The preferred transformation order remains:

```text
pass-through -> remux/repackage -> selective transcode -> full transcode only when unavoidable
```

## First vertical boundary

Included:

- Recording source resolution by stable Suite identity;
- typed source probing;
- typed client capability input;
- deterministic presentation selection;
- local Recording provider lease;
- MediaSession and route identity;
- scoped media access context;
- Streaming Gateway delivery;
- browser playback with real picture and sound;
- deterministic stop/cleanup;
- real yaVDR acceptance on the exact candidate commit.

Deferred from this vertical:

- Live-TV tuner allocation and channel switching;
- growing-recording seek semantics;
- remote-site route migration;
- general transcode farm/scheduler;
- Teletext/HbbTV;
- Legacy OSD;
- broad Timer UI;
- public API compatibility freeze.

## Acceptance gate

The exact candidate must prove on a real yaVDR installation:

1. an authenticated actor selects an existing Suite Recording;
2. no native recording path or provider URL is exposed to the browser;
3. a MediaSession reaches a usable route;
4. the browser presents real video and audible audio;
5. the selected adaptation matches the source/client evidence and does not perform unnecessary track transcoding;
6. stopping playback terminates/revokes the session and provider lease;
7. any adaptation worker is reaped and its private temporary state is removed;
8. expired/revoked media access is rejected;
9. existing control-plane, Agent and Phase-64 timer safety contracts remain green.
