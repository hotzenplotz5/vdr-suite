# Public-v1 generic Recording and EPG search (candidate)

## Scope / dependencies

The browser's `/api/search` already uses `GlobalSearchService` and
`GlobalSearchRepository`. The VIDAA client previously called
`/api/v1/recordings?view=search` and `/api/v1/epg/search` which did not
exist in the installed backend, causing an HTTP 400/503 failure. This
candidate adds two *read-only* device endpoints backed by that same index:

- `GET /api/v1/search/recordings?backendId=default&query=hobbit&limit=30&offset=0`
- `GET /api/v1/search/epg?backendId=default&query=hobbit&limit=30&offset=0`

Both return `{items:[],page:{offset,limit,totalCount,hasMore}}`.
Recording items use `kind=recording`, `backendId`, opaque
`recordingId=rec_<32hex>`, `title`.
EPG items use `kind=epg`, `backendId`, `channelId`, `eventId`,
`title`, `subtitle`, `startTime`, `endTime`.
No native paths, native recording identifiers, local HTML URLs, media grants,
or unversioned browser route responses are exposed.

The routes are gated through `SecurityHttpGate`: Recording search requires
`recordings.view`, EPG search requires `epg.view`. Both are individually
assignable to the paired Device through the administrator's Device-grant API;
pairing, `channels.view` and playback rights never imply search rights.
Exactly one authorized
backend and a valid actor are required. The server performs strict query
parsing, bounded `limit<=30`, `offset<=1000000`, rejects unknown/duplicate
fields, only projects matching backend data and checks actual live Recording
identities before returning search results. The same normalization/text-folding
code as Web is executed in the underlying index; **no regex matching claim**
is made until the engine explicitly supports it. Unavailable index, identity,
or backend fails closed rather than falsely returning an empty result.
Individual stale or unprojectable search hits are omitted without failing
other valid results. The adapter preserves the effective Web-search title,
not the native VDR title. See `docs/development/public-v1-search-parity.md`
for the mandatory Web/Public-v1/VIDAA and competitor acceptance matrix.

EPG scope is the same Web index window (six hours back, fourteen days ahead);
it is not EPG-history crawling. Search is generic across clients, never TV-only.

## Rollout order (no independent VIDAA-only deploy)

1. Confirm exact backend/VIDAA branch candidates, build and test.
2. Test public routes for authenticated, authorized and denied device callers,
   actual data, mixed case, accented text, empty, pagination and outdated cache.
3. Build backend as unprivileged `yavdr` using the separately documented
   daemon installer and its `check`/migration safeguards; operator approves
   installation.
4. Verify capabilities and public device reads on the installed backend.
5. Only then install the matching VIDAA static app using the existing
   `scripts/install-hevc-trickplay-trial.sh`, backing up, HTTPS byte checking
   and retaining rollback; no TV firmware/launcher replacement.
6. Real Hisense acceptance: query Hobbit in upper and lower case, select a
   Recording, open EPG result and verify no false “backend_unavailable.”

Live-TV playback **is not provided by these search GETs**. The pre-existing
`LiveMediaSessionController` uses a browser MediaSession route, which is not
safe to expose to a paired TV client without dedicated Public-v1 device
authorization, MediaAccess credential binding and lifecycle tests. Do not
wire it directly to TV or expose stream URLs.

### Person matches (additive projection)

Both authorized search routes return `personProjectionVersion: 1` and per-item `matchedPerson`, `matchedRole`, `matchReason`, and `personKey` from the existing global search owner. The key uses the same Unicode folding and role as the Web people grouping. Clients combine the already authorized recording and EPG pages in key order, counting only returned matches. No extra permission, second matcher, native ID or filesystem path is introduced. Older servers lack the version marker. Recording subtitles and EPG channel name/description are also projected.

### Search card data (additive projection)

Both search result types also expose `durationSeconds` and advisory
`artworkAvailable` from the same Web search index. Recording hits expose
`startTime`, `sizeMb` and `edited`, with the existing canonical `rec_` identity.
The availability hint is not an artwork grant or proof that a byte resource
is currently available. Use the existing Public-v1 artwork resources;
no native recording path, native ID or private URL is returned. This adds no
new index query and does not yet replace the combined browser search dialog.

### Cached person portraits in Recording search

A Recording search hit may carry `personPortrait: {recordingId, artworkId}`
for its matched person. The reference uses the existing backend-scoped,
bulk portrait lookup, complete cast ordinal, assignment revision and the
same opaque image identity as Recording detail and its binary artwork GET.
No additional index, provider request or per-person profile query is needed.
The referenced recording can differ from the search hit; its current backend
identity must resolve. Missing/stale portraits do not remove valid hits.
EPG search never emits Recording portrait references. Existing independent
`recordings.view` and `epg.view` authorization remains unchanged.

### Preferred artwork in Recording search

Recording hits optionally include `preferredArtworkId`, referencing the existing
individual Recording artwork GET with the hit's `recordingId` and `backendId`.
The shared search query already joins the selected manual and persisted native
metadata. Its image evidence now includes native dimensions and the selected
manual revision, so the same existing opaque asset identity is generated without
additional per-result metadata lookups. Selected manual metadata has precedence;
missing/stale images do not remove a hit, and no private paths are returned.
`artworkAvailable` remains an advisory hint from the shared index.
