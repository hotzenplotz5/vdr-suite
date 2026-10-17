# Public API v1: missing authenticated Device change-event stream (2026-10-17)

**Status: DESIGN / NOT IMPLEMENTED.** This document describes the source-confirmed transport and authorization gap. It is not a new live API, an installed feature or Hisense acceptance.

## Operator requirement

A paired TV must learn of native VDR Timer and Recording changes through the existing VDR/Suite event chain, not a newly invented 15-second poller. Events are hints only; authorized canonical Public-v1 reads supply the data.

## Checked Web-to-Backend production path

1. VDR/SuiteBridge emits TimerChange, RecordingList, Recording and MarksModified callbacks. Existing daemon snapshots and the SnapshotChangeFeedService produce backend-scoped changed domains: timers, recordings, recordingMarks, events, searchtimers and liveOverlay. Do not create a parallel VDR event registry.
2. LiveTransportService forwards SnapshotChangeFeedEntry to SseLiveTransport, whose serializer writes named SSE update frames with sequenceNumber, snapshotGeneration, backendId and changedDomains.
3. LiveTransportController::getStream currently returns transport.stream() as one **finite** response body. This does not establish an open incremental HTTP subscription.
4. The Webfrontend opens EventSource('/api/vdr/live', {withCredentials:true}) via web/frontend/api/live-remote-client-api.js. Its recordings2-folder-refresh.js checks changed domains, sequence and backend before rereading canonical recording-folder data; it also has an existing 30-second **Browser-specific** fallback. That fallback must not be transplanted into VIDAA.
5. SecurityHttpGate::authenticate rejects Device Authorization headers outside /api/v1. EventSource cannot set a custom Device Authorization header. The paired VIDAA device may not connect to /api/vdr/live using its secret.
6. SimpleHttpListener writes normal API responses with Content-Length then closes the socket. Its existing streaming path is HttpServerResponse.streamBodyPath for *media FIFOs*, with four media stream workers. A shared FIFO is not a per-subscriber authenticated SSE fanout and must not be reused for this purpose.

## Missing Public-v1 contracts

**Scoped admission.** Any device event subscription needs a newly versioned and capability-advertised read resource. Authenticate active persistent Device identity and authorize the requested backend and domain: timers.view for timers, recordings.view for recordings/recordingMarks, epg.view for events. No inherited Admin grant, no channels.view substitute, no wildcard, no private/legacy URL. Recheck revocation during a long-lived stream, fail closed and record decisions in the existing security audit; do not log secrets.

**Real streaming.** Add a bounded, cancellable **per-subscriber** HTTP response transport with independent fanout from the existing SnapshotChangeFeedService/serializer. No fake repeated finite body, no reconnect-as-polling trick, no second event aggregator and no repurposed shared media FIFO. Bound subscribers, per-subscriber pending bytes, event ages and writer memory. Do not block the daemon or other TVs on a slow reader. On overflow, signal resync and terminate that subscription; never silently lose events.

**VIDAA transport.** Use an authenticated HTTP streaming mechanism *only after* confirming the actual Hisense engine supports it. If it cannot attach Device Authorization to a true stream, a separate reviewed short-lived, tightly scoped server-issued cookie/lease bootstrap could be considered, including revocation. Neither a device secret nor a lease goes in a URL, query, hash or log. Unsupported engines must explicitly report unavailable event updates, not silently poll.

**Sequence/resync.** A subscriber needs backend-filtered and grant-filtered domain hints, a daemon-instance epoch and monotonic cursor. Sequence gaps, dropped hints, restart, expired cursor, changed grants, backend switch and disconnect require one canonical resync read. Coalesce hints by domain, never replay write operations. Tear down on leaving the route, Device sign-out and server shutdown. Live session updates do not imply Timer create/update/delete authority.

**Canonical readbacks.** After an authorized timers hint, GET /api/v1/native-timers?backendId=<backend> (timers.view). After recordings hint, GET /api/v1/recordings?backendId=<backend> (recordings.view). GET /api/v1/timer-assignments?backend=<backend> is a distinct Suite assignment projection, not native VDR Timer CRUD. Changes can be missed during subscription setup; the first subscription activation must establish or resync a canonical snapshot.

## Required regression and hardware gates

- Invalid, anonymous, unpaired, revoked and backend-mismatched Device access is rejected; presenting Device credentials to legacy /api/vdr/live remains invalid.
- Two clients observing separate backend/grant scopes never see one another's domains; revocation closes an active reader.
- Native TimerChange and RecordingList/Recording/MarksModified notifications flow once through the existing change feed and trigger at most one coalesced authorized canonical GET (not a periodic GET).
- Independent subscriber fanout, backpressure, overload, disconnect, shutdown, sequence gap and daemon restart all have deterministic tests; no cross-client data leak or daemon stall.
- Verify the actual Hisense Device engine can retain its authenticated streaming connection and update native Timer/Recording UI. GitHub CI alone is insufficient.

**Current result:** No Public-v1 device event channel, incremental server writer or automatic VIDAA Timer/Recording updates are implemented by this note. The 15-second Timer poll was removed. DEVICE_SSE=UNIMPLEMENTED; DEPLOY=NOT_STARTED; TV_ACCEPTANCE=PENDING.

## Consumer-ready per-Subscription handshake (not implemented)

The prepared VIDAA client helper at hotzenplotz5/vdr-suite-vidaa:
app/src/device-live-refresh-coordinator.js is **not loaded or connected**.
It requires the following new *authorized* transport handshake:

- backendId: the single validated backend selected by the Device actor.
- daemonEpoch: one opaque, nonsecret identifier for this daemon process generation (8–128 safe token characters). A changed epoch invalidates the old subscription and requires reauthentication.
- subscriptionSequence: a nonnegative, monotonic **per-subscriber** cursor, assigned after backend and grant-domain filtering. Existing global SnapshotChangeFeedEntry.sequenceNumber cannot be substituted, because filtering another backend would create false gaps.
- authorizedDomains: a nonempty unique subset of timers and recordings, checked from the Device's persisted exact backend grants. recordingMarks is covered by recordings.view. Do not authorize the stream merely because a device is paired.

Every incremental update carries backendId, daemonEpoch, strictly increasing per-subscription subscriptionSequence and the grant-filtered changedDomains. Native recording and timer snapshots are always read afterwards from existing Public-v1 resources. The client initial handshake invokes a canonical read of granted domains, and sequence gaps invoke one coalesced resync. Cross-backend, ungranted-domain or epoch-changing frames close the client feed; a new authenticated handshake is required. The incremental HTTP writer, revocation checks, fanout, backpressure, new route and actual VIDAA streaming support are still **UNIMPLEMENTED**, so no client live-refresh claim is valid yet.

Focused tests of the prepared client helper cover eight cases, including a 400-frame irrelevant EPG burst and a 180-frame Timer burst; they do not validate the missing server-side authorization or real streaming transport.
