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

The recovery flow is:

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

The same persistent first-party playback owner remains authoritative throughout. Recovery must not introduce another playback controller, provider-selection path or transport owner.

## Initial bounded scope

The first implementation is intentionally narrow:

- completed Recording;
- normal `progressive-fmp4` browser path;
- playback has already produced real media (`firstMediaReported`);
- the browser is observably offline when the post-start transport/platform network failure becomes authoritative;
- recovery starts only after connectivity returns;
- decoder, codec, source, authorization, buffer and adaptation failures remain terminal;
- startup failure before first media keeps the already accepted compatibility-fallback policy and is not changed by this ADR.

An `offline` browser event by itself is not enough to tear down healthy buffered playback. The owner waits for the actual playback/transport failure and uses browser connectivity only as evidence that the demonstrated failure is a transient network interruption candidate.

## Recovery semantics

When the eligible post-start failure occurs, the owner shall:

1. capture the canonical absolute Recording position owned by the current presentation;
2. stop consuming the failed local transport;
3. publish an `interrupted` lifecycle state while preserving the classified failure evidence;
4. retain the Recording/backend identity and the interrupted position;
5. wait for browser connectivity to return rather than creating sessions while offline.

When connectivity returns, the same owner shall:

1. best-effort clean up the old MediaSession;
2. request a fresh authorized Recording MediaSession using the normal capability contract;
3. require the fresh session to select the supported `progressive-fmp4` profile;
4. reposition that new session to the captured canonical absolute Recording position through the existing authoritative in-session seek operation when the position is non-zero;
5. connect the new continuous-fMP4 transport only after the replacement session/reposition contract is valid;
6. publish the replacement session through the canonical lifecycle and resume playback;
7. clear the transient failure only when real media playback resumes.

The replacement MediaSession is therefore an explicit owner action based on a fresh authorized contract. Failure classification does not create it by itself.

## Continuity semantics

A successful network recovery creates a new decoder-significant presentation. The canonical owner lifecycle must publish the replacement MediaSession and advance playback-presentation continuity independently from `routeEpoch` and lifecycle publication revision.

The interrupted absolute Recording position remains the user-visible timeline coordinate. Recovery must not resume from transport-local zero merely because the new transport starts with a fresh local presentation clock.

## Failure and retry policy

The first implementation does not use an unbounded retry loop.

- While the browser is offline, no recovery session is created.
- A browser `online` transition allows one recovery attempt for that interruption.
- If connectivity disappears again during that attempt, the owner returns to `interrupted` and waits for the next `online` transition.
- If the browser is online but the fresh authorization/session/reposition contract fails for a non-network reason, recovery becomes terminal and the owner enters `stopped`.
- No automatic HLS fallback, provider switch or unrelated profile switch is allowed after established fast-path playback.

A later change may add bounded backoff for demonstrated online-but-transient transport failures, but that is not part of this initial policy.

## Failure classification relationship

Network-capable browser failure classifications may advertise a recovery class such as `new-authorized-contract`, but that remains descriptive evidence only.

The owner must additionally verify the concrete recovery preconditions in this ADR. In particular, changing a `recoveryClass` value must never be sufficient on its own to start recovery.

## UI behavior

During the interruption the existing playback surface should remain owned and show a non-terminal status equivalent to:

```text
Verbindung unterbrochen · Wiedergabe wird fortgesetzt, sobald das Netzwerk wieder verfügbar ist.
```

During recovery it should show that reconnection is in progress. Only a failed recovery becomes a terminal playback error.

## Non-goals

This ADR does not authorize:

- automatic recovery for decoder/codec/platform incompatibility;
- automatic recovery for authorization, fencing or source failures;
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
2. an offline post-start network failure publishes `interrupted`, not terminal `stopped`;
3. no new MediaSession is created while offline;
4. an `online` transition causes the same owner to request one fresh authorized MediaSession;
5. non-zero interrupted position is restored through the authoritative seek path before the replacement transport is treated as resumed;
6. successful real-media playback clears the transient failure and returns the owner to `playing`;
7. replacement session identity and continuity are published truthfully;
8. decoder/buffer/source/authorization failures still stop and do not auto-recover;
9. startup failure before first media retains the accepted compatibility fallback;
10. repeated offline/online transitions do not create parallel sessions or an unbounded retry loop.

Real browser/yaVDR acceptance must include a completed Recording on the fast path, actual network loss after real playback, restoration of connectivity and automatic continuation near the interrupted absolute position.

## Relationship to existing architecture

- ADR-0046 remains authoritative for MediaSession/Gateway/provider ownership and authorization.
- ADR-0053 remains authoritative for platform playback engines, least-transformation adaptation and no hidden provider/profile fallback.
- ADR-0056 remains authoritative for the normalized playback contract, canonical lifecycle, continuity and failure classification.
- ADR-0057 adds only the bounded owner recovery policy for the demonstrated completed-Recording transient network interruption.

No earlier ADR is superseded.
