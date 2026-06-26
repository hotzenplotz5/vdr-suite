CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra \
        -Icore/sqlite/include \
        -Icore/recordings/include \
        -Icore/daemon/include \
        -Icore/vdr/include \
        -Icore/http/include \
        -Icore/runtime/include \
        -Iapi/rest/include

LDFLAGS := $(shell pkg-config --libs sqlite3)

SQLITE_SRC := core/sqlite/src/Database.cpp

.PHONY: all test clean prepare-test-db dashboard-cli daemon test-vdr-snapshot-builder-startup-snapshot test-search-timer-preview-epg-cache-refresh-service test-search-timer-preview-epg-cache-refresh-controller test-search-timer-preview-epg-cache-stale-guard test-api-router-searchtimer-preview-epg-cache-refresh-route test-api-router-searchtimer-preview-refresh-then-preview

all: test

test-ci-fast: test-vdr-snapshot-builder-startup-snapshot test-search-timer-preview-epg-cache-refresh-service test-search-timer-preview-epg-cache-refresh-controller test-search-timer-preview-epg-cache-stale-guard test-api-router-searchtimer-preview-epg-cache-refresh-route test-api-router-searchtimer-preview-refresh-then-preview

test-vdr: test-vdr-snapshot-builder-startup-snapshot test-search-timer-preview-epg-cache-refresh-service test-search-timer-preview-epg-cache-refresh-controller test-search-timer-preview-epg-cache-stale-guard test-api-router-searchtimer-preview-epg-cache-refresh-route test-api-router-searchtimer-preview-refresh-then-preview

test-vdr-snapshot-builder-startup-snapshot:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_snapshot_builder_startup_snapshot.cpp \
		-o /tmp/test_vdr_snapshot_builder_startup_snapshot
	/tmp/test_vdr_snapshot_builder_startup_snapshot


test-search-timer-preview-epg-cache-refresh-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_search_timer_preview_epg_cache_refresh_service.cpp \
		-o /tmp/test_search_timer_preview_epg_cache_refresh_service
	/tmp/test_search_timer_preview_epg_cache_refresh_service


test-search-timer-preview-epg-cache-refresh-controller:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		api/rest/src/SearchTimerPreviewEpgCacheRefreshController.cpp \
		api/rest/tests/test_search_timer_preview_epg_cache_refresh_controller.cpp \
		-o /tmp/test_search_timer_preview_epg_cache_refresh_controller
	/tmp/test_search_timer_preview_epg_cache_refresh_controller


test-search-timer-preview-epg-cache-stale-guard:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_search_timer_preview_epg_cache_stale_guard.cpp \
		-o /tmp/test_search_timer_preview_epg_cache_stale_guard
	/tmp/test_search_timer_preview_epg_cache_stale_guard


test-api-router-searchtimer-preview-epg-cache-refresh-route: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(REST_ROUTER_SRC) \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStaleProbeAdministrationService.cpp \
		api/rest/src/EpgSearchNativeFuzzyStaleProbeAdministrationController.cpp \
		core/vdr/src/EpgSearchNativeFuzzyOperatorRefreshService.cpp \
		api/rest/src/EpgSearchNativeFuzzyOperatorRefreshController.cpp \
		api/rest/src/SearchTimerController.cpp \
		api/rest/src/SearchTimerDiscoveryController.cpp \
		core/vdr/src/SearchTimerResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerService.cpp \
		api/rest/src/SearchTimerCreateRequestParser.cpp \
		api/rest/src/SearchTimerUpdateRequestParser.cpp \
		api/rest/src/SearchTimerDeleteRequestParser.cpp \
		api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp \
		api/rest/src/VdrController.cpp \
		api/rest/src/VdrRecordingQueryController.cpp \
		api/rest/tests/test_api_router_searchtimer_preview_epg_cache_refresh_route.cpp \
		$(LDFLAGS) \
		-o /tmp/test_api_router_searchtimer_preview_epg_cache_refresh_route
	/tmp/test_api_router_searchtimer_preview_epg_cache_refresh_route


test-api-router-searchtimer-preview-refresh-then-preview: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(REST_ROUTER_SRC) \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStaleProbeAdministrationService.cpp \
		api/rest/src/EpgSearchNativeFuzzyStaleProbeAdministrationController.cpp \
		core/vdr/src/EpgSearchNativeFuzzyOperatorRefreshService.cpp \
		api/rest/src/EpgSearchNativeFuzzyOperatorRefreshController.cpp \
		api/rest/src/SearchTimerController.cpp \
		api/rest/src/SearchTimerDiscoveryController.cpp \
		core/vdr/src/SearchTimerResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerService.cpp \
		api/rest/src/SearchTimerCreateRequestParser.cpp \
		api/rest/src/SearchTimerUpdateRequestParser.cpp \
		api/rest/src/SearchTimerDeleteRequestParser.cpp \
		api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp \
		api/rest/src/VdrController.cpp \
		api/rest/src/VdrRecordingQueryController.cpp \
		api/rest/tests/test_api_router_searchtimer_preview_refresh_then_preview.cpp \
		$(LDFLAGS) \
		-o /tmp/test_api_router_searchtimer_preview_refresh_then_preview
	/tmp/test_api_router_searchtimer_preview_refresh_then_preview

prepare-test-db:
	rm -f /tmp/vdr-suite-test.db
	sqlite3 /tmp/vdr-suite-test.db < database/schema/vdr-suite.sql
	sqlite3 /tmp/vdr-suite-test.db < database/testdata/sample-data.sql
