# Web ↔ Public-v1 search parity (VIDAA / future clients)

## Binding decision

The existing Webfrontend `GET /api/search` and the authenticated
`GET /api/v1/search/recordings` and `/api/v1/search/epg` MUST all use
`GlobalSearchApiRuntime::searchPublic` / the same `GlobalSearchService`
and persistent SQLite matcher. No separate client search index, title
predicates, regex implementation or discovery enumeration.

The Public-v1 path is an **authorized, bounded projection** only:
omit file paths, provider URLs and native IDs; supply opaque public recording
IDs and non-sensitive display data. Preserve the effective **search** title
(including selected manual metadata), order, query folding, case handling,
EPG subtitles and group totals. Verify the current snapshot for Recording
identities; never return an obsolete/deleted recording. A stale individual
match is filtered rather than turning surviving matches into HTTP 503.
A missing shared search service or missing recording snapshot remains a
fail-closed unavailable response.

## Mandatory parity fixture

| Scenario | Web baseline | Public-v1 acceptance |
| --- | --- | --- |
| `Hobbit` recording query | Three existing Hobbit recordings | Same three titles and ordering, public IDs |
| `hobbit` / `HOBBIT` | Case-insensitive shared folding | Matching result set |
| Selected manual recording title | Effective search title | Identical display title, never native-only fallback |
| `Hobbit` EPG query | Two Young Sheldon subtitle matches | Both events with title and subtitle |
| One stale/deleted search hit | Other hits remain discoverable | Stale entry omitted; remaining hits successful |
| Paired TV without grant | Server authorization required | 403, never a no-auth fallback |
| Invalid or absent shared search | No invented success payload | Fail closed; reason classified in daemon log |
| Pagination | Canonical shared search order | Limit 30, preserved page order |
| Sensitive native data | Internal to Web/service | Must never appear in Public-v1 JSON |

Automation: `make test-public-v1-search-web-parity` exercises the production
Public-v1 projection on a canonical Web result fixture. The established
`test-global-search-repository`, `test-global-search-controller` and
`test_public_recording_collection` separately validate index/search and
Public-v1 HTTP authorization/formatting.

## Competitor benchmark (scope, not an implementation mandate)

Before accepting a client-facing search change, check current, sourced Kodi
and Plex behavior for global search, live TV/EPG discovery and recording
presentation, and note verified gaps. Do not claim unverified competitor
capabilities. A competitor comparison never replaces Web parity, security
checks, or real Hisense acceptance.

## Installed yaVDR verification

Separate code tests from deployed behavior. Check the exact daemon source
identity before executing a Device-authorized `/api/v1/search/recordings`
request. Compare against authorized `/api/search` results with the same
query, backend, and EPG time window. Confirm zero native paths in the TV
response, three Hobbit recording results and two EPG subtitle results.
Search-specific errors must carry a non-secret reason in the daemon log.
Do not alter permissions or SQLite grants directly during the parity test.
