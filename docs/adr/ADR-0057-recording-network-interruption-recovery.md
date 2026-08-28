# ADR-0057: Recording Network Interruption Recovery

## Status

**Accepted**

Date: 2026-08-28

## Context

Phase 65.D Slice 4 established classified playback failures without allowing classification itself to trigger hidden recovery. Real browser acceptance on the accepted Slice-4 candidate demonstrated the remaining product behavior clearly: a completed Recording playing through the normal `progressive-fmp4` fast path becomes terminally stopped when the browser loses network connectivity after playback has already started, and playback does not continue automatically when connectivity returns.

That behavior is safe, but it is not the preferred product behavior for a transient client network interruption. A short Wi-Fi/mobile transition or temporary loss of connectivity should not force the user to manually reopen a long Recording when the Suite can recover through the already-authoritative playback owner.

The first ADR-0057 implementation candidate exposed an additional real-browser fact: Android/Edge connectivity hints are not a reliable authority for this policy. A long outage could produce the canonical post-start `client_media_network_error` while the browser did not provide the `offline` evidence required by the first implementation. Short outages could meanwhile be hidden by existing media buffering and resume without exercising recovery at all.

A later real Android/Edge acceptance run exposed a second, distinct transport behavior and reproduced it with a second Recording. After the existing media buffer drained, the HTML media element entered `waiting`, while the continuous fMP4 streaming reader did not reject and the platform did not produce a `MediaError`. The user-visible timeline stopped progressing, but the canonical owner still appeared logically `playing`, so no classified failure was published and ADR-0057 never armed. Restoring the network did not revive that stalled reader automatically.

The same real-browser run also exposed a recovery sequencing race. The ordinary owner start path started `video.play()` immediately, while ADR-0057 then called `pause()` before the authoritative seek. On a real browser the original play promise may still be pending, producing `The play() request was interrupted by a call to pause().` Test DOM mocks that resolve `play()` immediately do not model that race faithfully.

ADR-0056 already defines `recoveryClass` as descriptive policy evidence rather than an imperative and allows another presentation/session only through the canonical owner and an independently truthful authorized contract. This ADR defines the bounded policy for the demonstrated recovery cases without making browser connectivity hints or a raw `waiting` event authoritative.

## Decision

VDR-Suite will support **owner-driven automatic continuation of a completed Recording after a demonstrated transient browser network interruption**.

Conceptually the recovery policy moves through:

```text
playing
  -> interrupted
  -> recovering
  -> playing
```

If recovery cannot be completed truthfully:

```text
recovering
  -> stopped
```

These `interrupted` / `recovering` labels describe the bounded recovery policy and UI, not a second lifecycle authority. The canonical ADR-0056 owner remains authoritative for MediaSession identity and its existing stop/start/seek/play lifecycle publication throughout.

The same persistent first-party playback owner remains authoritative. Recovery must not introduce another playback controller, provider-selection path or transport owner.

Browser `online` / `offline` state is advisory only. A canonical classified post-start network failure arms the policy. For a continuous fMP4 stream that becomes indefinitely stalled without an explicit browser error, the canonical owner may first derive such a transport failure only from bounded post-start liveness evidence defined below. A separate read-only same-origin reachability probe then determines when the Suite origin is reachable enough to attempt a new authorized Recording session.

## Initial bounded scope

The first implementation is intentionally narrow:

- completed Recording;
- normal `progressive-fmp4` browser path;
- playback has already produced real media (`firstMediaReported`);
- the canonical owner publishes a classified post-start transport/platform network failure, either from an explicit transport/platform error or from the bounded liveness rule below;
- browser `online` / `offline` events may accelerate UI/probing but are not required evidence;
- recovery starts only after a same-origin Suite reachability request receives an HTTP response;
- decoder, codec, source, authorization, buffer and adaptation failures remain terminal;
- startup failure before first media keeps the already accepted compatibility-fallback policy and is not changed by this ADR.

An `offline` browser event by itself is not enough to tear down healthy buffered playback. Likewise, a `waiting` event by itself is not a playback failure: ordinary short buffering is expected and must not create a new MediaSession.

Conversely, a missing `offline` event and a browser streaming reader that remains pending must not block recovery forever after the buffer is exhausted and the Suite origin is demonstrably unreachable.

## Post-start stall and liveness evidence

The continuous fMP4 owner may use a bounded liveness check only after real media has already played. The initial policy is:

1. observe post-first-media `waiting` while the owner is otherwise still actively playing;
2. retain the canonical absolute Recording position at the start of that wait;
3. allow a bounded grace interval for normal buffering;
4. cancel the check if playback position advances, playback resumes, the user pauses, a seek begins, playback ends, or another canonical failure occurs;
5. after the grace interval, if the absolute position still has not meaningfully advanced, issue one read-only same-origin liveness probe to the existing VDR-Suite health endpoint;
6. only when that probe cannot reach the Suite origin and the presentation is still stalled may the canonical owner publish a classified post-start client transport failure.

