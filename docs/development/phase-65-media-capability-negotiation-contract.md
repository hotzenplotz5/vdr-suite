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
- maximum video dimensions;
- maximum supported audio channel count for the selected playback path.

Client capability values never become provider URLs, filesystem paths, shell fragments or raw FFmpeg arguments. Numeric media limits are parsed into bounded typed values before they can influence a presentation profile.

## Recording source contract

A VDR Recording is not modeled as one public native path and not as one HTTP URL. The local provider owns its native Recording representation and exposes a lease-scoped logical source.

For segmented VDR recordings, all current media segments form one logical byte address space:

```text
00001.ts  [0 ........ a)
00002.ts  [a ........ b)
00003.ts  [b ........ c)
                 ...
```

The provider source contract supports:

- current readable extent;
- bounded positioned reads `(offset, maximumLength)`;
- reads that cross native segment boundaries;
- explicit end-of-current-readable-extent;
- refresh of the current segment catalog/extent;
- later integration with VDR index evidence for normalized seek/time mapping;
- fail-closed behavior when a segment disappears, changes into a non-regular file, or cannot be opened safely.

The first implementation bounds one source read to 512 KiB. A caller asking for a larger range does not cause an unbounded allocation or read.

For a growing Recording, the provider may observe additional native segments on a later extent refresh. This capability is established now so the source abstraction does not have to be replaced later, but user-visible growing-recording seek remains deferred from the first vertical.

Native segment paths are provider-internal facts and never appear in the public MediaSession or Gateway contract.

## Track selection contract

A source may contain multiple video, audio or later subtitle tracks. Compatibility is evaluated per candidate track, not by requiring every source track to be playable by the client.

A selected `MediaPresentationProfile` records the exact selected source video/audio track indexes and an action for each selected track:

```text
copy | transcode | omit
```

For example, a Recording with German AC3 plus English AAC does not require audio transcoding for an AAC-capable client merely because the first audio track is AC3. The selector may choose the compatible AAC track.

Audio compatibility includes both codec and the client's typed maximum channel count. A codec-compatible track whose channel count exceeds the client limit is not eligible for copy. When AAC is the allowed target codec, the selector may keep the video copied while transcoding only the selected audio track and constraining its target channel count. The current browser MSE/fMP4 adapter advertises a two-channel limit because real Edge acceptance showed AAC 5.1 can be valid as ordinary MP4 while the same initialization segment fails in the MSE SourceBuffer path.

Video source validity and client output limits are separate facts. A known decodable source video track is not rejected merely because its source dimensions exceed the client limit. Direct copy still requires the source dimensions to fit the client capability, but an allowlisted transcode path derives a concrete target width and height that fit within the typed client bounds, preserve the source aspect ratio, never upscale, and use even dimensions suitable for the H.264 target. This permits, for example, a trusted HEVC 3840x2160 Recording source to become an H.264 1920x1080 browser presentation without pretending that the browser natively accepts the 4K source.

Track selection is deterministic from trusted source descriptors and typed client capabilities. Clients do not inject raw FFmpeg stream selectors, scale expressions or arbitrary audio-channel command arguments.

## Selection order

The selector uses the least transformation that satisfies both source facts and client capabilities:

1. `progressive-direct` when protocol, container, selected codecs, channel limits and seek semantics are directly supported;
2. `hls-fmp4` as the preferred interoperable HLS packaging profile when supported;
3. `hls-ts` as an HLS compatibility packaging profile when needed;
4. selective track transcoding only for incompatible selected tracks;
5. no route when no allowlisted profile can satisfy the request.

Packaging and codec transformation are separate decisions. A typical browser path may therefore be:

```text
VDR Recording: MPEG-TS + H.264 + AC3 5.1
  -> HLS/fMP4 packaging
  -> H.264 copied
  -> AC3 transcoded to AAC stereo
```

A high-resolution incompatible source may instead require both selected tracks to transform:

```text
VDR Recording: MPEG-TS + HEVC Main 10 3840x2160 + AAC 5.1
  -> HLS/fMP4 packaging
  -> HEVC transcoded to H.264 1920x1080 yuv420p
  -> AAC downmixed/transcoded to AAC stereo
```

The video is not re-encoded merely because the selected audio track is incompatible.

Unknown source codecs fail closed instead of silently selecting a costly generic transcode.

## Multi-client rule

The browser is the first product validator, not the architecture authority.

