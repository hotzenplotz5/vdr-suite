# Public-v1 M1 — Recording metadata detail (read-only)

M1 exposes exactly one backend-qualified read-only resource, without adding a
metadata repository, provider, cache or public Recording identity owner.

```http
GET /api/v1/recordings/{recordingId}?backendId=backend-a
```

`recordingId` is the existing opaque Suite `rec_` ID, never a native VDR
filename. `backendId` is mandatory, exactly once. A supported authenticated
actor and `recordings.view` on that exact backend are required; the existing
SecurityHttpGate is the authorization and audit authority. The daemon uses the
existing `PublicRecordingPlaybackTargetResolver` and the warmed
`VdrRecordingCacheRepository`; no legacy fallback or new loader is allowed.

The response retains the existing collection item fields at its root:
`recordingId`, `backendId`, `title`, `recordedAt`, `durationSeconds`,
`durationKnown`, `edited`, `description`, `sizeMb`. A single optional-fact
container adds detail without repeating the collection:

```json
{
  "recordingId": "rec_0123456789abcdef0123456789abcdef",
  "backendId": "backend-a",
  "title": "Example",
  "recordedAt": "2026-10-20T20:15:00Z",
  "durationSeconds": 3600,
  "durationKnown": true,
  "edited": false,
  "description": "Description",
  "sizeMb": 2048,
  "metadata": {
    "state": "available",
    "source": "restfulapi-scraper-bridge",
    "contentKind": "series-episode",
    "epgTitle": "EPG title",
    "subtitle": "EPG subtitle",
    "seriesTitle": "Series",
    "episodeTitle": "Episode",
    "genreText": "Drama",
    "seasonNumber": 3,
    "episodeNumber": 7
  }
}
```

The metadata object always provides `state` (`absent`, `partial`,
`available`), `source` (`none` or
`restfulapi-scraper-bridge`) and `contentKind` (`unknown`, `movie`,
`series-episode`). Additional nonempty fields may include `epgTitle`,
`subtitle`, `epgDescription`, `originalTitle`, `tagline`,
`seriesTitle`, `episodeTitle`, `genreText`, `releaseDate`; positive
fields may include `seasonNumber`, `episodeNumber` and
`providerRuntimeMinutes`. An absent metadata state returns HTTP 200 with
no invented provider fields, provided the recording exists. The existing
collection's description preference and bounds remain authoritative;
native EPG text remains separately identifiable.

The response **never** contains native VDR paths, backend-native recording
IDs, scraper entity IDs, provider URLs, or raw artwork cache references.
Artwork delivery is reserved for M2. No manual scraper mutations, artwork
fetch, forced enrichments, or Recording writes occur. `Cache-Control:
no-store` is enforced. No ETag/304 guarantee is introduced without an
authoritative Recording *metadata* revision.

Errors: 400 invalid ID/backend query, 401 unauthenticated, 403 backend
permission denied, 404 missing/retired public identity, 503 unavailable
backend/cache or inconsistent projection. Standard Public-v1 Problem Details
and request/correlation response handling are reused.

## Acceptance and integration boundary

M1 work branch: `work/public-v1-m1-metadata-20261022`, based on verified
`main` commit `6c05493ad53c82e19fef4a2bc70e09bc1603e0c8`.
This branch is an isolated parallel development allowance, **not** permission
to merge before the strict E1 → E2 → M1 sequence.

The new detail uses the existing backend-bound identity repository, cache,
presentation projector, security gate, and read callback. Tests cover the
metadata-state projection, native/provider mixing, UTF-8 truncation, identity
and backend scope, public response schema, malformed inputs, missing identities,
permission denials, and source inconsistencies. CI is distinct from actual
yaVDR/device acceptance.

Before integration, recheck `main` and open E1/S1 PRs: #519, #514, #516
change `api/openapi.json` and shared `PublicApiRuntime` and daemon files.
Review/reconcile only the M1 hunks, preserve other work, rerun the final-head
tests, and do not merge automatically.
