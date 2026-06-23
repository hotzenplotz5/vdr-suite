# EPGSearch Capability Matrix

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Live / EPGSearch Feature Inventory](live-feature-inventory.md)
- [Current Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

Phase 48.1 maps Live/EPGSearch capabilities to RESTfulAPI and VDR-Suite.

The goal is to decide which Live goldstandard features should become first-class VDR-Suite capabilities.

---

## Source Findings

EPGSearch service capabilities include:

- Epgsearch-search-v1.0
- Epgsearch-searchresults-v1.0
- SearchTimerList
- AddSearchTimer
- ModSearchTimer
- DelSearchTimer
- QuerySearchTimer
- QuerySearch
- ExtEPGInfoList
- ChanGrpList
- BlackList
- DirectoryList
- TimerConflictList
- IsConflictCheckAdvised
- ShortDirectoryList
- Evaluate

RESTfulAPI epgsearch exposes rich SearchTimer fields and validation rules, including:

- search
- mode
- tolerance
- match_case
- use_title
- use_subtitle
- use_description
- content_descriptors
- use_ext_epg_info
- ext_epg_info
- use_in_favorites
- time filters
- channel filters
- duration filters
- day-of-week filters
- blacklist mode and ids
- repeat avoidance
- comparison flags
- recording retention and deletion policy

---

## Capability Matrix

| Capability | Live / EPGSearch | RESTfulAPI | VDR-Suite Status | Priority |
| --- | --- | --- | --- | --- |
| Basic EPG search | yes | likely via events/search capabilities | partial | high |
| Search result list | yes | available through EPG/event output | partial | high |
| Search modes | phrase/and/or/regex plus RESTfulAPI fuzzy support | mode 0-5 | partial | high |
| Title/subtitle/description flags | yes | yes | present in SearchTimer, unclear in EPG query | high |
| Channel filter | channel number / channel id / group semantics | use_channel, channel_min/max, channels | partial | high |
| Time filter | yes | start_time/stop_time | partial | medium |
| Duration filter | yes | duration_min/max | present in SearchTimer | medium |
| Day-of-week filter | yes | dayofweek | present in SearchTimer | medium |
| Extended EPG categories | ExtEPGInfoList | ext_epg_info | not first-class | high |
| Content descriptors | yes | content_descriptors | present in SearchTimer write/read | medium |
| Favorites | use_in_favorites | yes | present in SearchTimer | medium |
| Blacklists | BlackList | blacklist_mode / blacklist_ids | fields present, list capability missing | high |
| Channel groups | ChanGrpList | channels when use_channel group | not first-class | high |
| Directories | DirectoryList / ShortDirectoryList | directory fields | partial | medium |
| SearchTimer CRUD | SearchTimerList/Add/Mod/Del | yes | implemented and real-VDR tested | done |
| SearchTimer query preview | QuerySearchTimer | likely not first-class | partial | high |
| Ad-hoc query search | QuerySearch | likely not first-class | partial | high |
| Timer conflicts | TimerConflictList | unclear | missing | high |
| Conflict-check advice | IsConflictCheckAdvised | unclear | missing | high |
| Expression evaluation | Evaluate(expr,event) | unclear | missing | low/advanced |

---

## Gap Classification

### Already strong

- SearchTimer read/write model
- SearchTimer real VDR validation
- Timer lifecycle real VDR validation
- RESTfulAPI read-only regression
- unified real VDR regression command

### Needs mapping before implementation

- EPG search modes
- EPG result semantics
- channel-group semantics
- extended EPG category semantics
- blacklist discovery
- SearchTimer query preview
- ad-hoc EPGSearch query
- timer conflict reporting

### Should not be implemented blindly

- conflict detection
- expression evaluation
- TVScraper-related enrichment
- series/episode semantics

These need exact upstream source and real-backend behavior validation first.

---

## Recommended Next Phases

### Phase 48.2 - Backend-neutral EPGSearch query model

Define a domain model for EPGSearch-like query capabilities without binding it to one backend.

Required fields:

- query
- search mode
- target fields
- channel scope
- time scope
- duration scope
- day-of-week scope
- extended EPG categories
- content descriptors
- favorite scope

### Phase 48.3 - EPGSearch result semantics

Define a backend-neutral result object for EPGSearch-style query results.

### Phase 48.4 - EPGSearch real VDR query smoke

Add a safe real-VDR read-only query smoke for ad-hoc searches.

### Phase 48.5 - Timer conflict capability analysis

Audit EPGSearch conflict service and determine whether RESTfulAPI can expose it or needs extension.

---

## Decision

The next implementation should not start with conflicts or TVScraper.

The safest and highest-value continuation is a backend-neutral EPGSearch query model followed by a real VDR ad-hoc EPGSearch query smoke.

## Back

- [Back to Development Index](index.md)
- [Back to Live / EPGSearch Feature Inventory](live-feature-inventory.md)

## Phase 48.2 Query Model Result

Phase 48.2 introduced a backend-neutral EPGSearch query model.

The model covers:

- search text
- search mode
- fuzzy tolerance
- title/subtitle/description field selection
- case sensitivity
- channel interval, channel group and free-to-air scopes
- time window
- duration window
- day-of-week filter
- extended EPG info values
- content descriptors
- favorites-only scope

No REST endpoint, adapter or backend execution was added in this phase.

## Phase 48.3 Result Model Audit

Phase 48.3 audited the existing EPGSearch result domain model.

Finding:

- `EpgSearchMatch` already wraps a `VdrEvent`, optional backend identity and matched fields.
- `EpgSearchResult` already provides matches, total count, returned count, limit and offset.
- No replacement model is needed before introducing the EPGSearch service interface.

The next missing abstraction is a backend-neutral service boundary.

## Phase 48.4 Service Boundary Result

Phase 48.4 introduced a backend-neutral EPGSearch service boundary.

Implemented:

- `EpgSearchService`
- `search(events, query) -> EpgSearchResult`
- text matching across title/subtitle/description
- explicit field selection support
- case-sensitive and case-insensitive matching behavior

Not implemented in this phase:

- REST endpoint
- RESTfulAPI adapter
- real VDR EPGSearch execution
- fuzzy matching semantics
- channel/time/duration/day/category filtering

Those belong to later matcher and adapter phases.

## Phase 48.5 Matcher Extraction Result

Phase 48.5 extracted EPGSearch text matching into `EpgSearchMatcher`.

Implemented:

- dedicated matcher class
- title/subtitle/description matching
- field-selection aware matching
- case-sensitive and case-insensitive matching
- service delegation from `EpgSearchService` to `EpgSearchMatcher`

Not implemented in this phase:

- channel filtering
- time filtering
- duration filtering
- day-of-week filtering
- extended EPG category filtering
- fuzzy matching semantics

Those belong to later matcher expansion phases.

## Phase 48.6 Matcher Filter Expansion Result

Phase 48.6 expanded `EpgSearchMatcher` beyond text matching.

Implemented with existing `VdrEvent` fields:

- channel interval matching through event channel id
- duration window matching through event duration seconds
- content descriptor matching through event content descriptors

Intentionally not implemented yet:

- extended EPG category filtering
- channel group semantics
- favorites semantics
- time-window semantics
- day-of-week semantics
- fuzzy matching semantics

Those require additional domain fields or exact backend semantics before implementation.

## Phase 48.7 Result Serializer Result

Phase 48.7 added JSON serialization for backend-neutral EPGSearch results.

Implemented:

- result metadata serialization
- match list serialization
- backend id serialization
- matched fields serialization
- nested VDR event serialization
- content descriptor array serialization
- JSON string escaping test coverage

No REST controller or route was added in this phase.

## Phase 48.8 Query Alignment Audit

Phase 48.8 audited the relationship between `EpgSearchRequest` and `EpgSearchQuery`.

Finding:

- controller and route already exist
- `EpgSearchRequest` represents API/controller request concerns
- `EpgSearchQuery` represents backend-neutral search semantics
- they should be connected through an explicit mapper, not merged blindly

## Phase 48.9 Request Mapper Result

Phase 48.9 introduced an explicit mapper from `EpgSearchRequest` to `EpgSearchQuery`.

Implemented:

- `EpgSearchRequestMapper`
- request text to domain search text
- backend id mapping
- channel id to single-channel interval mapping
- title/subtitle/description field selection mapping
- controller alignment so `EpgSearchService` receives `EpgSearchQuery`

Intentionally not mapped yet:

- paging
- sorting
- request window metadata

Those remain API/request concerns and should not be forced into the domain query.

## Phase 49.0 Test Coverage Audit

Phase 49.0 audited EPGSearch test coverage after the request-to-query mapper was introduced.

Finding:

- old underscore-style EPGSearch tests still exist
- newer compact epgsearch-style tests now cover the query, mapper, matcher, service and serializer path
- `GET /api/epg/search` is routed, but explicit router regression coverage should wait until test consolidation

Decision:

- do not add another endpoint test yet
- consolidate the EPGSearch tests first

## Phase 49.1 Matcher Test Consolidation

Phase 49.1 consolidated active matcher coverage.

Result:

- active matcher tests use `EpgSearchQuery`
- obsolete request-based matcher tests were identified
- no-search-field behavior is now covered in the compact matcher test
- fuzzy tolerance remains modeled but not implemented in the matcher

## Phase 49.2 Service Test Consolidation

Phase 49.2 consolidated active service coverage.

Result:

- active service tests use `EpgSearchQuery`
- backend-scoped query metadata is covered
- channel interval filtering is covered
- duration-window filtering is covered
- paging and sorting remain API/request concerns

## Phase 49.3 Legacy Test Retirement

Phase 49.3 retired obsolete request-era matcher and service tests.

Result:

- request model coverage remains available through `test-epg-search-request`
- active matcher coverage remains available through `test-epgsearch-matcher`
- active service coverage remains available through `test-epgsearch-service`
- obsolete direct `EpgSearchRequest` to matcher/service tests were removed

## Phase 49.4 Endpoint Regression

Phase 49.4 added explicit regression coverage for the EPGSearch HTTP endpoint.

Result:

- `/api/epg/search` is now covered in `test_api_router.cpp`
- query/backend/channel/time-window/sort/order parameters are routed
- response JSON shape is verified through the nested `matches[].event` structure

## Phase 49.5 Parameter Regression

Phase 49.5 extended endpoint regression coverage with invalid parameter handling for `/api/epg/search`.

Result:

- invalid `timespan` is rejected
- invalid `limit` is rejected
- invalid `offset` is rejected
- invalid `sort` is rejected
- invalid `order` is rejected