The same source/session model must remain usable by native clients such as Android/Android TV, Apple platforms, Windows, Kodi-style clients and television runtimes. Platform-specific assumptions belong in client capability adapters, not in the provider or Recording model.

No server rule is keyed to a browser brand, operating-system name, television vendor or user-agent string when an explicit media capability can express the requirement.

HLS/fMP4 (CMAF-style fragmented MP4 packaging where supported by the worker/profile) is a strong interoperable adaptation profile, not the universal VDR-Suite media protocol. A native client that can consume the source container/codecs directly should remain eligible for a cheaper progressive/pass-through profile.

## Lessons from existing VDR implementations

### vdr-live

`vdr-live` demonstrates useful practical facts: FFmpeg/HLS can make VDR media browser-consumable and an input can be paced for playback. Its implementation also demonstrates failure modes VDR-Suite intentionally avoids: provider URLs in the web path, shell-oriented process composition, broad default transcoding and session-local HLS materialization as the playback architecture.

VDR-Suite does not bind playback to a web-session temp directory, a shell-built FFmpeg command, a Streamdev URL, a native recording path or a browser-specific player contract.

### Streamdev

Streamdev remains a possible private `StreamProvider`, especially for later Live TV. Its URL, credentials, tuner/priority behavior and protocol details are never the public VDR-Suite playback contract.

### Kodi/VNSI

Kodi/VNSI demonstrates the useful separation between a seekable server-side Recording stream/session and client-owned playback/demux. Its Recording flow also supplies an important source-semantic reference: open, bounded positioned read, current length, seek, refresh length for a growing Recording, and close.

The VNSI server's treatment of multiple native VDR recording files as one logical byte address space is adopted as a provider-internal semantic, while VDR-Suite keeps its own MediaSession, route, permission, grant and Gateway contracts rather than exposing VNSI as the platform API.

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
- exact selected source tracks are mapped from the typed presentation profile;
- deterministic child-process ownership by the MediaSession/ProviderStreamLease;
- bounded private working directories;
- explicit stop, reap and cleanup;
- provider credentials and internal paths remain private;
- capacity and later resource policy can deny expensive transformations.

For audio transcoding, a positive `targetAudioChannels` is emitted only from the internally selected typed profile and only after the command builder has bounded the value. Copy paths never receive `-ac`.

For H.264 video transcoding, `targetVideoWidth` and `targetVideoHeight` are concrete bounded even integers produced by the selector. The command builder rejects invalid dimensions and emits only the fixed typed scale operation. The first browser target also normalizes transcoded output to `yuv420p`, so high-bit-depth HEVC input does not silently become a high-bit-depth H.264 presentation outside the advertised browser capability. Video copy paths receive neither scaling nor pixel-format conversion.

The preferred transformation order remains:

```text
pass-through -> remux/repackage -> selective transcode -> full transcode only when unavoidable
```

### First-browser HLS bound

The first browser proof does not yet promise arbitrary Recording seek. Its HLS worker therefore uses a paced input and a bounded sliding output window rather than racing through and materializing the whole Recording into a temporary session directory.

The initial worker contract uses 4-second target segments, an 8-entry playlist window, a small delete threshold, deletion of old segments, independent-segment signaling and temporary-file publication. It explicitly does not use an unbounded `event` playlist.

This bounded first profile is not the final VOD seek design. Later seek is derived from the logical Recording source plus VDR/index evidence and explicit MediaSession seek semantics, not from retaining an unlimited temporary HLS copy of the Recording.

## First vertical boundary

Included:

- Recording source resolution by stable Suite identity;
- typed source probing;
- logical segmented Recording byte source with bounded reads;
- typed client capability input;
- deterministic track-aware presentation selection;
- local Recording provider lease;
- MediaSession and route identity;
- scoped media access context;
- Streaming Gateway delivery;
- bounded browser adaptation when direct playback is not viable;
- browser playback with real picture and sound;
- deterministic stop/cleanup;
- real yaVDR acceptance on the exact candidate commit.

Deferred from this vertical:

- Live-TV tuner allocation and channel switching;
- user-visible growing-recording seek semantics;
- arbitrary VOD seek and VDR-index time mapping;
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
6. multi-track input can select a compatible track without gratuitous transcoding;
7. the first HLS adaptation cannot grow its temporary workspace without bound merely because the Recording is long;
8. stopping playback terminates/revokes the session and provider lease;
9. any adaptation worker is reaped and its private temporary state is removed;
10. expired/revoked media access is rejected;
11. existing control-plane, Agent and Phase-64 timer safety contracts remain green.
