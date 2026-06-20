# VDR-Suite Completed Phases

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

This file tracks completed implementation phases.

Current status belongs to:

- [Current Project Status](current-status.md)

Future planning belongs to:

- [Roadmap](../planning/roadmap.md)
- [Milestones](../planning/milestones.md)

---

## Phase 46.27 - Recording Person Search Result Model

Status: Completed.

Summary:
- Adds a recording-person search result model that preserves both matched person metadata and the recording context.

## Phase 46.26 - Recording Additional Media Person Import

Status: Completed.

Summary:
- Imports RESTfulAPI additional_media actors into VdrRecording person collections as scraper-sourced actor metadata.

## Phase 46.25 - Real VDR Person Metadata Validation

Status: Completed.

Summary:
- Documents real VDR person metadata findings and identifies RESTfulAPI additional_media actors and modern TVScraper characters as the validated import path.

## Phase 46.24 - Person Query Documentation

Status: Completed.

Summary:
- Documents the routed person query API, parameters, validation, response contract, current empty data source, and recording-search limitations.

## Phase 46.23 - Person Query Router Wiring

Status: Completed.

Summary:
- Wires person query routing through ApiRouter and DaemonRuntime with empty result data until real metadata sources are connected.

## Phase 46.22 - Person Query REST Boundary

Status: Completed.

Summary:
- Extends the person REST controller with validated person query parameters backed by PersonSearchService and PersonQueryResultJsonSerializer.

## Phase 46.21 - Person Search Service

Status: Completed.

Summary:
- Adds a person search service that filters person collections with PersonQueryMatcher and returns paged query results.

## Phase 46.20 - Person Query JSON Contract

Status: Completed.

Summary:
- Adds a person query result model and JSON serializer for paged person search results.

## Phase 46.19 - Person Query Matcher

Status: Completed.

Summary:
- Adds a person query matcher for optional name, normalized name, role, source, and provider reference filters.

## Phase 46.18 - Person Query Model

Status: Completed.

Summary:
- Adds the first person query domain model with optional filters for name, normalized name, role, source, and provider reference.

## Phase 46.17 - Person API Documentation

Status: Completed.

Summary:
- Added person API documentation.
- Documented the person JSON contract and REST-facing controller boundary.
- Clarified that ApiRouter wiring, provider integration, persistence and search remain out of scope.

## Phase 46.16 - Person REST Boundary

Status: Completed.

Summary:
- Added PersonController as the first REST-facing person metadata boundary.
- Returned person resolution results as ApiResponse.
- Preserved the existing JSON contract through PersonResolutionJsonSerializer.
- Added isolated controller coverage without routing or provider integration.

## Phase 46.15 - Person JSON Contract

Status: Completed.

Summary:
- Added PersonResolutionJsonSerializer.
- Serialized resolved state, primary person and all person evidence.
- Exposed source, role, original name, normalized name, character name, confidence and provider reference.
- Kept REST, provider integration and search out of scope.

## Phase 46.14 - Person Resolution Model

Status: Completed.

Summary:
- Added PersonResolver.
- Added PersonResolutionResult.
- Preserved all person evidence while selecting a primary person.
- Preferred confidence, manual user entries, provider references and deterministic source priority.
- Kept JSON, REST, provider integration and search out of scope.

## Phase 46.13 - Person Domain Foundation

Status: Completed.

Summary:
- Added  as the first source-aware person metadata domain object.
- Added  for actor, director, writer, producer, moderator, guest, composer and generic roles.
- Added  as a multi-evidence container for future cast and crew metadata.
- Kept resolver, JSON, REST, provider integration and search out of scope.

## Phase 46.12 - Content Rating API Documentation

Status: Completed.

Summary:
- Added content rating API documentation.
- Documented the content rating JSON contract and REST-facing controller boundary.
- Clarified that ApiRouter wiring, persistence, provider integration, profile policy and enforcement remain out of scope.

## Phase 46.11 - Content Rating REST Boundary

Status: Completed.

Summary:
- Added `ContentRatingController` as the first REST-facing content rating boundary.
- Returned content rating resolution results as `ApiResponse`.
- Preserved the existing JSON contract through `ContentRatingResolutionJsonSerializer`.
- Added isolated controller coverage without routing or backend integration.
- Kept ApiRouter wiring, database, provider integration, profile policy and enforcement out of scope.

## Phase 46.10 - Content Rating JSON Contract

Status: Completed.

Summary:
- Added `ContentRatingResolutionJsonSerializer`.
- Serialized resolved state, primary rating and all rating evidence.
- Exposed source, rating system, original value, minimum age, confidence and provider reference.
- Preserved all rating evidence while exposing the selected primary rating.
- Kept REST, database, profile policy and enforcement out of scope.

## Phase 46.9 - Content Rating Resolution Model

Status: Completed.

Summary:
- Added `ContentRatingResolver`.
- Added `ContentRatingResolutionResult`.
- Preserved all content rating evidence while selecting a primary rating.
- Preferred user ratings as explicit manual overrides.
- Otherwise preferred explicit confidence, higher confidence, conservative minimum age and deterministic source priority.
- Kept JSON, REST, database, profile policy and enforcement out of scope.

