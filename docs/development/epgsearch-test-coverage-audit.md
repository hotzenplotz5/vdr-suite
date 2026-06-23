# EPGSearch Test Coverage Audit

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [EPGSearch Capability Matrix](epgsearch-capability-matrix.md)
- [EPGSearch Query Alignment Audit](epgsearch-query-alignment-audit.md)
- [Current Status](current-status.md)

---

## Purpose

Phase 49.0 audits the EPGSearch test landscape after the request-to-query mapper was introduced.

The goal is to prevent duplicate test tracks and avoid adding a new endpoint regression test before the existing old and new EPGSearch tests are understood.

## Repository Findings

The repository currently contains two visible EPGSearch test naming families.

### Older underscore-style tests

- `core/vdr/tests/test_epg_search_request.cpp`
- `core/vdr/tests/test_epg_search_matcher.cpp`
- `core/vdr/tests/test_epg_search_service.cpp`

These tests were introduced around the earlier `EpgSearchRequest`-based EPG search architecture.

### Newer compact epgsearch-style tests

- `core/vdr/tests/test_epgsearch_query.cpp`
- `core/vdr/tests/test_epgsearch_request_mapper.cpp`
- `core/vdr/tests/test_epgsearch_matcher.cpp`
- `core/vdr/tests/test_epgsearch_service.cpp`
- `core/vdr/tests/test_epgsearch_result_json_serializer.cpp`

These tests cover the newer `EpgSearchQuery`, `EpgSearchRequestMapper`, `EpgSearchMatcher`, `EpgSearchService` and `EpgSearchResultJsonSerializer` flow.

## Existing Endpoint Coverage

`ApiRouter` already routes `GET /api/epg/search`.

`test_api_router.cpp` currently covers other EPG routes such as:

- `/api/epg/now-next`
- `/api/epg/time-window`
- `/api/epg/channel-window`

The local repository search did not show an explicit `/api/epg/search` assertion in `test_api_router.cpp`.

## Risk

Adding a new `/api/epg/search` endpoint regression immediately would leave the old underscore-style tests unresolved.

That risks keeping two partially overlapping EPGSearch test tracks:

- old tests around `EpgSearchRequest`
- new tests around `EpgSearchQuery` and the mapper

## Decision

Do not add another endpoint regression in Phase 49.0.

Use Phase 49.0 to document the test coverage state and make the next phase a consolidation step.

## Recommended Next Phase

Phase 49.1 - EPGSearch test consolidation

Expected work:

- decide whether old underscore-style tests are still valid
- migrate valid old tests to the new query/mapper architecture
- remove or retire redundant old tests
- add explicit `/api/epg/search` router coverage only after the test model is consistent

## Back

- [Back to Development Index](index.md)
- [Back to EPGSearch Capability Matrix](epgsearch-capability-matrix.md)

## Phase 49.1 Matcher Test Consolidation Result

Phase 49.1 started consolidation of the old and new EPGSearch test tracks.

Observed test state:

- `test-epg-search-request` still passes
- `test-epg-search-matcher` no longer compiles because it still uses `EpgSearchRequest` directly against `EpgSearchMatcher`
- the current matcher boundary is `EpgSearchQuery`
- `test-epgsearch-matcher` is the active matcher test for the new architecture

Migrated coverage:

- no-search-field behavior was added to `test_epgsearch_matcher.cpp`

Intentionally not migrated:

- old fuzzy example `tator` matching `Tatort`

Reason:

- `EpgSearchQuery` models search mode and fuzzy tolerance
- `EpgSearchMatcher` does not implement fuzzy semantics yet
- the capability matrix already marks fuzzy matching semantics as intentionally not implemented yet

Decision:

- old underscore-style matcher test is obsolete as an executable test in its current form
- its remaining useful cases must be migrated only when their domain semantics are implemented

## Phase 49.2 Service Test Consolidation Result

Phase 49.2 continued consolidation of the old and new EPGSearch test tracks.

Observed test state:

- `test-epg-search-service` is request-era coverage
- the current service boundary is `EpgSearchQuery`
- `test-epgsearch-service` is the active service test for the new architecture

Migrated coverage:

- backend-scoped query metadata is covered without asserting match backend propagation
- channel interval filtering is covered
- duration-window filtering is covered

Intentionally not migrated:

- paging
- sorting
- request window metadata

Reason:

- paging and sorting are API/request concerns of `EpgSearchRequest`
- the current `EpgSearchService` receives only `EpgSearchQuery`
- backend id is currently query metadata and not match backend propagation in this service

Decision:

- old underscore-style service test is obsolete as an executable service test in its current form
- its API/request-specific cases should move to mapper/controller/router tests instead of the domain service test

## Phase 49.3 Legacy Test Retirement Result

Phase 49.3 retired obsolete underscore-style EPGSearch matcher and service tests.

Retired:

- `test-epg-search-matcher`
- `test-epg-search-service`
- `core/vdr/tests/test_epg_search_matcher.cpp`
- `core/vdr/tests/test_epg_search_service.cpp`

Kept:

- `test-epg-search-request`
- `core/vdr/tests/test_epg_search_request.cpp`

Reason:

- `EpgSearchRequest` still exists as API/controller request model
- old matcher/service tests used `EpgSearchRequest` directly against `EpgSearchMatcher` and `EpgSearchService`
- current matcher/service boundaries use `EpgSearchQuery`
- equivalent active coverage now lives in compact `test_epgsearch_*` tests

## Phase 49.4 Endpoint Regression Result

Phase 49.4 added explicit router-level coverage for `GET /api/epg/search`.

Covered route:

- `/api/epg/search?query=Router&backend=living-room&channelId=router-channel-1&from=123&timespan=3600&limit=10&offset=0&sort=title&order=asc`

Covered assertions:

- HTTP 200
- JSON content type
- `matches` response array
- nested event id
- nested channel id
- nested title

This verifies the router-to-controller-to-query-mapper-to-service-to-serializer path.

## Phase 49.5 Parameter Regression Result

Phase 49.5 added explicit router-level regression coverage for invalid `/api/epg/search` parameters.

Covered invalid cases:

- `timespan=0` returns HTTP 400.
- `limit=-1` returns HTTP 400.
- `offset=-1` returns HTTP 400.
- `sort=unknown` returns HTTP 400.
- `order=sideways` returns HTTP 400.

The positive endpoint regression from Phase 49.4 remains in place.
