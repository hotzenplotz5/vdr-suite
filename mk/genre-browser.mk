CXXFLAGS += -Icore/metadata/include

GENRE_BROWSER_REST_SRC := \
	$(METADATA_GENRE_SRC) \
	api/rest/src/GenreBrowserController.cpp \
	api/rest/src/GenreBrowserApiRuntime.cpp \
	api/rest/src/GenreBrowserApiRuntimeEpgTypeSnapshot.cpp

GENRE_BROWSER_TEST_SUPPORT_SRC := \
	$(SQLITE_SRC) \
	$(GENRE_BROWSER_REST_SRC) \
	api/rest/src/RestQueryParameters.cpp \
	core/vdr/src/VdrConfig.cpp \
	core/vdr/src/BackendRegistry.cpp \
	core/vdr/src/BackendRegistryService.cpp

.PHONY: test-vdr-channel-cache-repository test-genre-epg-enrichment-priority test-genre-epg-refresh-fast-path test-genre-write-batching test-genre-recording-sync-noop test-genre-browser-controller test-genre-browser-epg-type-snapshot test-genre-browser-pagination test-genre-browser-architecture test-genre-browser-frontend test-phase61-live-genre-tool test-genre-browser

test-vdr-channel-cache-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrChannelCacheRepository.cpp \
		core/vdr/tests/test_vdr_channel_cache_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_channel_cache_repository
	$(BUILD_DIR)/test_vdr_channel_cache_repository

test-genre-epg-enrichment-priority: CXXFLAGS += -Icore/vdr/include
test-genre-epg-enrichment-priority:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_GENRE_SRC) \
		core/metadata/tests/test_genre_epg_enrichment_priority.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_epg_enrichment_priority
	$(BUILD_DIR)/test_genre_epg_enrichment_priority

test-genre-epg-refresh-fast-path: CXXFLAGS += -Icore/vdr/include
test-genre-epg-refresh-fast-path:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_GENRE_SRC) \
		core/metadata/tests/test_genre_epg_refresh_fast_path.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_epg_refresh_fast_path
	$(BUILD_DIR)/test_genre_epg_refresh_fast_path

test-genre-write-batching: CXXFLAGS += -Icore/vdr/include
test-genre-write-batching:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_GENRE_SRC) \
		core/metadata/tests/test_genre_write_batching.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_write_batching
	$(BUILD_DIR)/test_genre_write_batching

test-genre-recording-sync-noop: CXXFLAGS += -Icore/vdr/include
test-genre-recording-sync-noop:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_GENRE_SRC) \
		core/metadata/tests/test_genre_recording_sync_noop.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_recording_sync_noop
	$(BUILD_DIR)/test_genre_recording_sync_noop

test-genre-browser-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(GENRE_BROWSER_TEST_SUPPORT_SRC) \
		api/rest/tests/test_genre_browser_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_browser_controller
	$(BUILD_DIR)/test_genre_browser_controller

test-genre-browser-epg-type-snapshot:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(GENRE_BROWSER_TEST_SUPPORT_SRC) \
		api/rest/tests/test_genre_browser_epg_type_snapshot.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_browser_epg_type_snapshot
	$(BUILD_DIR)/test_genre_browser_epg_type_snapshot

test-genre-browser-pagination:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(GENRE_BROWSER_TEST_SUPPORT_SRC) \
		api/rest/tests/test_genre_browser_pagination.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_browser_pagination
	$(BUILD_DIR)/test_genre_browser_pagination

test-genre-browser-architecture:
	python3 tools/check_genre_browser_architecture_contracts.py
	python3 tools/check_epg_type_snapshot_architecture_contracts.py
	python3 tools/check_epg_genre_continuation_latency.py
	python3 tools/check_phase61_live_tvscraper_comparison_contracts.py

test-genre-browser-frontend:
	node --check web/frontend/api/genre-client-api.js
	node --check web/frontend/epg-detail-owner.js
	node --check web/frontend/modules/genres.js
	node --check web/frontend/recordings2-browser-view.js
	node --check web/frontend/recordings2.js
	node --check web/frontend/platform/deferred-runtime-loader.js
	node web/frontend/tests/test_genres_runtime.js
	python3 tools/check_genre_browser_frontend_contracts.py

test-phase61-live-genre-tool:
	python3 -m py_compile tools/check_phase61_live_genres.py
	python3 tools/check_phase61_live_genres.py --self-test
	python3 -m py_compile tools/compare_phase61_live_tvscraper.py
	python3 tools/compare_phase61_live_tvscraper.py --self-test
	python3 -m py_compile tools/run_phase61_live_tvscraper_comparison.py
	python3 tools/run_phase61_live_tvscraper_comparison.py --self-test
	python3 -m py_compile tools/analyze_phase61_live_tvscraper_report.py
	python3 tools/analyze_phase61_live_tvscraper_report.py --self-test

test-genre-browser: test-vdr-channel-cache-repository test-genre-epg-enrichment-priority test-genre-epg-refresh-fast-path test-genre-write-batching test-genre-recording-sync-noop test-http-listener-image-write-isolation test-metadata-genres test-metadata-genre-conflicts test-genre-browser-controller test-genre-browser-epg-type-snapshot test-genre-browser-pagination test-genre-browser-architecture test-genre-browser-frontend test-phase61-live-genre-tool

test-fast: test-genre-browser
