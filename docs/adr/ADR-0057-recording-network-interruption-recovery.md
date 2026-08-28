# ADR-0057: Recording Network Interruption Recovery

## Status

**Accepted**

Date: 2026-08-28

## Context

Phase 65.D Slice 4 established classified playback failures without allowing classification itself to trigger hidden recovery. Real browser acceptance on the accepted Slice-4 candidate demonstrated the remaining product behavior clearly: a completed Recording playing through the normal `progressive-fmp4` fast path becomes terminally stopped when the browser loses network connectivity after playback has already started, and playback does not continue automatically when connectivity returns.

That behavior is safe, but it is not the preferred product behavior for a transient client network interruption. A short Wi-Fi/mobile transition or temporary loss of connectivity should not force the user to manually reopen a long Recording when the Suite can recover through the already-authoritative playback owner.

The first ADR-0057 implementation candidate exposed an additional real-browser fact: Android/Edge connectivity hints are not a reliable authority for this policy. A long outage could produce the canonical post-start `client_media_network_error` while the browser did not provide the `offline` evidence required by the first implementation. Short outages could meanwhile be hidden by existing media buffering and resume without exercising recovery at all.

ADR-0056 already defines `recoveryClass` as descriptive policy evidence rather than an imperative and allows another presentation/session only through the canonical owner and an independently truthful authorized contract. This ADR defines the bounded policy for that demonstrated recovery case without making browser connectivity hints authoritative.

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

Browser `online` / `offline` state is advisory only. A canonical classified post-start network failure arms the policy. A separate read-only same-origin reachability probe determines when the Suite origin is reachable enough to attempt a new authorized Recording session.

## Initial bounded scope

The first implementation is intentionally narrow:

- completed Recording;
- normal `progressive-fmp4` browser path;
- playback has already produced real media (`firstMediaReported`);
- the canonical owner publishes a classified post-start transport/platform network failure;
- browser `online` / `offline` events may accelerate UI/probing but are not required evidence;
- recovery starts only after a same-origin Suite reachability request receives an HTTP response;
- decoder, codec, source, authorization, buffer and adaptation failures remain terminal;
- startup failure before first media keeps the already accepted compatibility-fallback policy and is not changed by this ADR.

An `offline` browser event by itself is not enough to tear down healthy buffered playback. The policy waits for the actual canonical playback/transport failure.

Conversely, a missing `offline` event must not prevent recovery after the canonical network failure has occurred.

## Reachability evidence

The recovery policy may issue a read-only same-origin request to the existing VDR-Suite health endpoint solely to distinguish "origin unreachable" from "origin reachable enough to attempt authorization".

The initial implementation uses:

```text
GET /api/vdr/health
credentials: same-origin
cache: no-store
```

Any completed HTTP response proves same-origin transport reachability for this purpose. The health payload or HTTP success status is not reused as playback capability, provider, authorization or VDR-health authority.

A failed fetch means only that the Suite origin is not currently reachable. While armed, the policy may repeat this observational probe at a bounded cadence. These probes are not MediaSession creation attempts and do not change playback lifecycle authority.

## Recovery semantics

When the eligible post-start failure occurs, the recovery policy shall:

1. retain the last canonical absolute Recording position observed while the owner was actively playing/paused/seeking;
2. let the canonical owner stop consuming the failed local transport and publish its classified stopped failure;
3. keep the same Recording/backend presentation owner alive;
4. present the interruption as recoverable in the existing owner UI;
5. create no replacement MediaSession until same-origin reachability is positively demonstrated.

The failed old MediaSession remains governed by the canonical owner/Gateway stop, disconnect and idle-cleanup rules. ADR-0057 does not add a parallel direct cleanup API merely to retry a stop request that could not cross the lost network.

When same-origin reachability is positively demonstrated, the same owner shall:

