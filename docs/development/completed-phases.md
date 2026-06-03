# VDR-Suite – Completed Phases

This file keeps the longer phase history out of `docs/development/current-status.md`.

## Phase 8 status history

### Phase 8.0 – Daemon foundation

Implemented daemon foundation with `DaemonApp`, `RuntimeConfig`, signal handling, database lifecycle and dashboard runtime wiring.

### Phase 8.1 – External VDR adapter foundation

Implemented the first external VDR adapter foundation with `VdrStatus`, `ExternalVdrAdapter` and initial unit tests.

### Phase 8.2 – VdrConfig architecture

Introduced dedicated VDR configuration object and adapter configuration defaults.

### Phase 8.3 – IVdrAdapter abstraction layer

Introduced backend-independent VDR adapter contract.

### Phase 8.4 – VdrAdapterFactory

Introduced central adapter creation and backend selection layer.

### Phase 8.5 – MockVdrAdapter multi-backend foundation

Added deterministic mock backend and test coverage for backend-independent VDR integration.

### Phase 8.6 – VDR backend architecture documentation

Documented backend-independent VDR backend architecture and future adapter strategy.

### Phase 8.7 – RESTfulAPI integration architecture analysis

Documented RESTfulAPI adapter boundary, endpoint classification and first mapping direction.

### Phase 8.8 – HTTP abstraction layer

Introduced `IHttpClient`, `HttpRequest`, `HttpResponse`, `MockHttpClient` and HTTP abstraction tests.

### Phase 8.9 – RESTfulAPI VDR adapter foundation

Added `RestfulApiVdrAdapter`, `IHttpClient` injection and mocked `/info.json` mapping path.

### Phase 8.10 – VDR domain model documentation

Documented VDR domain model direction.

### Phase 8.11 – VDR domain objects

Introduced backend-neutral VDR domain objects.

### Phase 8.12 – VDR event domain object

Added `VdrEvent` with backend-neutral EPG fields and tests.

### Phase 8.13 – VDR event adapter architecture

Expanded adapter architecture for event access and mock event data.

### Phase 8.14 – RESTfulAPI event mapping foundation

Added RESTfulAPI event mapping foundation.

### Phase 8.15 – RESTfulAPI status mapping foundation

Added dedicated RESTfulAPI status mapper.

### Phase 8.16 – RESTfulAPI channel mapping foundation

Added RESTfulAPI channel mapper.

### Phase 8.17 – RESTfulAPI recording mapping foundation

Added RESTfulAPI recording mapper.

### Phase 8.18 – RESTfulAPI timer mapping foundation

Added RESTfulAPI timer mapper.

### Phase 8.19 – VDR service layer foundation

Introduced `VdrService` as service boundary between consumers and `IVdrAdapter` implementations.

### Phase 8.20 – VDR overview service foundation

Introduced `VdrOverviewService`.

### Phase 8.21 – VDR overview enrichment

Expanded VDR overview data for dashboard use.

### Phase 8.22 – VDR overview JSON serializer

Added `VdrOverviewJsonSerializer`.

### Phase 8.23 – VDR overview controller

Added `VdrController` overview endpoint handling.

### Phase 8.24 – VDR API router integration

Integrated VDR overview route into `ApiRouter`.

### Phase 8.25 – REST API runtime architecture

Documented REST API runtime architecture.

### Phase 8.26 – daemon VDR runtime integration

Integrated VDR runtime wiring into daemon architecture.

### Phase 8.27 – daemon API router integration

Integrated API router into daemon runtime direction.

### Phase 8.28 – HTTP server boundary contract

Introduced HTTP server boundary contract.

### Phase 8.29 – HTTP server boundary architecture documentation

Documented HTTP server boundary architecture.

### Phase 8.30 – daemon REST runtime architecture documentation

Documented daemon REST runtime architecture.

### Phase 8.31 – test HTTP server architecture documentation

Documented `TestHttpServer` architecture.

### Phase 8.32 – TestHttpServer runtime implementation

Implemented `TestHttpServer` runtime routing, response mapping, 404 propagation and 405 handling.

### Phase 8.33 – IHttpServer runtime ownership

Added `IHttpServer` ownership to `DaemonRuntime` and integrated `TestHttpServer` lifecycle.

### Phase 8.34 – RealHttpServer strategy

Added ADR for future production HTTP server strategy.

### Phase 8.35 – HTTP server factory strategy

Added ADR for future `HttpServerFactory` and `HttpServerConfig` direction.

### Phase 8.36 – Media platform and Library First VDR architecture

Documented media platform comparison, Library First direction, Multi-VDR requirements and source/capability/permission ideas.

### Phase 8.69 – PollingService interface

Introduced `PollingService` interface around snapshot polling.

### Phase 8.70 – PollingService implementation

Implemented `PollingService::poll()` and `PollingService::snapshot()`.

### Phase 8.72 – extract VDR source list into make include

Moved VDR source list into modular make include structure.

### Phase 8.74 – extract VDR test targets into make include

Moved VDR test targets into `mk/vdr-tests.mk`.

### Phase 8.75 – extract HTTP source list into make include

Moved HTTP source list into make include structure.

### Phase 8.76 – extract daemon source list into make include

Moved daemon source list into make include structure.

### Phase 8.77 – extract recording source lists into make include

Moved recording source lists into make include structure.

### Phase 8.78 – extract action and job source lists into make include

Moved action and job source lists into make include structure.

### Phase 8.79 – initial root Makefile include conversion

Started root Makefile conversion to modular includes.

### Fix 06667cf – Fix modular Makefile include conversion

Fixed modular Makefile include conversion while keeping the build functional.

Known remaining debt after this fix:

- duplicate VDR test targets still exist in the root Makefile and `mk/vdr-tests.mk`
- Phase 8.80 removes the duplicate root Makefile targets
