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

## Phase 49.6 Search Mode Baseline Regression

Phase 49.6 added explicit baseline regression coverage for the current `/api/epg/search` search behavior.

Verified behavior:

- default text search is case-insensitive
- default text search behaves as phrase/contains matching
- router-level search for `TATORT` matches the local `Tatort` EPG event
- controller-level search for `TATORT` matches the local `Tatort` EPG event
- unrelated `Tagesschau`/`event-time` results are excluded by the text query

Known gap:

- `EpgSearchQuery` defines search modes such as phrase, all words, any word, exact, regular expression and fuzzy.
- `EpgSearchMatcher` currently does not evaluate the mode field.
- `EpgSearchRequestMapper` currently does not map a search-mode request parameter.
- Advanced search-mode implementation remains a future phase.

## Phase 49.7 Deterministic Search Modes

Phase 49.7 implemented and tested deterministic EPGSearch modes.

Covered behavior:

- `mode=exact` matches a complete selected field value.
- `mode=all` requires all query words to be present across the selected search fields.
- `mode=any` requires at least one query word to be present across the selected search fields.
- default behavior remains case-insensitive phrase/contains search.
- unsupported modes are rejected at the REST/controller boundary.

Deferred behavior:

- regular-expression mode remains a separate safety decision.
- fuzzy mode remains a separate scoring/tolerance decision.

## Phase 49.8 Regex Mode Safety Decision

Phase 49.8 records the regex-mode safety contract before enabling regex execution.

Decision coverage:

- `mode=regex` is the intended public mode name.
- invalid regex input must become a stable HTTP 400 contract.
- invalid regex input must not crash the process.
- invalid regex input must not fall back to phrase matching.
- default phrase/contains search remains unchanged.
- exact/all/any deterministic modes remain independent from regex.
- fuzzy search remains a future decision.

Implementation coverage is intentionally deferred to the follow-up regex implementation phase.

## Phase 49.9 Regex Mode Implementation

Phase 49.9 implements and tests explicit regex search mode.

Covered behavior:

- `mode=regex` is accepted by the REST/controller boundary.
- valid regex patterns are evaluated by `EpgSearchMatcher`.
- invalid regex patterns return HTTP 400.
- invalid regex patterns do not fall back to phrase matching.
- default phrase/contains behavior remains unchanged.
- exact/all/any deterministic modes remain independent from regex.

Deferred behavior:

- fuzzy search remains a separate scoring and tolerance decision.
- performance limits for complex real-world regex patterns may be refined after real VDR validation.

## Phase 49.10 Fuzzy Mode Decision

Phase 49.10 records the fuzzy-mode decision before enabling fuzzy execution.

Decision coverage:

- mode=fuzzy is the intended public mode name.
- fuzzy uses a numeric tolerance parameter.
- native epgsearch adapters may map fuzzy to mode 5 plus tolerance.
- VDR-Suite may implement a backend-neutral fallback matcher.
- fallback fuzzy initially remains boolean match/no-match.
- ranking/scoring is intentionally deferred.
- invalid tolerance handling is required for the implementation phase.

Implementation coverage is intentionally deferred to the follow-up fuzzy implementation phase.

## Phase 49.11 Fuzzy Fallback Matcher

Phase 49.11 implements and tests the backend-neutral fuzzy fallback path.

Covered behavior:

- `mode=fuzzy` is accepted by the REST/controller boundary.
- `tolerance=<int>` is forwarded from router to controller.
- omitted tolerance defaults to 1.
- negative tolerance is rejected with HTTP 400.
- non-integer tolerance is rejected with HTTP 400.
- fuzzy fallback is deterministic and boolean.
- fuzzy fallback does not introduce public ranking or score output.
- request and mapper transport fuzzy tolerance to the domain query.

Deferred behavior:

- native epgsearch fuzzy passthrough.
- backend capability distinction between native and fallback fuzzy.
- public fuzzy score/ranking semantics.

## Phase 49.12 Native Fuzzy Capability Mapping

Phase 49.12 covers the capability distinction introduced after the fuzzy fallback matcher.

Covered behavior:

- `epg.search.fuzzy.fallback` is part of the default capability report.
- `epg.search.fuzzy.native` is part of the default capability report.
- empty backends report both fuzzy capabilities as unsupported.
- snapshot read-only capability sets report fallback fuzzy as available.
- snapshot read-only capability sets keep native fuzzy unsupported.
- the capability controller serializes both fuzzy capability names.

Deferred behavior:

- native RESTfulAPI/epgsearch fuzzy passthrough.
- real-backend native fuzzy detection.
- runtime capability merging between fallback and native execution paths.

## Phase 49.13 Native Fuzzy Adapter Passthrough

Phase 49.13 covers native fuzzy passthrough at the SearchTimer/RESTfulAPI boundary.

Covered behavior:

- create request parser accepts `mode=fuzzy`.
- update request parser accepts `mode=fuzzy`.
- create request parser accepts `tolerance=<int>`.
- update request parser accepts `tolerance=<int>`.
- `mode=fuzzy` maps to native epgsearch mode 5.
- RESTfulAPI mapper reads native `mode=5` and `tolerance` into SearchTimer match options.
- RESTfulAPI command executor sends native `mode=5` and `tolerance` during create.
- RESTfulAPI command executor sends native `mode=5` and `tolerance` during update.

Deferred behavior:

- real-backend validation against an installed epgsearch backend.
- backend-specific native fuzzy capability detection beyond the static capability flag.

## Phase 49.14 Native Fuzzy Real-Backend Validation

Phase 49.14 adds a real-backend validation harness for native fuzzy SearchTimer passthrough.

Covered behavior:

- dry-run mode shows the exact payload without sending requests.
- execution mode creates a temporary SearchTimer with `mode=5`.
- execution mode sends a configurable `tolerance`.
- readback mode verifies native `mode=5`.
- readback mode verifies the configured tolerance.
- cleanup mode deletes the temporary SearchTimer by native id.
- basic-auth parameters are available for protected RESTfulAPI endpoints.

Deferred behavior:

- automatic native fuzzy capability detection.
- permanent capability state changes based on validation outcome.
- CI execution against a real VDR instance.

## Phase 49.15 Native Fuzzy Capability Autodetection

Phase 49.15 adds deterministic native fuzzy capability detection rules.

Covered behavior:

- successful complete probe enables `epg.search.fuzzy.native`.
- failed tolerance preservation keeps native fuzzy disabled.
- failed cleanup keeps native fuzzy disabled.
- detector preserves unrelated base capabilities.
- validator emits a machine-readable capability probe result on successful real-backend validation.

Deferred behavior:

- automatic runtime storage of probe results.
- automatic backend registry capability mutation.
- scheduled or startup probing.