1. request a fresh authorized Recording MediaSession through its existing normal start path;
2. require the fresh session to select the supported `progressive-fmp4` profile;
3. suppress the ordinary startup HLS compatibility rescue only for this already-established-playback recovery attempt;
4. pause the newly created presentation before treating it as resumed;
5. reposition that new session to the captured canonical absolute Recording position through the existing authoritative in-session seek operation when the position is non-zero;
6. resume through the existing owner play operation;
7. report recovery success only after the owned media element produces real media again.

The canonical owner may clear the previous terminal failure when the fresh start request begins, as it already does for an explicit restart. The recovery policy must nevertheless remain visibly `recovering` until real media is observed, and any new failure produced by that fresh attempt remains canonical evidence.

The replacement MediaSession is therefore an explicit owner action based on a fresh authorized contract. Failure classification does not create it by itself.

## Continuity semantics

A successful network recovery creates a new decoder-significant presentation. The canonical owner lifecycle must publish the replacement MediaSession and advance playback-presentation continuity independently from `routeEpoch` and lifecycle publication revision.

The interrupted absolute Recording position remains the user-visible timeline coordinate. Recovery must not be accepted as successful from transport-local zero merely because the new transport starts with a fresh local presentation clock.

## Failure and retry policy

The first implementation does not use an unbounded MediaSession retry loop.

- While same-origin reachability probes fail, no recovery MediaSession is created.
- Reachability probes may repeat at a bounded cadence while the interruption remains armed.
- One canonical interruption epoch allows one owner-authorized recovery attempt.
- Duplicate browser `online` events do not create additional sessions.
- Browser `navigator.onLine` is not sufficient proof that the Suite origin is reachable.
- If the fresh authorization/session/reposition contract fails after reachability was proven, recovery becomes terminal and the canonical owner remains/stops in `stopped`.
- No automatic HLS fallback, provider switch or unrelated profile switch is allowed after established fast-path playback.

A later change may add bounded backoff for demonstrated online-but-transient failures during the fresh recovery attempt, but that is not part of this initial policy.

## Failure classification relationship

Network-capable browser failure classifications may advertise a recovery class such as `new-authorized-contract`, but that remains descriptive evidence only.

The recovery policy must additionally verify the concrete preconditions in this ADR. In particular, changing a `recoveryClass` value must never be sufficient on its own to start recovery.

## UI behavior

During the interruption the existing playback surface remains owned and shows a non-terminal recovery status equivalent to:

```text
Verbindung unterbrochen · Wiedergabe wird fortgesetzt, sobald das Netzwerk wieder verfügbar ist.
```

During recovery it shows that reconnection is in progress. Only a failed authorized recovery becomes a terminal playback error. The UI policy does not claim a successful continuation until real media is playing again.

## Non-goals

This ADR does not authorize:

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
5. same-origin reachability return causes the same owner to request exactly one fresh authorized MediaSession for that interruption epoch;
6. non-zero interrupted position is restored through the authoritative seek path before recovery is accepted as resumed;
7. successful real-media playback completes the recovery and returns the canonical owner to `playing`;
8. replacement session identity and continuity are published truthfully;
9. decoder/buffer/source/authorization failures still stop and do not auto-recover;
10. startup failure before first media retains the accepted compatibility fallback;
11. failed automatic recovery does not activate compatibility HLS;
12. repeated connectivity hints/probes do not create parallel sessions or an unbounded MediaSession retry loop;
13. the recovery decorator itself issues no direct MediaSession/API mutation request and delegates commands to the canonical owner.

Real browser/yaVDR acceptance must include a completed Recording on the fast path, actual network loss after real playback, restoration of connectivity and automatic continuation near the interrupted absolute position without a user Play/Restart action or compatibility-mode switch. The long-outage case must not depend on a browser `offline` event being emitted.

## Relationship to existing architecture

- ADR-0046 remains authoritative for MediaSession/Gateway/provider ownership and authorization.
- ADR-0053 remains authoritative for platform playback engines, least-transformation adaptation and no hidden provider/profile fallback.
- ADR-0056 remains authoritative for the normalized playback contract, canonical lifecycle, continuity and failure classification.
- ADR-0057 adds only the bounded owner recovery policy for the demonstrated completed-Recording transient network interruption.

No earlier ADR is superseded.
