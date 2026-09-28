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

The next candidate mutation extension adds:

- `POST /api/v1/timer-assignments/{timerAssignmentId}?backend=...` for the
  accepted Timer CREATE admission. The caller supplies the item ETag and
  Idempotency-Key; the reference client performs one POST and returns the
  accepted Operation representation without retrying or polling automatically.

The caller supplies the Suite origin, transport and any authentication headers
or credentials. The reference client does not invent login/session behavior,
does not know browser-session/cache/provider routes, and performs no automatic
retry or route fallback.

The implementation exists to make the public contract executable for future TV,
mobile, desktop, Kodi and automation clients without pretending that the
bundled browser's richer pre-v1 routes are equivalent.
