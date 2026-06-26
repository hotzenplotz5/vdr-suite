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

.PHONY: all test clean prepare-test-db dashboard-cli daemon test-vdr-snapshot-builder-startup-snapshot test-search-timer-preview-epg-cache-refresh-service test-search-timer-preview-epg-cache-refresh-controller

all: test

test-ci-fast: test-vdr-snapshot-builder-startup-snapshot test-search-timer-preview-epg-cache-refresh-service test-search-timer-preview-epg-cache-refresh-controller

test-vdr: test-vdr-snapshot-builder-startup-snapshot test-search-timer-preview-epg-cache-refresh-service test-search-timer-preview-epg-cache-refresh-controller

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

prepare-test-db:
	rm -f /tmp/vdr-suite-test.db
	sqlite3 /tmp/vdr-suite-test.db < database/schema/vdr-suite.sql
	sqlite3 /tmp/vdr-suite-test.db < database/testdata/sample-data.sql
