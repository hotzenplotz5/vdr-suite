# Phase 65.C Recording startup / progressive-direct vertical

Status: implementation candidate; real yaVDR acceptance required before closeout.

## Scope

This document records the first Phase-65.C product/safety vertical for existing-recording startup performance. It does not close Phase 65.C and it does not authorize Phase 66. Phase 65.B Live-TV remains closed and is intentionally unchanged.

The vertical reuses ADR-0046 and ADR-0053. No new streaming protocol or architecture authority is introduced.

## Baseline hot path on main

Baseline commit: `5ca47b95d09872f446b7b8858842ff8e637b17ba`.

A recording Play request currently traverses these stages synchronously:

1. resolve the public Suite recording identity to the private local VDR recording directory and ordered segment set;
2. create a probe workspace and run `ffprobe`; the process timeout is 10 seconds;
3. select a presentation from typed `ClientMediaCapabilities`;
4. issue the Suite MediaSession, route lease and MediaAccessGrant;
5. for the accepted browser path, prepare an HLS workspace and start FFmpeg;
6. wait until `master.m3u8` plus `init.mp4` and the first media segment exist; the readiness timeout is 15 seconds;
7. the browser MSE owner waits until at least 12 seconds of fMP4 media have been accumulated before playback begins.

The 10-second probe timeout and 15-second HLS-readiness timeout are safety ceilings, not measured startup costs. Two structural latency sources are more important for the existing browser HLS path: recording FFmpeg input is paced with `-re`, HLS segments are 4 seconds long, and the browser deliberately waits for a 12-second startup buffer. Those values remain unchanged in this vertical until real before/after measurements exist.

## Source truth and descriptor reuse

Completed recordings do not have a trustworthy persisted codec descriptor in the current VDR recording cache. The first vertical therefore uses a narrow in-process descriptor cache instead of adding a broad schema migration.

A cached descriptor is reusable only when all of the following still match:

- Suite backend and recording identity;
- the exact private recording directory;
- ordered VDR segment names;
- each segment device/inode identity;
- each segment byte length;
- each segment modification timestamp;
- completed/growing state.

The VDR `.timer` recording marker is treated conservatively as growing evidence. A growing recording is never inserted into the completed-descriptor cache. Any fingerprint change causes a cache miss and a fresh probe. No stale codec facts are reused blindly.

The 10-second `ffprobe` timeout is retained as a failure bound. A first play may still probe; repeated play of an unchanged completed recording can reuse the descriptor. More importantly, a compatible completed recording no longer needs to wait for an HLS worker after presentation selection.

## Progressive-direct delivery contract

`progressive-direct` is provisioned only when all of these conditions hold:

- the recording is completed rather than growing;
- the source is a modern VDR MPEG-TS segment set (`00001.ts`, `00002.ts`, ...);
- the typed client capabilities include Progressive delivery, MPEG-TS, compatible selected codecs and byte-range support;
- `MediaPresentationSelector` chooses the existing pass-through `progressive-direct` profile.

Legacy `.vdr` recordings and growing recordings keep the existing adaptation fallback. No native recording directory or segment path is returned to a client.

The public media route is Suite-owned:

`/api/media/sessions/<session-id>/recording/stream.ts`

It is protected by the same MediaSession / MediaAccessGrant / route-lease checks as HLS and Live-TV media routes.

The first direct gateway contract is deliberately range-only. A request must contain one valid HTTP `Range: bytes=...` interval. The logical VDR segment set is read as one byte stream. Reads may cross native segment boundaries but each gateway response is capped by `SegmentedRecordingByteSource::DefaultMaximumReadBytes` (512 KiB). A larger requested interval may therefore be satisfied by a smaller valid `206 Partial Content` response; the returned `Content-Range` describes the bytes actually delivered.

Completed direct sources are revalidated against their source fingerprint before and after reads. If the source begins growing or any registered segment identity/size/mtime changes, the route fails closed instead of continuing with a stale length or seek guarantee.

Growing recordings are not assigned immutable direct `Content-Length`, EOF or byte-seek semantics by this vertical.

## Cleanup and authorization

Direct sessions are owned by `RecordingMediaSessionRuntime` just like HLS sessions. Stop, grant idle expiry, daemon shutdown and failure paths remove the direct-source registration and end the MediaSession bundle. Every media request re-enters MediaAccessGrant authentication and the active route lease.

No permanent bearer URL is introduced. Credentials remain in the existing same-origin cookie/header transport and never appear in media URLs.

## Startup measurement

The server records one stage breakdown for each successful recording MediaSession creation:

- `source_ms` — request parsing, recording identity lookup and source resolution;
- `descriptor_ms` — descriptor-cache lookup or probe workspace + ffprobe;
- `selection_ms` — capability-based presentation selection;
- `session_ms` — MediaSession/grant issuance and credential transport preparation;
- `provision_ms` — direct registration or HLS worker/readiness;
- `total_server_ms` — request start through server-ready media session.

Real product acceptance must additionally measure click-to-first-picture/audio in the browser. The server timings alone are not a substitute for visible playback timing.

## Required yaVDR acceptance before closeout

On the exact candidate commit, compare the same completed recording before and after the vertical and record at least:

- click/Play request to MediaSession response;
- server startup stage log;
- click to first visible picture and audible audio;
- selected presentation profile;
- direct `206` range response and bounded response size when direct is selected;
- seek behavior only where the concrete client/source path supports it;
- fallback playback for an incompatible recording/client capability set;
- stop/close cleanup and absence of stale direct registrations/workers;
- no public native VDR path leakage.

A growing recording must additionally prove that progressive-direct is not selected and that no immutable range/EOF guarantee is advertised.

Do not reduce the existing 12-second HLS startup/rebuffer buffer or change the 4-second HLS segment policy in response to source inspection alone. Any later tuning requires before/after real yaVDR evidence.