The initial grace interval is eight seconds. This is not an eight-second network retry policy and does not create or replace any MediaSession. Its sole purpose is to avoid interpreting ordinary short buffering as a network interruption while still detecting the real browser condition in which `reader.read()` can remain pending indefinitely.

If the same-origin liveness probe receives any HTTP response, the owner does **not** infer a network interruption from `waiting`. A reachable-origin playback stall remains outside ADR-0057 automatic network recovery and must be diagnosed by its truthful underlying decoder/buffer/source semantics if it later fails.

The liveness probe uses:

```text
GET /api/vdr/health
credentials: same-origin
cache: no-store
X-VDR-Suite-Playback-Liveness-Probe: 1
```

This request is observational only. Its body and HTTP success status are not reused as capability, provider, authorization, VDR-health or playback truth.

## Recovery reachability evidence

After a canonical eligible network failure has armed ADR-0057, the recovery policy may issue a separate read-only same-origin request to the existing VDR-Suite health endpoint solely to distinguish `origin unreachable` from `origin reachable enough to attempt authorization`.

The recovery probe uses:

```text
GET /api/vdr/health
credentials: same-origin
cache: no-store
X-VDR-Suite-Recovery-Probe: 1
```

Any completed HTTP response proves same-origin transport reachability for this purpose. The health payload or HTTP success status is not reused as playback capability, provider, authorization or VDR-health authority.

A failed fetch means only that the Suite origin is not currently reachable. While armed, the policy may repeat this observational recovery probe at a bounded cadence. These probes are not MediaSession creation attempts and do not change playback lifecycle authority.

The pre-failure liveness probe and the post-failure recovery reachability probe therefore answer different questions. The former decides whether a long post-start `waiting` condition is allowed to become canonical network-failure evidence; the latter decides when an already-armed recovery is allowed to request a fresh authorized session.

## Recovery semantics

When the eligible post-start failure occurs, the recovery policy shall:

1. retain the last canonical absolute Recording position observed while the owner was actively playing/paused/seeking;
2. let the canonical owner stop consuming the failed local transport and publish its classified stopped failure;
3. keep the same Recording/backend presentation owner alive;
4. present the interruption as recoverable in the existing owner UI;
5. create no replacement MediaSession until same-origin recovery reachability is positively demonstrated.

The failed old MediaSession remains governed by the canonical owner/Gateway stop, disconnect and idle-cleanup rules. ADR-0057 does not add a parallel direct cleanup API merely to retry a stop request that could not cross the lost network.

When same-origin recovery reachability is positively demonstrated, the same owner shall:

1. request a fresh authorized Recording MediaSession through its existing owner start path with start-time autoplay explicitly suppressed;
2. require the fresh session to select the supported `progressive-fmp4` profile;
3. suppress the ordinary startup HLS compatibility rescue only for this already-established-playback recovery attempt;
4. keep the newly created presentation paused/not-yet-playing while its authoritative timeline position is prepared;
5. reposition that new session to the captured canonical absolute Recording position through the existing authoritative in-session seek operation when the position is non-zero;
6. only after the seek contract is complete, resume through the existing owner play operation;
7. report recovery success only after the owned media element produces real media again.

The recovery path must not implement `start -> autoplay -> immediate pause`. Starting the fresh transport without autoplay is a semantic owner option, not an artificial delay. It prevents the recovery policy from interrupting its own still-pending browser `play()` promise.

The canonical owner may clear the previous terminal failure when the fresh start request begins, as it already does for an explicit restart. The recovery policy must nevertheless remain visibly `recovering` until real media is observed, and any new failure produced by that fresh attempt remains canonical evidence.

The replacement MediaSession is therefore an explicit owner action based on a fresh authorized contract. Failure classification does not create it by itself.

## Continuity semantics

A successful network recovery creates a new decoder-significant presentation. The canonical owner lifecycle must publish the replacement MediaSession and advance playback-presentation continuity independently from `routeEpoch` and lifecycle publication revision.

The interrupted absolute Recording position remains the user-visible timeline coordinate. Recovery must not be accepted as successful from transport-local zero merely because the new transport starts with a fresh local presentation clock.

## Failure and retry policy

The first implementation does not use an unbounded MediaSession retry loop.

