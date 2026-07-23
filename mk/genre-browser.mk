CXXFLAGS += -Icore/metadata/include

GENRE_BROWSER_REST_SRC := \
	$(METADATA_GENRE_SRC) \
	api/rest/src/GenreBrowserController.cpp \
	api/rest/src/GenreBrowserApiRuntime.cpp

.PHONY: test-genre-browser-controller test-genre-browser

test-genre-browser-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(GENRE_BROWSER_REST_SRC) \
		api/rest/src/RestQueryParameters.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		api/rest/tests/test_genre_browser_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_browser_controller
	$(BUILD_DIR)/test_genre_browser_controller

test-genre-browser: test-metadata-genres test-genre-browser-controller

test-fast: test-genre-browser
