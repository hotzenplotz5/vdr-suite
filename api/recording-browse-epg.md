# Public-v1 Recording browse, Genres and EPG now/next — developer contract

## Navigation

- [API developer entry](README.md)
- [OpenAPI 3.1 source-verified subset](openapi.json)
- [Recording playback API: separate access boundary](../docs/development/public-v1-device-recording-playback.md)
- [Public-v1 EPG now/next original contract](../docs/development/public-v1-epg-now-next.md)
- [REST runtime](rest/src/PublicApiRuntime.cpp)
- [Security gate](../core/security/include/SecurityHttpGate.h)

## Scope and evidence

Source baseline: `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`, 2026-10-11. Public-v1 routes, bounded parsers, serializers, security permissions and REST tests have been inspected. These are *read-only metadata* APIs and must not be confused with device-owned playback, media-track, native VDR directory or Web-private endpoints. Source presence is not proof that a given yaVDR daemon or Hisense VIDAA build has deployed these routes.

All four GET operations require an authenticated Actor and **exactly one** `backendId` query value. The security gate enforces the grant and authorized backend scope; runtime checks that the supplied backend matches the scoped set. The Recording and Genre reads require `recordings.view@backend`; EPG now/next requires `epg.view@backend`, **not just `channels.view`**. Invalid/duplicate/unknown query fields and mismatched scopes yield HTTP 400 (the security gate can return 401/403 first).

## 1. Recording collection (keyset mode)

```http
GET /api/v1/recordings?backendId=home&limit=50&sort=recordingId&order=asc
```

The default mode reads the normalized Suite recording catalog with `recordingId ASC`, `limit` 1–100 (default 50) and opaque `rc1_` backend-scoped cursor. Do **not** construct, inspect or substitute a different backend's cursor. A malformed cursor yields 400; a cursor from another backend scope yields **409 `cursor_expired`**. Response:

```json
{"items":[{"recordingId":"rec_0123456789abcdef0123456789abcdef","backendId":"home","title":"Film","recordedAt":"2026-10-09T20:15:00Z","durationSeconds":3600,"durationKnown":true,"description":"Kurzbeschreibung","sizeMb":2048}],"page":{"limit":50,"nextCursor":null,"hasMore":false},"meta":{"partial":false},"links":{"self":"/api/v1/recordings?backendId=home&limit=50&sort=recordingId&order=asc","next":null}}
```

No provider/native recording path, native identifier, playlist URL, artwork URL, ETag or media session is exposed. The `recordingId` is a durable Suite identity (`rec_` + 32 lowercase hexadecimal digits). For `durationKnown:false`, the numerical field is not proof of a measured duration.

## 2. Folder browse (same URL, distinct wire shape)

```http
GET /api/v1/recordings?backendId=home&view=folders&limit=50&offset=0
GET /api/v1/recordings?backendId=home&view=folders&folderId=fld1_0123456789abcdef0123456789abcdef&limit=50&offset=0
```

Folder mode uses **offset pagination**, not a cursor. It permits `view=folders`, optional opaque `folderId` (`fld1_` + 32 lowercase hex characters), `limit` 1–100, `offset` 0–1,000,000. It rejects `cursor`, `sort`, `order`; default keyset mode rejects `folderId` and `offset`. The shape is:

```json
{"items":[{"kind":"folder","folderId":"fld1_0123456789abcdef0123456789abcdef","name":"Serien","recordingCount":3},{"kind":"recording","recordingId":"rec_0123456789abcdef0123456789abcdef","backendId":"home","title":"Film","recordedAt":"2026-10-09T20:15:00Z","durationSeconds":3600,"durationKnown":true,"description":"Kurzbeschreibung","sizeMb":2048}],"page":{"limit":50,"offset":0,"totalCount":2,"hasMore":false},"meta":{"partial":false}}
```

**Single-recording title folders may appear as `kind=recording` leaves**, not an extra clickable folder. Do not reconstruct a filesystem path from the folder ID. For player requests use the separate device-owned Recording playback API, not this metadata endpoint.

