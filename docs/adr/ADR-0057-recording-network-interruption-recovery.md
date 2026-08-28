# ADR-0057: Recording Network Interruption Recovery

## Status

**Accepted**

Date: 2026-08-28

## Context

Phase 65.D Slice 4 established classified playback failures without allowing classification itself to trigger hidden recovery. Real browser acceptance on the accepted Slice-4 candidate demonstrated the remaining product behavior clearly: a completed Recording playing through the normal `progressive-fmp4` fast path becomes terminally stopped when the browser loses network connectivity after playback has already started, and playback does not continue automatically when connectivity returns.

That behavior is safe, but it is not the preferred product behavior for a transient client network interruption. A short Wi-Fi/mobile transition or temporary loss of connectivity should not force the user to manually reopen a long Recording when the Suite can recover through the already-authoritative playback owner.

ADR-0056 already defines `recoveryClass` as descriptive policy evidence rather than an imperative and allows another presentation/session only through the canonical owner and an independently truthful authorized contract. This ADR defines the bounded policy for that one demonstrated recovery case.

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

## Initial bounded scope

The first implementation is intentionally narrow:

- completed Recording;
- normal `progressive-fmp4` browser path;
- playback has already produced real media (`firstMediaReported`);
- browser offline evidence exists for the post-start transport/platform network failure;
- recovery starts only when connectivity is available again;
- decoder, codec, source, authorization, buffer and adaptation failures remain terminal;
- startup failure before first media keeps the already accepted compatibility-fallback policy and is not changed by this ADR.

An `offline` browser event by itself is not enough to tear down healthy buffered playback. The policy waits for the actual canonical playback/transport failure and uses browser connectivity only as evidence that the demonstrated failure is a transient network interruption candidate.

## Recovery semantics

When the eligible post-start failure occurs, the recovery policy shall:

1. retain the last canonical absolute Recording position observed while the owner was actively playing/paused/seeking;
2. let the canonical owner stop consuming the failed local transport and publish its classified stopped failure;
3. keep the same Recording/backend presentation owner alive;
4. present the interruption as recoverable in the existing owner UI;
5. create no replacement MediaSession while the browser remains offline.

The failed old MediaSession remains governed by the canonical owner/Gateway stop, disconnect and idle-cleanup rules. ADR-0057 does not add a parallel direct cleanup API merely to retry a stop request that could not cross the lost network.

When connectivity is available again, the same owner shall:

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

The first implementation does not use an unbounded retry loop.

- While the browser is offline, no recovery session is created.
- One connectivity-return epoch allows one recovery attempt for that interruption.
- Duplicate `online` events in the same epoch do not create additional sessions.
- If connectivity disappears again during that attempt, the policy waits for the next connectivity-return epoch rather than spinning.
- If the browser is online but the fresh authorization/session/reposition contract fails for a non-network reason, recovery becomes terminal and the canonical owner remains/stops in `stopped`.
- No automatic HLS fallback, provider switch or unrelated profile switch is allowed after established fast-path playback.

A later change may add bounded backoff for demonstrated online-but-transient transport failures, but that is not part of this initial policy.

## Failure classification relationship

Network-capable browser failure classifications may advertise a recovery class such as `new-authorized-contract`, but that remains descriptive evidence only.

The recovery policy must additionally verify the concrete preconditions in this ADR. In particular, changing a `recoveryClass` value must never be sufficient on its own to start recovery.

## UI behavior

During the interruption the existing playback surface remains owned and shows a non-terminal recovery status equivalent to:

```text
Verbindung unterbrochen · Wiedergabe wird fortgesetzt, sobald das Netzwerk wieder verfügbar ist.
```

During recovery it shows that reconnection is in progress. Only a failed recovery becomes a terminal playback error. The UI policy does not claim a successful continuation until real media is playing again.

## Non-goals

This ADR does not authorize:

- automatic recovery for decoder/codec/platform incompatibility;
- automatic recovery for authorization, fencing or source failures;
- automatic recovery for buffer/adaptation failures;
- hidden HLS fallback after established fast-path playback;
- provider switching;
- Live-TV reconnection policy;
- growing-Recording recovery semantics;
- unbounded retries;
- Phase 66 work;
- replacement of the platform-native playback engine.

## Required proof

The implementation must prove at the production composition root that:

1. ordinary completed-Recording fast-path playback remains unchanged;
2. an offline-evidenced post-start network failure is presented as recoverable while the canonical failed transport is stopped;
3. no new MediaSession is created while offline;
4. connectivity return causes the same owner to request exactly one fresh authorized MediaSession for that epoch;
5. non-zero interrupted position is restored through the authoritative seek path before recovery is accepted as resumed;
6. successful real-media playback completes the recovery and returns the canonical owner to `playing`;
7. replacement session identity and continuity are published truthfully;
8. decoder/buffer/source/authorization failures still stop and do not auto-recover;
9. startup failure before first media retains the accepted compatibility fallback;
10. failed automatic recovery does not activate compatibility HLS;
11. repeated connectivity events do not create parallel sessions or an unbounded retry loop;
12. the recovery decorator itself issues no direct MediaSession/API request and delegates commands to the canonical owner.

Real browser/yaVDR acceptance must include a completed Recording on the fast path, actual network loss after real playback, restoration of connectivity and automatic continuation near the interrupted absolute position without a user Play/Restart action or compatibility-mode switch.

## Relationship to existing architecture

- ADR-0046 remains authoritative for MediaSession/Gateway/provider ownership and authorization.
- ADR-0053 remains authoritative for platform playback engines, least-transformation adaptation and no hidden provider/profile fallback.
- ADR-0056 remains authoritative for the normalized playback contract, canonical lifecycle, continuity and failure classification.
- ADR-0057 adds only the bounded owner recovery policy for the demonstrated completed-Recording transient network interruption.

No earlier ADR is superseded.
