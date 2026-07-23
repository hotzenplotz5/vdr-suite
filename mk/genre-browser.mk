CXXFLAGS += -Icore/metadata/include

GENRE_BROWSER_REST_SRC := \
	$(METADATA_GENRE_SRC) \
	api/rest/src/GenreBrowserController.cpp \
	api/rest/src/GenreBrowserApiRuntime.cpp

GENRE_BROWSER_TEST_SUPPORT_SRC := \
	$(SQLITE_SRC) \
	$(GENRE_BROWSER_REST_SRC) \
	api/rest/src/RestQueryParameters.cpp \
	core/vdr/src/BackendRegistry.cpp \
	core/vdr/src/BackendRegistryService.cpp

.PHONY: install-genre-frontend test-genre-browser-controller test-genre-browser-pagination test-genre-browser-architecture test-genre-browser-frontend test-genre-browser

install-runtime: install-genre-frontend

install-genre-frontend:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/api
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/modules
	$(INSTALL) -m 0644 web/frontend/api/genre-client-api.js $(DESTDIR)$(DATADIR)/web/frontend/api/genre-client-api.js
	$(INSTALL) -m 0644 web/frontend/epg-detail-owner.js $(DESTDIR)$(DATADIR)/web/frontend/epg-detail-owner.js
	$(INSTALL) -m 0644 web/frontend/modules/genres.js $(DESTDIR)$(DATADIR)/web/frontend/modules/genres.js

test-genre-browser-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(GENRE_BROWSER_TEST_SUPPORT_SRC) \
		api/rest/tests/test_genre_browser_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_browser_controller
	$(BUILD_DIR)/test_genre_browser_controller

test-genre-browser-pagination:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(GENRE_BROWSER_TEST_SUPPORT_SRC) \
		api/rest/tests/test_genre_browser_pagination.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_browser_pagination
	$(BUILD_DIR)/test_genre_browser_pagination

test-genre-browser-architecture:
	python3 tools/check_genre_browser_architecture_contracts.py

test-genre-browser-frontend:
	node --check web/frontend/api/genre-client-api.js
	node --check web/frontend/epg-detail-owner.js
	node --check web/frontend/modules/genres.js
	node --check web/frontend/recordings2-browser-view.js
	node --check web/frontend/recordings2.js
	node --check web/frontend/platform/deferred-runtime-loader.js
	python3 tools/check_genre_browser_frontend_contracts.py

test-genre-browser: test-metadata-genres test-metadata-genre-conflicts test-genre-browser-controller test-genre-browser-pagination test-genre-browser-architecture test-genre-browser-frontend

test-fast: test-genre-browser
