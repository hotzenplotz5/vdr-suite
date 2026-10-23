# FIX-LIVE-01 – Live-TV streaming lifecycle, first verified fix slice

Status: source candidate; productive long-running yaVDR acceptance **not performed**.

## Real-world evidence and causal limits

Operator-observed sessions on 2026-10-23 ended with `live_worker_exited`,
`backpressure_overflow`, `client_closed` and `channel_replaced`. The
FFmpeg log collection contained `Broken pipe` and transport corruption
warnings, but the individual FFmpeg files have not been correlated with
individual `media_sessions` rows. Neither DVB signal, FFmpeg corruption nor
Chrome background throttling is proven to be the exclusive original cause.

Verified in source on the pinned main baseline
`72a786af91e8763aea3c02de164f74aab5aec9f2`:

1. The continuous MSE reader previously called `endOfStream()` on a
   successful-but-final HTTP read (`done=true`) even for broadcast Live-TV.
   It had no owner failure callback for that condition, permitting frozen
   last-frame output with an obsolete "Live-TV läuft" status.
2. The HTTP streaming writer treated every send timeout
   (`EAGAIN/EWOULDBLOCK`) as infinitely retryable, even when the browser
   permanently stopped consuming bytes. Four bounded stream workers could
   remain occupied and propagate zero-progress backpressure to the FIFO.
3. The Live worker reaper recorded the generic `live_worker_exited` reason
   without emitting the underlying exit code or signal.
4. A `pagehide` handler treated BFCache suspension (`persisted=true`)
   as a permanent tab close.

## Changes

- MSE now distinguishes continuous Live EOF from legitimate completed
  recording/HbbTV media EOF. Live EOF enters an owner recovery path.
- A browser owner uses a 20-second no-frame-progress liveness threshold with
  five-second checks. Background tab checks defer while hidden; reactivation
  allows a grace check. Paused playback is not classified as stalled.
  JavaScript throttling can delay these checks; it cannot be prevented.
- Unexpected Live failures use at most three delayed retries (1/2/4 seconds)
  and preserve server-owned `replacesSessionId` STOP-old-before-OPEN-new
  semantics. The budget is reset by actual video-time progress, not merely a
  successful HTTP response. Errors indicating unsupported playback or denied
  authority fail closed. Manual stop, page close, HbbTV switch and channel
  handoff cancel retries. Late MSE callbacks are generation-fenced.
- `pagehide.persisted` no longer STOPs the live session. A real close keeps
  the keepalive stop request and destroys the browser owner.
- HTTP writer now has a 15-second *zero-send-progress* limit, not a total
  stream duration limit. Each successful send resets its budget. Terminal
  send failures log one reason per affected stream using the workspace
  identifier (no credentials).
- The daemon logs one exit code, signal or wait error per unexpectedly
  reaped Live worker, correlated by media session id and pid. The stable
  persisted terminal reason remains compatible.

## Regression and acceptance matrix

The `test_live_tv_eof_recovery.js` regression tests unexpected MSE EOF,
truthful status, BFCache suspension, sequential `replacesSessionId`,
cleanup of an old reader, explicit stop and bounded failed retries.
It is included in `test-ci-frontend` through the existing MSE test target.

The existing `test_live_tv_continuous_mse_backpressure.js` checks the
Live/Recording forward-buffer distinction. The full daemon build and related
media/SuiteBridge tests must run at the exact final PR head. A green CI
result demonstrates only source/test integration.

Not yet proven here: the initiating cause of the real `backpressure_overflow`
events, real Chrome tab freeze/resume behavior, long-term FFmpeg/TS health,
the exact installed yaVDR binary/source match, and native VDR stability.
The new send-progress limit is a bounded defense, not proof that its value
fits every client/network environment; observe real logs before tuning.

## Separate productive yaVDR acceptance (operator approval required)

Do not install or replace binaries, restart VDR/daemon, change productive
configuration/SQLite, or interrupt sessions without explicit permission.
Once authorized, first correlate session id, timestamp and worker pid with
FFmpeg logs, `media_stream_terminal` events and SuiteBridge reason.
Then perform at least 60 minutes active-tab Live TV, 60 minutes inactive-tab
Live TV where browser playback is allowed, tab resume, channel switches,
HbbTV transition, and separately approved failure/recovery tests.

Until those succeed, this slice must **not** be called productively stable.