## 3. Recording genre facets and membership

```http
GET /api/v1/genres?backendId=home&locale=de&limit=30&offset=0
GET /api/v1/genres/recordings?backendId=home&genreId=crime&locale=en&limit=30&offset=0
```

Both require `recordings.view@home`; `genreId` is **required** only on the membership endpoint and forbidden on the facet root. The genre ID is a bounded alphanumeric/dot/underscore/hyphen key, not a path. Optional `locale` = `de` or `en` (default `de`), limit 1–30 (default 30), offset 0–1,000,000. No cursor or ETag.

`GET /genres` items carry `genreId`, `label`, `labelDe`, `labelEn`, `count`. `GET /genres/recordings` items carry `recordingId`, `backendId`, `title`, `recordedAt`, `durationSeconds`, `durationKnown`. Both responses have `items`, `page:{limit,offset,totalCount,hasMore}`, `meta:{partial:false}`. The count is a facet membership count, not evidence that an item can be played with the current Device grants.

### Browser migration: recording genre overview

The facet root additionally returns `totalItems` (distinct indexed recordings)
and `assignmentCount` for the entire backend, including on subsequent pages.
Each facet adds `known`, `activeCount`, `staleCount`, `conflictCount` and
`displayOrder` (zero-based position in the canonical Web index order).
The existing genreId page ordering, membership response and authorization are
unchanged. Counts and display order come from the existing Genre index, not a
second service or a client-side sum of facets.

The facet root reads the existing persisted index of a registered, enabled
backend even when its live VDR is offline or no VDR snapshot is available.
The counts are cached index data, not proof of current backend availability.
The membership route retains its existing online, snapshot and recording
identity requirements. Unknown or disabled backends remain unavailable.

The maintainer confirmed that recording Genre visibility follows the existing
recordings.view grant, without a separate Genre permission. The browser
recording overview (Home and Genres module) now uses `GET /api/v1/genres`
with the same grant as external clients. This deliberately replaces the
legacy authenticated-only overview admission for these browser callers.
The existing Public-v1 enabled-backend requirement remains; the legacy
endpoint itself is unchanged. EPG and membership callers are not migrated
by this change.

The HTTP response decorator supplies the existing session/backend/grant Home
recording-cache proof for successful browser-authenticated pages, only for
the already admitted backend. The browser Client API reads all pages,
restores the canonical Web display order and passes each original response
to the existing cache ticket. An aggregate object is never a substitute
for page proofs. Missing cache proof does not block online reads. No Device
cache proof, new grant, administrator requirement or fallback on 403.

### Browser migration: recording membership cards

Membership now adds `sizeMb`, `genreIds` and optional `cardMetadata`
(`mediaType`, `title`, `episodeName`, `seasonNumber`, `episodeNumber`)
from the same page-scoped Genre index that the Web cards use. The existing
locked manual/native metadata precedence is preserved. No provider ID,
native recording path, resource key or artwork path is exposed. The current
opaque recording identity and membership ordering remain unchanged.

The membership page optionally includes `preferredArtworkId` from the
existing Recording Artwork identity owner. A selected locked manual poster
takes precedence; absent manual artwork does not fall back to an older
native image. Otherwise the existing cached preferred-artwork selection
is used. Clients can request the existing authenticated binary route
`/api/v1/recordings/{recordingId}/artwork/{preferredArtworkId}?backendId=...`
without a separate artwork-list HTTP request for each card. The read-only
manual selection is checked per member using the existing metadata owner;
no images are loaded or provider requests made by the page.

The optional `seriesHierarchyOverride` object uses the same page-batched
SQLite projection as Web. It preserves the existing `groupType`
(season, special, custom), `seasonNumber`, `groupLabel`, `sortOrder`,
`episodeStart`, `episodeEnd` and `revision`, independently of the native
season and episode numbers in cardMetadata. No per-recording hierarchy
lookup, schema or additional identifier is introduced.

