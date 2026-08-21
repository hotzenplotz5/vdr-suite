# Phase 65.C Recording startup / progressive-direct vertical

Status: **Accepted and closed for the bounded Phase-65.C Recording startup/performance scope.**

## Scope

This document records the accepted Phase-65.C product/safety vertical for existing-recording startup performance. Phase 65.C is **not** the old planning label `Recording seek and growing-recording semantics`; that label is superseded by the implementation history. The vertical was accepted through PR #206 and does not authorize Phase 66. Phase 65.B Live-TV remains closed and is intentionally unchanged.

The vertical reuses ADR-0046 and ADR-0053. No new streaming protocol or architecture authority is introduced.

Truthful seek/range/growing capability remains a hard Phase-65 invariant: unsupported functionality is reported as unsupported. This vertical does not claim arbitrary VOD time-seek, VDR-index time mapping or user-visible growing-Recording seek.

## Baseline hot path on main

Baseline commit: `5ca47b95d09872f446b7b8858842ff8e637b17ba`.

A recording Play request on that baseline traverses these stages synchronously:

1. resolve the public Suite recording identity to the private local VDR recording directory and ordered segment set;
2. create a probe workspace and run `ffprobe`; the process timeout is 10 seconds;
3. select a presentation from typed `ClientMediaCapabilities`;
4. issue the Suite MediaSession, route lease and MediaAccessGrant;
5. for the accepted browser path, prepare an HLS workspace and start FFmpeg;
6. wait until `master.m3u8` plus `init.mp4` and the first media segment exist; the readiness timeout is 15 seconds;
7. the browser MSE owner waits until at least 12 seconds of fMP4 media have been accumulated before playback begins.

The 10-second probe timeout and 15-second HLS-readiness timeout are safety ceilings, not measured startup costs. The structural latency sources in the accepted browser HLS path are more important: Recording FFmpeg input is paced with `-re`, HLS segments are four seconds long, and the browser deliberately waits for a 12-second startup buffer.

The optimization therefore does not merely shrink those timeout constants. It removes the HLS startup pipeline from the normal compatible completed-Recording browser path while retaining HLS as a compatibility fallback.

## Source truth and descriptor reuse

Completed recordings do not have a trustworthy persisted codec descriptor in the current VDR recording cache. This vertical therefore uses a narrow in-process descriptor cache instead of adding a broad schema migration.

A cached descriptor is reusable only when all of the following still match:

- Suite backend and recording identity;
- the exact private recording directory;
- ordered VDR segment names;
- each segment device/inode identity;
- each segment byte length;
- each segment modification timestamp;
- completed/growing state.

The VDR `.timer` recording marker is treated conservatively as growing evidence. A growing recording is never inserted into the completed-descriptor cache. Any fingerprint change causes a cache miss and a fresh probe. No stale codec facts are reused blindly.

The 10-second `ffprobe` timeout is retained as a failure bound. A first play may still probe; repeated play of an unchanged completed recording can reuse the descriptor.

## Delivery ordering

The selector keeps ADR-0053's least-transformation order and uses only typed client capabilities:

1. `progressive-direct` — original MPEG-TS bytes when the client can consume the source container/codecs and truthful byte ranges;
2. `progressive-fmp4` — continuous fragmented-MP4 adaptation when the client consumes fMP4 but not the native MPEG-TS source;
3. HLS — existing compatibility fallback when the continuous path is not valid;
4. within either adaptation path, transcode only the selected incompatible tracks.

No User-Agent, browser name, operating system or device-vendor branch is introduced.

### Native progressive-direct

`progressive-direct` is provisioned only when all of these conditions hold:

- the recording is completed rather than growing;
- the source is a modern VDR MPEG-TS segment set (`00001.ts`, `00002.ts`, ...);
- the typed client capabilities include Progressive delivery, MPEG-TS, compatible selected codecs and byte-range support;
- `MediaPresentationSelector` chooses the pass-through `progressive-direct` profile.

The public media route is Suite-owned:

`/api/media/sessions/<session-id>/recording/stream.ts`

It is protected by the same MediaSession / MediaAccessGrant / route-lease checks as the other media routes.

The direct gateway contract is deliberately range-only. A request must contain one valid HTTP `Range: bytes=...` interval. The logical VDR segment set is read as one byte stream. Reads may cross native segment boundaries but each gateway response is capped by `SegmentedRecordingByteSource::DefaultMaximumReadBytes` (512 KiB). A larger requested interval may therefore be satisfied by a smaller valid `206 Partial Content` response; the returned `Content-Range` describes the bytes actually delivered.

Completed direct sources are revalidated against their source fingerprint before and after reads. If the source begins growing or any registered segment identity/size/mtime changes, the route fails closed instead of continuing with a stale length or seek guarantee.

### Low-latency browser progressive-fmp4

A completed modern Recording whose typed client capabilities advertise Progressive delivery plus fMP4 can instead receive:

`/api/media/sessions/<session-id>/recording/stream.mp4`

This path intentionally mirrors the already accepted low-latency Live-TV browser transport at the HTTP/media boundary while retaining Recording-specific source ownership.

The server prepares the private ordered Recording segment set through the existing workspace/ffconcat boundary and starts one FFmpeg worker that writes fragmented MP4 to a private session FIFO. For this completed-Recording path the worker does **not** use `-re`: the HTTP reader and browser provide natural FIFO backpressure, so the producer cannot race arbitrarily ahead of the consumer. This removes the four-second HLS publication cadence and the browser's 12-second MSE startup gate from the normal fast path.