## Phase 46.8 - Content Rating Domain Foundation

Status: Completed.

Summary:
- Added `ContentRating` as the first content rating evidence domain object.
- Added `ContentRatingSystem` for FSK, USK, TV parental guideline, provider-specific and user-defined rating systems.
- Added `ContentRatingCollection` as a multi-evidence container.
- Preserved original provider values, minimum age, optional confidence and optional provider references.
- Kept resolver, JSON, REST, database, profile policy and enforcement out of scope.

## Phase 46.7 - Genre Architecture Documentation

Status: Completed.

Summary:
- Added genre architecture documentation.
- Documented canonical genre IDs, source evidence, resolver behavior, JSON contract and localized labels.
- Documented the future `resources/i18n/<locale>/genres.json` direction.
- Clarified that clients should use canonical IDs for filtering and labels only for display.
- Kept REST, database, provider integration, SearchTimer and policy enforcement out of scope.

## Phase 46.6 - Genre Resolution Localization JSON

Status: Completed.

Summary:
- Added localized JSON serialization for genre resolution results.
- Added `serializeLocalized(result, locale)` to `GenreResolutionJsonSerializer`.
- Preserved the existing non-localized JSON contract.
- Added `label` and `locale` fields while keeping stable `canonicalId` values.
- Verified German and English labels for canonical genre IDs.
- Kept REST, database, external language files, provider integration, SearchTimer and policy enforcement out of scope.

## Phase 46.5 - Genre Localization Layer

Status: Completed.

Summary:
- Added `GenreLocalization`.
- Added first German and English labels for canonical genre IDs.
- Added locale prefix handling for values such as `de_DE` and `en_US`.
- Added English fallback for unsupported locales.
- Preserved canonical IDs as stable internal keys.
- Kept REST, database, external language files, provider integration, SearchTimer and policy enforcement out of scope.

## Phase 46.4 - Canonical Genre Registry

Status: Completed.

Summary:
- Added `CanonicalGenreRegistry`.
- Added first canonical IDs for crime, comedy, action, drama, documentary, children, sports, news, movie and series.
- Mapped German and English aliases such as `Krimi`, `Crime`, `Komödie`, `Comedy`, `Spielfilm` and `Movie`.
- Preserved unknown genres by normalizing them into stable fallback IDs.
- Prepared the future localization layer by separating canonical IDs from display labels.
- Kept REST, database, provider integration, SearchTimer and policy enforcement out of scope.

## Phase 46.3 - Genre JSON Contract

Status: Completed.

Summary:
- Added `GenreResolutionJsonSerializer`.
- Serialized resolved state, primary genre and genre evidence.
- Exposed `canonicalId`, `source`, `originalValue`, `normalizedValue`, `confidence` and `providerReference`.
- Prepared genre JSON for future localization by keeping canonical IDs separate from source display values.
- Kept REST, database, metadata provider integration, SearchTimer, localization files and policy enforcement out of scope.

## Phase 46.2 - Genre Source Resolution Model

Status: Completed.

Summary:
- Added `GenreResolver` as the first deterministic genre resolution boundary.
- Added `GenreResolutionResult` to preserve both the primary resolved genre and all source evidence.
- Preferred higher confidence when present.
- Used source priority as a deterministic tie-breaker.
- Preserved provider evidence instead of deleting lower-priority classifications.
- Kept REST, database, metadata provider integration, SearchTimer and policy enforcement out of scope.

## Phase 46.1 - Genre Domain Foundation

Status: Completed.

Summary:
- Added the source-aware `GenreClassification` domain foundation.
- Added `ContentClassificationSource` values for EPG, DVB, tvscraper, scraper2vdr, TMDb, TVDb, IMDb, user, folder and derived sources.
- Added `GenreCollection` as a multi-evidence container.
- Preserved original provider values, normalized values, optional confidence and optional provider references.
- Kept REST, database, metadata provider integration, SearchTimer and policy enforcement out of scope.

## Phase 46.0 - Content Classification Architecture ADR

Status: Completed.

Summary:
- Added ADR-0028 as the source-aware content classification architecture.
- Defined content classification as a general model instead of a genre-only model.
- Prepared multi-source genres, content ratings, keywords, collections, user tags and folder hints.
- Preserved VDR as source of truth while allowing metadata and user-owned enrichment.
- Prepared future policy, FSK/content rating, profile and TV frontend work without implementing enforcement.

## Phase 45.8 - EPG Search Documentation

Status: Completed.

Summary:
- Documented the implemented `/api/epg/search` REST contract.
- Documented parameters, validation rules, response shape, matching, sorting and pagination.
- Linked the EPG Search API documentation from README, Development Index and Phase 45 architecture documentation.
- Kept genres, SearchTimer, metadata providers and user policy work out of scope.

## Phase 45.7 - EPG Search REST Validation

Status: Completed.

