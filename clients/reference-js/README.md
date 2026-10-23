# VDR-Suite Public-v1 JavaScript Reference Client

This directory contains a small reference implementation for the deliberately
stable public-v1 client boundary.

It is **not** a published npm package and it is not the browser Web Client API.
The accepted initial discovery slice covers:

- `GET /api/v1`;
- `GET /api/v1/capabilities`;
- `GET /api/v1/backends`.

The accepted next bounded read extension adds:

- `GET /api/v1/channels?backendId=...` with explicit 1–16 source selection,
  keyset pagination and unchanged partial-source metadata.

The accepted TimerAssignment collection extension adds:

- `GET /api/v1/timer-assignments?backend=...` as a single-backend Suite-owned
  keyset collection with no collection ETag and no native-VDR fallback.

The accepted revisioned item extension adds:

- `GET /api/v1/timer-assignments/{timerAssignmentId}?backend=...` with the
  accepted opaque ETag and `If-None-Match -> 304` contract.

The accepted mutation extension adds:

- `POST /api/v1/timer-assignments/{timerAssignmentId}?backend=...` for the
  accepted Timer CREATE admission. The caller supplies the item ETag and
  Idempotency-Key; the reference client performs one POST and returns the
  accepted Operation representation without retrying or polling automatically.

The final stable-contract read extension adds:

- `GET /api/v1/operations/{operationId}` with the accepted opaque ETag and
  `If-None-Match -> 304` contract. It is an explicit one-shot read; the
  reference client does not add a polling loop.

The caller supplies the Suite origin, transport and any authentication headers
or credentials. The reference client does not invent login/session behavior,
does not know browser-session/cache/provider routes, and performs no automatic
retry or route fallback.

The implementation exists to make the public contract executable for future TV,
mobile, desktop, Kodi and automation clients without pretending that the
bundled browser's richer pre-v1 routes are equivalent.

## P1 Device recording progress — source candidate

The client also offers `getContinueWatching({backendId, headers})`: a bounded list of public rec_ IDs, confirmed positions and nullable durations. Both GET variants require an authenticated Device with the exact play grant.

A currently verified Device with `media.recording.play@backendId` may use
`getRecordingProgress({backendId, recordingId, headers})` and
`updateRecordingProgress({backendId, recordingId, operationId,
positionSeconds, expectedRevision, sessionId, headers})`. Use
`clearRecordingProgress({backendId, recordingId, operationId,
expectedRevision, headers})` to commit a zero-position reset.

The recording ID is an opaque public `rec_` identity; it is never a VDR path.
Read `revision` before the first write; send its current value as
`expectedRevision` on subsequent checkpoints, pause and stop. On 409, re-read
and resolve the conflict from the client's observed playback, **not** by
blind retry. Persist only measured playback positions, not elapsed session time. Nonzero progress must reference an active, exact-recording, Device-owned `sessionId`; zero reset may omit it.
Resume playback by explicitly creating the existing Device Recording
MediaSession with `startPositionSeconds` *only if the chosen profile actually
supports it*. Session CREATE is not automatically invoked by these helpers.

Device pairing does not bind its Service Actor to a Human Account. Consequently
this initial projection is **Device-actor-private**, not cross-account or
cross-Device shared history; it cannot read a Browser Human Account's saved
Continue Watching position. Device credentials are provided by the caller,
never synthesized by the library. The browser uses its original CSRF-protected
legacy route unchanged; the new route does not accept a browser session.