The fMP4 worker uses small fragments and immediate packet flushing. Compatible selected tracks are copied; only incompatible selected tracks are transcoded. Typical examples are:

- H.264 + AAC -> fMP4 remux, no media transcode;
- H.264 + AC3 -> H.264 copy plus AAC audio transcode;
- MPEG-2/HEVC or interlaced video -> H.264 adaptation according to the existing calibrated transcode policy.

The progressive-fMP4 FIFO is a continuous delivery stream, not an immutable byte-addressable representation of the original Recording. The Gateway therefore does **not** advertise `Accept-Ranges`, `Content-Range` or a synthetic immutable `Content-Length` for it. This vertical does not pretend that browser time-seek is already implemented on that path.

Growing and unsupported Recordings fail out of the fast progressive request and retain the accepted HLS fallback. The browser does not decide completed/growing state itself; server source truth remains authoritative.

## Browser ownership and fallback

The browser advertises the low-latency Recording request through typed capabilities only:

- Progressive delivery;
- fMP4;
- H.264;
- AAC;
- no byte-range guarantee for the continuous stream.

A successful `progressive-fmp4` MediaSession is assigned directly to the native `<video>` element, like the accepted Live-TV direct stream. It does not instantiate the Recording HLS/MSE startup-buffer machinery.

The existing HLS Recording player remains intact as an automatic fallback. A failed or unavailable fast-path MediaSession therefore does not require weakening source truth, removing HLS, or special-casing a browser family.

## Cleanup, liveness and authorization

Direct, HLS and continuous Recording sessions remain owned by `RecordingMediaSessionRuntime`. Explicit stop and daemon shutdown terminate any owned FFmpeg worker, remove the private workspace/FIFO or direct-source registration, and end the MediaSession bundle.

HLS and native range-direct requests keep the existing request-idle expiry contract because their clients make repeated authenticated media requests. The continuous `progressive-fmp4` response is different: it is one long authenticated GET and therefore has no polling cadence with which to refresh `last_seen_at`. Its runtime disables only MediaAccessGrant **idle** expiry while that continuous worker is owned. Explicit grant revocation and the absolute grant expiry remain enforced.

Continuous-stream transport liveness is fenced by the FFmpeg worker. The runtime checks the child process during the normal media-session reap cycle; browser disconnect/HTTP reader loss causes the FIFO writer to terminate, and the exited worker then closes the MediaSession bundle and workspace. This prevents a valid long playback from being killed at the ordinary 300-second request-idle boundary without turning the stream into an unbounded grant.

Every media request re-enters MediaAccessGrant authentication and the active route lease. No permanent bearer URL is introduced. Credentials remain in the existing same-origin cookie/header transport and never appear in media URLs.

## Startup measurement

The server records one stage breakdown for each successful Recording MediaSession creation:

- `source_ms` — request parsing, Recording identity lookup and source resolution;
- `descriptor_ms` — descriptor-cache lookup or probe workspace + ffprobe;
- `selection_ms` — capability-based presentation selection;
- `session_ms` — MediaSession/grant issuance and credential transport preparation;
- `provision_ms` — direct registration, progressive FIFO worker provisioning or HLS worker/readiness;
- `total_server_ms` — request start through server-ready MediaSession.

The fast browser owner additionally measures the user Play action to the `<video>` `playing` event and reports the value as `recording playback first-media ... startupMs=...` evidence in the browser console/status. Real picture and audible audio remain mandatory acceptance observations; a JavaScript event alone is not sufficient closeout evidence.

## Acceptance contract

The implementation contract requires the fast path to preserve these truths:

- selected presentation profile is capability-driven;
- `progressive-fmp4` runs without the old HLS readiness/startup-buffer dependency and without `-re` pacing;
- `progressive-direct` keeps bounded truthful byte-range semantics when selected;
- continuous fMP4 does not advertise fake Range/Content-Length semantics;
- disconnect/stop cleanup does not strand workers/workspaces/direct registrations;
- incompatible or growing sources do not get a false immutable completed-Recording fast path;
- provider-native VDR paths remain private.

The existing 12-second HLS startup/rebuffer threshold and four-second HLS segment policy remain unchanged because they now belong to the compatibility fallback rather than being blindly retuned. Any later HLS tuning requires separate real evidence.

## Accepted closeout evidence

PR #206 merged the bounded Phase-65.C Recording startup/performance vertical after exact-head GitHub CI and real yaVDR acceptance.

```text
accepted_65c_candidate=51de13337edd0a072308a9df1bad6e245a764ac2
source_ci_workflow=VDR-Suite CI
source_ci_run_number=7972
source_ci_run_id=32350815560
source_ci_result=PASS
merge_pr=206
merge_commit=0513edf6166e096aa60cf313b74a43073cacd786
```

Real yaVDR acceptance on the exact candidate established the user-visible closeout result recorded by PR #206:

- the Recording playback control was present after the frontend runtime-wiring correction;
- a representative completed Recording started without the previously visible startup delay;
- picture and sound were clean;
- the previous startup artifacts were absent.

The closeout does not convert unimplemented time-seek or growing-Recording seek into implicit support. The retained safety rule is explicit capability truthfulness: advanced seek may remain unsupported until a later coherent implementation is justified.