Summary:
- Added REST validation for `/api/epg/search`.
- Rejected non-positive `timespan` values.
- Rejected negative `limit` and `offset` values.
- Rejected unsupported `sort` and `order` values.
- Returned JSON 400 responses for invalid EPG search parameters.
- Kept genres, SearchTimer and advanced policy work out of scope.

## Phase 45.6 - EPG Search Controller Foundation

Status: Completed.

Summary:
- Added EPG search controller wiring through `EpgController::search`.
- Added `/api/epg/search` router support.
- Kept existing EPG routes intact.
- Routed search over selective EPG time-window data through `EpgSearchService`.
- Updated runtime and test wiring for the EPG search controller.

## Phase 45.5 - EPG Search Service Foundation

Status: Completed.

Summary:
- Added `EpgSearchService`.
- Searched already provided `VdrEvent` windows instead of introducing full EPG loading.
- Combined request, matcher, match and result foundations.
- Added sorting, offset and limit handling.
- Kept REST routing, backend fetch orchestration, genres and SearchTimer work out of scope.

## Phase 45.4 - EPG Search JSON Contract

Status: Completed.

Summary:
- Added `EpgSearchResultJsonSerializer`.
- Serialized EPG search totals, returned count, limit, offset and result entries.
- Serialized event identity, backend identity, channel identity, title, subtitle, description, time fields, duration and matched fields.
- Added JSON escaping coverage for quotes, newlines and backslashes in EPG payload text.
- Added isolated serializer contract coverage through `test-epg-search-result-json-serializer`.
- Kept service wiring, REST routing, genres and SearchTimer work out of scope.

## Phase 45.3 - EPG Search Result Foundation

Status: Completed.

Summary:
- Added `EpgSearchMatch` as the per-event search match wrapper.
- Added `EpgSearchResult` as the backend-aware EPG search result foundation.
- Preserved event payloads while preparing backend identity and match metadata for future JSON and REST contracts.
- Added total, returned, limit and offset counters for future paged search responses.
- Added isolated result contract coverage through `test-epg-search-result`.
- Kept JSON serialization, REST routing, genres and SearchTimer work out of scope.

## Phase 45.2 - EPG Search Matcher Foundation

Status: Completed.

Summary:
- Added the isolated `EpgSearchMatcher` foundation.
- Matched `VdrEvent` objects against `EpgSearchRequest` without RESTfulAPI-specific URL semantics.
- Supported case-insensitive text matching across title, subtitle and description according to selected search fields.
- Supported channel matching and selective numeric time-window matching.
- Added isolated matcher contract coverage through `test-epg-search-matcher`.
- Kept sorting, REST routing, result modeling, genres and SearchTimer work out of scope.

## Phase 45.1 - EPG Search Request Foundation

Status: Completed.

Summary:
- Added the backend-neutral `EpgSearchRequest` domain foundation.
- Prepared future EPG text search across title, subtitle and description fields.
- Added selective channel, time-window, limit, offset and backend identity fields without RESTfulAPI-specific URL semantics.
- Added sort field and sort order foundations for future EPG search result ordering.
- Added isolated request contract coverage through `test-epg-search-request`.
- Kept VDR as source of truth and avoided persistent full EPG mirroring.

## Phase 44.30 - Recording Action Runtime Diagnostics Completion

Status: Completed.

Summary:
- Added structured upstream diagnostics to recording action execution results.
- Exposed `upstreamHttpStatus`, `upstreamEndpoint` and `upstreamResponseBody` in the execution result JSON contract.
- Preserved RESTfulAPI HTTP status, endpoint and response body for successful and failed upstream requests.
- Verified HTTP result mapping, recording action execution controller, recording action contract, daemon build, documentation checks and phase consistency.

## Phase 44.29 - Enrich Recording Action Execution Result

Status: Completed.

Summary:
- Added `backendNativeId`, `recordingPath` and `snapshotRefreshed` to the recording action execution result contract.
- Enriched controller responses from the resolved request and snapshot refresh outcome.
- Kept VDR snapshot state as the read-side source of truth.

## Phase 44.28 - Refresh Snapshot After Recording Action

Status: Completed.

Summary:
- Added a daemon runtime callback to refresh VDR snapshot state after successful recording action execution.
- Fixed the immediate second-rename problem after a successful rename changed the backend-native recording path.
- Verified successful back-to-back rename behavior against the real VDR runtime.

## Phase 44.27 - Normalize RESTfulAPI Rename Targets

Status: Completed.

Summary:
- Normalized RESTfulAPI rename targets to VDR folder-name semantics.
- Converted slashes to VDR folder separators and spaces to underscores where required by the RESTfulAPI move endpoint.
- Updated request-builder and contract tests for move and rename target behavior.

## Phase 44.25 - Resolve Recording Native ID From Snapshot

Status: Completed.

Summary:
- Resolved `backendNativeId` from the current VDR snapshot when action clients provide only `backendId` and `recordingId`.
- Wired the resolver into the recording action execution controller.
- Added controller coverage for resolved backend-native recording action execution.

## Phase 44.0 - Documentation Synchronization After Recording Action Live Verification

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
