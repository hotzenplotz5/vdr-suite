# Native Device change events (Public API v1)

**Candidate implementation; not yet accepted on yaVDR/Hisense.**

The versioned resource is `GET /api/v1/events?backendId=<backendId>&domain=<domain>` where domain is exactly `timers` or `recordings`. It is an incremental HTTP/1.1 `text/event-stream` response, not a video stream, an EventSource URL with a secret, an administrative mutation, or a snapshot listing. The HTTP writer sends individual SSE frames without Content-Length and keeps the socket open. The daemon feeds the stream from the same `SnapshotChangeFeedService` → `LiveTransportService` → `SseLiveTransport` chain already used for Web.

## Request, identity and domain authorization

- Present the existing `Authorization: VDR-Suite-Device <credential-id>.<secret>` header, exclusively on HTTPS `/api/v1` and without browser-session cookies; never include a bearer secret or session lease in the URL.
- `SecurityHttpGate` requires a verified persistent Device Service actor, attached Device and credential, no browser session, one valid backendId and exactly one domain. A device with `timers.view` can subscribe to `timers` for that backend; `recordings.view` grants `recordings` (including the `recordingMarks` change hint). No grant is inferred from pairing, `channels.view`, live playback or an Admin account.
- Authorization and denial decisions use the existing accountability event journal; failed audit persistence rejects the subscription. An invalid backend/domain is rejected, and no event body is sent before authorization.
- Successful Device-grant, lifecycle or credential changes through the Public-v1 device mutation routes close existing subscriptions, requiring a fresh admission check. Each subscription also has a bounded two-minute lifetime as an upper bound on out-of-band revocation delay. This is a **reconnection/reauthentication** limit, not a Timer/Recording request poller; after reconnect the Device client performs a safe snapshot resync.

## Event wire protocol

The first frame is a `hello` handshake, after admission:

```text
event: hello
data: {"backendId":"home","daemonEpoch":"daemon_<random>","subscriptionSequence":0,"authorizedDomains":["timers"]}

```

Subsequent frames are `update` events with the same backend/daemonEpoch, strictly increasing **per-subscriber** subscriptionSequence starting at 1 and `changedDomains` filtered to the authorized domain. `recordingMarks` is normalized to the `recordings` event hint to request a full canonical recording snapshot. Source `SnapshotChangeFeedEntry.sequenceNumber` is *not* copied into the subscription sequence: updates about another VDR and ungranted domains must not produce spurious holes.

The daemon epoch is randomly minted once per runtime and changes after daemon restart. A queued subscriber has at most 64 pending messages; overflow clears the pending frames and sends `event: resync` before closing. The HTTP listener has a separate four-thread event-writer pool, at most four pending writer jobs, and keeps media FIFO workers independent. The hub limits active+queued subscriptions to eight. A slow reader receives bounded socket writes, heartbeats every 10 seconds, and is disconnected on send failure or termination. Shutdown terminates event writers.

## Authoritative client reads and reconnect semantics

After the `hello` handshake and on coalesced `timers` hints the Device client reads the existing `GET /api/v1/native-timers?backendId=<backend>&limit=30&offset=<page-offset>` resource with `timers.view` rights. On `recordings`/marks hints the Device reads its current authorized paginated Recording folder, preserving its folder and offset; the same `recordings.view` grant applies. The event payload never becomes timer/recording truth and may never initiate a native write.

On a gap, restart, overload or reconnect the Device coordinator resynchronizes the granted native snapshot once, preserving backend identity and cancelling stale route reads. The VIDAA client uses `fetch` with Device Authorization and `ReadableStream.getReader()`, not plain `EventSource` or `/api/vdr/live`. It fails closed if the target Hisense browser lacks Fetch streaming, TextDecoder or AbortController; no silent interval-polling alternative.

## Acceptance status

Automated JS transport and view integration regressions, plus the C++ hub and HTTP security/runtime build checks, are separate gates. The candidate is **not** considered production-ready until exact backend final-head CI, exact VIDAA tests, a test installation and actual Hisense runtime observation have passed. Nothing in this document asserts TV acceptance, installed yaVDR provenance or automatic timer update verification on real hardware.
