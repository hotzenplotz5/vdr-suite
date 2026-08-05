CXX := g++
BUILD_DIR ?= $(CURDIR)/.build
BUILD_CXX = mkdir -p "$(BUILD_DIR)" && $(CXX)
CXXFLAGS := -std=c++17 -Wall -Wextra \
        -Icore/sqlite/include \
        -Icore/recordings/include \
        -Icore/daemon/include \
        -Icore/agent/include \
        -Icore/vdr/include \
        -Icore/http/include \
        -Icore/runtime/include \
        -Iapi/rest/include

LDFLAGS := $(shell pkg-config --libs sqlite3)

SQLITE_SRC := core/sqlite/src/Database.cpp

.PHONY: all test clean prepare-test-db check-vdr-linkage-contracts test-build-artifact-paths test-runtime-config test-epg-event-repository test-epg-cache-service test-epg-cache-controller test-api-router-epg-cache-routes dashboard-cli daemon test-vdr-snapshot-builder-startup-snapshot test-search-timer-preview-epg-cache-refresh-service test-search-timer-preview-epg-cache-refresh-controller test-search-timer-preview-epg-cache-stale-guard test-search-timer-preview-epg-cache-change-invalidator test-snapshot-change-feed-preview-epg-cache-invalidation test-api-router-searchtimer-preview-epg-cache-refresh-route test-api-router-searchtimer-preview-refresh-then-preview

all: test

check-vdr-linkage-contracts:
	python3 tools/check_vdr_linkage_contracts.py

test-build-artifact-paths:
	python3 tools/check_build_artifact_paths.py


test-runtime-config:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/daemon/src/RuntimeConfig.cpp \
		core/daemon/tests/test_runtime_config.cpp \
		-o $(BUILD_DIR)/test_runtime_config
	$(BUILD_DIR)/test_runtime_config


test-epg-cache-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgEventRepository.cpp \
		core/vdr/src/EpgArtworkRepository.cpp \
		core/vdr/src/EpgArtworkEnrichmentService.cpp \
		core/vdr/src/EpgCacheService.cpp \
		core/vdr/src/VdrService.cpp \
		core/vdr/tests/test_epg_cache_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_cache_service
	$(BUILD_DIR)/test_epg_cache_service


test-epg-cache-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgEventRepository.cpp \
		core/vdr/src/EpgArtworkRepository.cpp \
		core/vdr/src/EpgArtworkPublicJsonSerializer.cpp \
		core/vdr/src/EpgArtworkEnrichmentService.cpp \
		core/vdr/src/EpgCacheService.cpp \
		core/vdr/src/VdrService.cpp \
		$(EPG_SCRAPER_METADATA_CONTROLLER_SRC) \
		api/rest/src/EpgArtworkController.cpp \
		api/rest/src/EpgCacheController.cpp \
		api/rest/tests/test_epg_cache_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_cache_controller
	$(BUILD_DIR)/test_epg_cache_controller


test-api-router-epg-cache-routes: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
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
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		api/rest/src/VdrRecordingFolderController.cpp \
		api/rest/tests/test_api_router_epg_cache_routes.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_api_router_epg_cache_routes
	$(BUILD_DIR)/test_api_router_epg_cache_routes


test-vdr-snapshot-builder-startup-snapshot:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_snapshot_builder_startup_snapshot.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_snapshot_builder_startup_snapshot
	$(BUILD_DIR)/test_vdr_snapshot_builder_startup_snapshot


test-search-timer-preview-epg-cache-refresh-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_search_timer_preview_epg_cache_refresh_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_search_timer_preview_epg_cache_refresh_service
	$(BUILD_DIR)/test_search_timer_preview_epg_cache_refresh_service


test-search-timer-preview-epg-cache-refresh-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		api/rest/src/SearchTimerPreviewEpgCacheRefreshController.cpp \
		api/rest/tests/test_search_timer_preview_epg_cache_refresh_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_search_timer_preview_epg_cache_refresh_controller
	$(BUILD_DIR)/test_search_timer_preview_epg_cache_refresh_controller


test-search-timer-preview-epg-cache-stale-guard:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_search_timer_preview_epg_cache_stale_guard.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_search_timer_preview_epg_cache_stale_guard
	$(BUILD_DIR)/test_search_timer_preview_epg_cache_stale_guard


test-search-timer-preview-epg-cache-change-invalidator:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_search_timer_preview_epg_cache_change_invalidator.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_search_timer_preview_epg_cache_change_invalidator
	$(BUILD_DIR)/test_search_timer_preview_epg_cache_change_invalidator


test-snapshot-change-feed-preview-epg-cache-invalidation:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_snapshot_change_feed_preview_epg_cache_invalidation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_change_feed_preview_epg_cache_invalidation
	$(BUILD_DIR)/test_snapshot_change_feed_preview_epg_cache_invalidation


test-api-router-searchtimer-preview-epg-cache-refresh-route: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
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
		api/rest/src/VdrRecordingFolderController.cpp \
		api/rest/tests/test_api_router_searchtimer_preview_epg_cache_refresh_route.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_api_router_searchtimer_preview_epg_cache_refresh_route
	$(BUILD_DIR)/test_api_router_searchtimer_preview_epg_cache_refresh_route


test-api-router-searchtimer-preview-refresh-then-preview: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
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
		api/rest/src/VdrRecordingFolderController.cpp \
		api/rest/tests/test_api_router_searchtimer_preview_refresh_then_preview.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_api_router_searchtimer_preview_refresh_then_preview
	$(BUILD_DIR)/test_api_router_searchtimer_preview_refresh_then_preview

prepare-test-db:
	python3 tools/prepare_test_database.py \
		/tmp/vdr-suite-test.db \
		database/schema/vdr-suite.sql \
		database/testdata/sample-data.sql
