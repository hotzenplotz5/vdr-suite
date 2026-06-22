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
