# Public-v1 EPG genres — Web parity

The established Web `GenreBrowserApiRuntime` and `GenreIndexRepository` own
Film, Serie, Dokumentation, Sport and canonical Film subgenres. These are
projected, not reimplemented, through two authenticated Device GET endpoints:

- `/api/v1/genres/epg?backendId=default&limit=30&offset=0`
- `/api/v1/genres/epg/events?backendId=default&contentClass=movie&genreId=adventure&limit=30&offset=0`

Both require `epg.view@default` (separate from `channels.view` for
Live-TV and `media.live.play` for playback). Granting a TV its rights
requires a real, CSRF-protected, audited administrator Device-grant mutation.
There are **no implicit pairing permissions and no direct SQLite writes**.

The overview is the existing Web taxonomy, flattened for D-pad navigation
(parent categories followed by their canonical movie children).
The EPG window is the Web Genre default: now through the next 48 hours.
Events contain backend, channel and event IDs, title, subtitle and times,
but never native paths, provider URLs or unauthenticated playback links.
Pages are bounded to 30 records, and scope/permission denial fails closed.

Acceptance must compare EPG-category counts and EPG items with the Web
`/api/metadata/genres?scope=epg` and
`/api/metadata/genres/epg` for the same window before live TV testing.
Live-TV permissions are separate: `channels.view`, `epg.view` and
`media.live.play` may all be assigned to an already paired Hisense device
via administrator `/api/v1/devices/{deviceId}/grants` with ETag and CSRF.