- A post-first-media `waiting` condition does not itself authorize recovery.
- A reachable-origin liveness probe does not convert a stall into network recovery.
- While recovery reachability probes fail, no recovery MediaSession is created.
- Recovery reachability probes may repeat at a bounded cadence while the interruption remains armed.
- One canonical interruption epoch allows one owner-authorized recovery attempt.
- Duplicate browser `online` events do not create additional sessions.
- Browser `navigator.onLine` is not sufficient proof that the Suite origin is reachable.
- If the fresh authorization/session/reposition contract fails after reachability was proven, recovery becomes terminal and the canonical owner remains/stops in `stopped`.
- No automatic HLS fallback, provider switch or unrelated profile switch is allowed after established fast-path playback.

A later change may add bounded backoff for demonstrated online-but-transient failures during the fresh recovery attempt, but that is not part of this initial policy.

## Failure classification relationship

Network-capable browser failure classifications may advertise a recovery class such as `new-authorized-contract`, but that remains descriptive evidence only.

The recovery policy must additionally verify the concrete preconditions in this ADR. In particular, changing a `recoveryClass` value must never be sufficient on its own to start recovery.

The bounded liveness rule does not bypass ADR-0056 classification. When its conditions are met, the canonical Recording owner converts the demonstrated post-start network stall into the same classified transport-failure surface consumed by ADR-0057. The recovery decorator continues to observe that canonical surface rather than starting recovery from DOM `waiting` directly.

## UI behavior

During ordinary short buffering the existing playback surface may continue to show:

```text
Aufnahme wartet auf Daten …
```

Only after the canonical owner has demonstrated an eligible network interruption does the recovery UI show a non-terminal status equivalent to:

```text
Verbindung unterbrochen · Wiedergabe wird fortgesetzt, sobald das Netzwerk wieder verfügbar ist.
```

During recovery it shows that reconnection is in progress. Only a failed authorized recovery becomes a terminal playback error. The UI policy does not claim a successful continuation until real media is playing again.

## Non-goals

This ADR does not authorize:

- treating every `waiting` or `stalled` event as a network failure;
- automatic recovery for decoder/codec/platform incompatibility;
- automatic recovery for authorization, fencing or source failures;
- automatic recovery for buffer/adaptation failures;
- hidden HLS fallback after established fast-path playback;
- provider switching;
- Live-TV reconnection policy;
- growing-Recording recovery semantics;
- unbounded MediaSession retries;
- Phase 66 work;
- replacement of the platform-native playback engine.

## Required proof

The implementation must prove at the production composition root that:

1. ordinary completed-Recording fast-path playback remains unchanged;
2. a classified post-start network failure is presented as recoverable while the canonical failed transport is stopped;
3. no new MediaSession is created while the Suite origin is unreachable;
4. browser `navigator.onLine=true` and missing `offline` events do not suppress recovery when the real Suite origin is unreachable;
5. a post-first-media `waiting` state with no explicit fetch/media error remains non-terminal during the bounded grace interval;
6. a long `waiting` state with no meaningful timeline progress does not become a network failure when the Suite origin remains reachable;
7. the same long no-progress state with failed same-origin liveness evidence publishes a canonical classified client transport failure and arms ADR-0057;
8. same-origin recovery reachability return causes the same owner to request exactly one fresh authorized MediaSession for that interruption epoch;
9. the fresh recovery session is started without autoplay, so recovery does not create a pending start-time `play()` request and then cancel it with `pause()`;
10. non-zero interrupted position is restored through the authoritative seek path before the recovery play request is issued;
11. successful real-media playback completes the recovery and returns the canonical owner to `playing`;
12. replacement session identity and continuity are published truthfully;
13. decoder/buffer/source/authorization failures still stop and do not auto-recover;
14. startup failure before first media retains the accepted compatibility fallback;
15. failed automatic recovery does not activate compatibility HLS;
16. repeated connectivity hints/probes do not create parallel sessions or an unbounded MediaSession retry loop;
17. the recovery decorator itself issues no direct MediaSession/API mutation request and delegates commands to the canonical owner.

Real browser/yaVDR acceptance must include a completed Recording on the fast path, actual network loss after real playback, a long enough outage for the existing buffer to drain into `waiting`, restoration of connectivity and automatic continuation near the interrupted absolute position without a user Play/Restart action or compatibility-mode switch. The long-outage case must not depend on a browser `offline` event, fetch-reader rejection or immediate `MediaError` being emitted.

## Relationship to existing architecture

- ADR-0046 remains authoritative for MediaSession/Gateway/provider ownership and authorization.
- ADR-0053 remains authoritative for platform playback engines, least-transformation adaptation and no hidden provider/profile fallback.
- ADR-0056 remains authoritative for the normalized playback contract, canonical lifecycle, continuity and failure classification.
- ADR-0057 adds only the bounded owner recovery policy for the demonstrated completed-Recording transient network interruption.

No earlier ADR is superseded.