Series membership adds optional `seriesGrouping` from the same embedded
Genre page facts: an opaque backend-scoped `groupKey`, the folder's display
leaf `folderTitle`, and `nativeMetadataAvailable` for the existing native
metadata priority. The initial Web grouping uses the Serien/<series> root
across seasons or the immediate parent elsewhere, then native series identity
or a title fallback. The grouping seed is never returned; the existing daemon
SHA-256 implementation derives a non-persistent sg1_ key. No additional index
or metadata lookup is introduced. Case normalization of the seed is ASCII;
Unicode case-variant folders and rich/enriched grouping parity require
verification before switching the browser.

The existing authenticated Series artwork settings response now adds a
`publicGroupKey` alias for each stored legacy cover key, computed by the
same owner as the Genre member group. The browser resolves both keys to
the same cover and uses the original persisted key when changing or
clearing an existing assignment. No settings rows, image URLs or revisions
are migrated. This is compatibility for matching group seeds, not proof
of equivalence for all enriched/Unicode group variants.

Series members additionally carry `seriesGrouping.ancestorFolderIds`
using the existing Recording cache folder hierarchy and public folder ID
calculation. The legacy Web folder response adds the same IDs to the
current page and child entries. Home's folder Series projection can use
exact IDs instead of native descendant paths for Public-v1 members; the
existing partial-root/full-direct-folder rules and original-folder escape
remain unchanged. The same Series group and selected cover are reused on
both surfaces. No new browse database, lookup or folder ID scheme is used.

The standalone Web Genres module now reads membership from
`GET /api/v1/genres/recordings` in pages of 30. The existing rich card
presentation is retained during migration by bounded, four-at-a-time private
Web detail reads keyed by the public member identity. The existing private
folder endpoint accepts `backend` and `recordingId` for one exact recording:
it resolves the backend-scoped public ID using the existing identity
repository, verifies the cached native recording, and returns the established
Browser-only recording/metadata model. On selection the module resolves that
identity again before handing the recording to the existing detail/action
owner; no title-based lookup or native ID substitution. Native paths are not
added to Public-v1 responses. The private Web authorization and all action
contracts are unchanged. A failed optional enrichment leaves the public card
visible; a failed detail resolution is shown locally without selecting a
different recording.

Recording RENAME/DELETE now use the existing Public-v1 action contract
when a public recording identity is present; MOVE remains private until
new-folder creation is represented. Candidate search, seasons and episodes,
plus manual assignment/withdrawal with a public identity, now use
`POST /api/v1/backends/{backendId}/recordings/metadata/{operation}`.
These calls reuse the existing scoped `metadata.recording.assign`
permission, Browser CSRF, controller, revision and presentation-change
notification. Assignment/withdrawal accept only an opaque recordingId
and resolve the current backend-native key server-side. The existing
candidate DTO and provider-relative poster reference are preserved.
The separate read-only automatic metadata resolve endpoint is unchanged.
Legacy-only recording entry points still use their existing write routes.
Manual recording Genre selection now supports GET and POST on
`/api/v1/backends/{backendId}/recordings/metadata/genre`. GET accepts
exactly one opaque `recordingId` query value and uses the existing
`recordings.view` grant; POST accepts `recordingId` and an explicit
`genreId` (empty clears the manual selection), using the existing
`metadata.recording.assign` grant and Browser CSRF. The canonical genre
list, automatic genres and presentation-change notification are preserved.
The browser uses these operations whenever it has a public identity.
Full rich metadata detail and series hierarchy editing remain private
pending migration. Home Series membership,
new cover writes from opaque-only clients, and enriched/Unicode grouping
parity remain separate work; the existing Web cover workflow is unchanged.

## Recording titles and edited cuts: public presentation parity

