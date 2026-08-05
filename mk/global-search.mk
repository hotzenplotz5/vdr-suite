GLOBAL_SEARCH_SRC := \
	core/vdr/src/GlobalSearchRepository.cpp \
	core/vdr/src/GlobalSearchPersonPortraitRepository.cpp \
	core/vdr/src/GlobalSearchService.cpp \
	api/rest/src/GlobalSearchController.cpp \
	api/rest/src/GlobalSearchApiRuntime.cpp

GLOBAL_SEARCH_TEST_SUPPORT_SRC := \
	$(SQLITE_SRC) \
	core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
	core/vdr/src/VdrConfig.cpp \
	core/vdr/src/GlobalSearchRepository.cpp \
	core/vdr/src/GlobalSearchPersonPortraitRepository.cpp \
	core/vdr/src/GlobalSearchService.cpp \
	core/vdr/src/BackendRegistry.cpp \
	core/vdr/src/BackendRegistryService.cpp \
	api/rest/src/GlobalSearchController.cpp \
	api/rest/src/GlobalSearchApiRuntime.cpp \
	api/rest/src/RestQueryParameters.cpp

.PHONY: test-global-search-repository test-global-search-controller test-global-search-frontend test-global-search-architecture test-global-search

test-global-search-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/src/GlobalSearchRepository.cpp \
		core/vdr/tests/test_global_search_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_global_search_repository
	$(BUILD_DIR)/test_global_search_repository

test-global-search-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(GLOBAL_SEARCH_TEST_SUPPORT_SRC) \
		api/rest/tests/test_global_search_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_global_search_controller
	$(BUILD_DIR)/test_global_search_controller

test-global-search-frontend:
	node --check web/frontend/modules/global-search.js
	node --check web/frontend/api/client-api.js
	node web/frontend/tests/test_global_search_runtime.js

test-global-search-architecture:
	python3 tools/check_global_search_architecture_contracts.py

test-global-search: test-global-search-repository test-global-search-controller test-global-search-frontend test-global-search-architecture

test-fast: test-global-search