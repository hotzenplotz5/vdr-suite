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
`available`), `source` (`none`, `restfulapi-scraper-bridge` or
`manual`) and `contentKind` (`unknown`, `movie`, `series`,
`series-episode`). Additional nonempty fields may include `epgTitle`,
`subtitle`, `epgDescription`, `providerTitle`, `overview`, `originalTitle`, `tagline`,
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

M1 work branch: `work/public-v1-m1-metadata-20261022`, originally based on
`6c05493ad53c82e19fef4a2bc70e09bc1603e0c8`, with E1 main
`ea7acfec7dd1b1f20aa49a479e3ca4a48bab0418` integrated in
M1-only merge commit `0dcb2d9c0283a2bc2fca7a253b8342fdc729a4c1`.
This branch is an isolated parallel development allowance, **not** permission
to merge before the strict E1 → E2 → M1 sequence.

The new detail uses the existing backend-bound identity repository, cache,
presentation projector, security gate, and read callback. Tests cover the
metadata-state projection, native/provider mixing, UTF-8 truncation, identity
and backend scope, public response schema, malformed inputs, missing identities,
permission denials, and source inconsistencies. CI is distinct from actual
yaVDR/device acceptance.

Before integration, recheck `main` and open S1 PRs #514/#516; E1 PR #519
has already merged. Shared `api/openapi.json`, `PublicApiRuntime` and daemon
files require final-head integration review.
Review/reconcile only the M1 hunks, preserve other work, rerun the final-head
tests, and do not merge automatically.

## M1 parity and remaining read-model gaps (isolated development)

| Existing Web/backend fact | Public-v1 detail projection | M1 disposition |
| --- | --- | --- |
| Recording display name and edited cut marker | Same `PublicRecordingCollectionProjection` / `PublicRecordingDisplayName` | Implemented, including path-derived title when native title/media ID is missing |
| Recording ID, backend scope and deleted/moved source | Same persisted `rec_` identity, cache and rebind evidence, independently authorized `recordings.view` | Implemented; playback still requires playable media ID |
| Native EPG title/subtitle/description | Bounded native `metadata.*` fields; duplicate description omitted when identical to collection excerpt | Implemented |
| Scraper source, provider title, series/episode, genre text, dates | Bounded existing `VdrRecordingMetadata` values, only when known provider source is present | Implemented; no new provider or artwork URL |
| Manual locked metadata assignment | Web `MetadataRepository` reads `ManualRecordingMetadataAssignmentRepository`, keyed by backend + canonical recording resource key | Implemented on the M1 branch: same read-only source, active target binding, `mdasg_`/`mdent_` and revision; no private resource keys, provider IDs or image paths |
| Manual genre selection / series hierarchy overrides | Web `GenreIndexRepository` and `RecordingSeriesHierarchyOverrideRepository` | **Not projected in detail yet**; separate focused read integration required |
| Artwork and image cache | Web poster/image references and M2 public-artwork work | Out of scope for M1 |

The legacy `MetadataRepository::getMetadataForRecording(int)` is **not** a
suitable public ID resolver: its integer legacy recording identity does not
provide the exact authorized backend/`rec_` binding. Do not expose it directly
or introduce a second identity or metadata database.

A selected manual metadata assignment already owns Suite opaque `mdent_`,
`mdasg_`, and `mdtgt_` identities with a revision. The M1 detail read uses
`MetadataRepository::getManualRecordingMetadataReadOnly`, which does not create
metadata schemas or materialize portraits. It requires an active backend-target
binding, selected locked relationship, valid IDs and positive revision. Manual
selection wins over automatically scraped provider fields, but does not rewrite
the public Recording title or native EPG fields. The HTTP metadata object
emits `manualAssignmentId`, `metadataEntityId` and `manualRevision` **only** for
`source=manual`. Backend-native keys, provider/external IDs, actor and image
references are withheld. Manual genre assignment remains a separate gap.

Competitor baseline: Kodi/Plex metadata detail and provenance behavior has
**not been verified in this branch**; this implementation prioritizes actual
VDR-Suite Web parity and explicit availability over speculative competitor
fields. No migration or installed-device parity is claimed from hosted CI.

Recommended remaining M1 slices:

- M1.2: source implementation complete; validate exact-head CI and compare
  manual assignment Web/authorized Public-v1 responses on controlled fixtures.
- M1.3: integrate canonical genre and optional series-hierarchy overrides
  only through the existing server repositories, with backend/security parity.
- M1.4: targeted authorization, malformed-data, manual-override and Web
  comparisons; prepare E1/E2/S1 merge reconciliation without merging them.