Public-v1 projects Recording names using the established Web
\`recordings2-shared.js\` display rules, **on the server**:
- the last title directory from a VDR recording path (discarding the final
  \`.rec\` timestamp directory) wins over raw VDR \`Folder~Title\` metadata;
- if no usable path is present, the last component of the native title wins;
- VDR \`#XX\` escapes (up to 6 decoding rounds) and underscores are decoded;
- a **leading** \`%\` is removed only from the visible title and separately
  advertised as the additive read-only boolean \`edited\`; an embedded percent
  (e.g. \`100% Abenteuer\`) is never removed.

A path \`/srv/vdr/video/Filme/%Der_Hobbit/2026-10-15.rec\`
is presented as \`"title":"Der Hobbit","edited":true\`, while the
unsliced original has \`edited:false\`. The two recordings retain **different
opaque recordingId values**. No native folder path, source ID, media token,
mutating rename or cut action leaves the server.

The same projection is used by keyset collection, folder browsing, genre
memberships and native-title search results. The existing Web global search
may prefer a manually assigned metadata title; that canonical search title
is intentionally retained. The \`edited\` marker still comes from the current
authorized recording identity. Legacy clients can ignore the additive field.

## 4. EPG now/next (cached read)

```http
GET /api/v1/epg/now-next?backendId=home&channelId=S19.2E-1-100&fromTime=1791565200&limit=2
```

Requires **`epg.view@home`** for the requested `backendId`. Both `backendId`, `channelId` and `fromTime` are mandatory. `channelId` is limited to 1–256 alphanumeric/dot/underscore/hyphen characters; `fromTime` is **decimal Unix epoch seconds**, 10–12 digits without a leading zero. Limit is 1–2 (default 2); `cursor`/`offset` and all unknown fields are invalid. One `GET` uses cached service data only; it does **not** refresh EPG.

```json
{"backendId":"home","channelId":"S19.2E-1-100","items":[{"channelId":"S19.2E-1-100","title":"Nachrichten","subtitle":"Jetzt","startTime":"1791565200","endTime":"1791568800","durationSeconds":3600}],"page":{"limit":2,"hasMore":false},"meta":{"partial":false}}
```

An empty `items` array means no matching cached events, not that nothing is broadcast. Response timestamps are **strings** in seconds, not JavaScript milliseconds or localized date strings. It returns `404` for an unknown channel, `503` for an unavailable cache/backend or inconsistent provider data; no native EPG IDs or private URLs are exposed.

## Error and client interoperability notes

| HTTP | Meaning |
| --- | --- |
| `200` | Read succeeded, including a valid empty result |
| `400` | Invalid query or backend authorization scope mismatch |
| `401` / `403` | Authentication required / backend-specific permission denied |
| `404` | Unknown EPG channel (not a generic empty event list) |
| `409` | Recording keyset cursor from wrong backend scope |
| `503` | Backend/data provider/read projection or accountability unavailable |

These HTTP reads cannot create Timer assignments, start playback or select tracks. Track 404 diagnostics must compare the **installed** daemon's actual route/method and Device grant scope with the requesting client's URL; Web frontend success is not a substitute for that comparison.

## Regression evidence

- [Public recording collection and folders](rest/tests/test_public_recording_collection.cpp)
- [Genre facet/membership responses](rest/tests/test_public_recording_genre_collection.cpp)
- [EPG now/next contract](rest/tests/test_public_epg_now_next.cpp)
- [Recording read security checks](../core/security/tests/test_public_recording_collection_security.cpp)
- [EPG permission/scope security checks](../core/security/tests/test_public_epg_now_next_security.cpp)
- [Scoped OpenAPI drift guard](../tools/check_public_v1_openapi.py)

## Back

- [API entry](README.md)
- [Documentation index](../docs/index.md)

### Home discovery views

`GET /api/v1/recordings?backendId=...&view=new&limit=12&offset=0` uses the same newest-recording query as Web Home. `view=movies` requires `movieReleaseYearFrom` and `movieReleaseYearTo` (inclusive) and reuses the Web owner's release-date order and manual metadata overlay. Both return the existing offset browse response with recording-only items and optional subtitle. Movie items additionally expose the effective `releaseDate` from the same native/manual query selection. Home cards optionally carry the existing opaque `preferredArtworkId`; a locked manual assignment takes precedence over cached artwork, with no fallback to old artwork when the selected manual poster is absent. No native path or extra permission. The legacy keyset and folder views remain unchanged.
