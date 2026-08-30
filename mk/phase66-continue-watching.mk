CXXFLAGS += -Icore/media/include -Icore/security/include -Icore/http/include

DAEMON_SRC += \
	api/rest/src/ContinueWatchingApiRuntime.cpp \
	core/media/src/ContinueWatching.cpp \
	core/media/src/ContinueWatchingRepository.cpp

.PHONY: test-continue-watching test-continue-watching-security-request test-phase66-continue-watching-frontend install-phase66-continue-watching-assets test-phase66-continue-watching-install-staging

test-continue-watching:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/media/src/ContinueWatching.cpp \
		core/media/src/ContinueWatchingRepository.cpp \
		core/media/tests/test_continue_watching.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_continue_watching
	$(BUILD_DIR)/test_continue_watching

test-continue-watching-security-request:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/security/tests/test_continue_watching_security_request.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_continue_watching_security_request
	$(BUILD_DIR)/test_continue_watching_security_request

test-phase66-continue-watching-frontend:
	node web/frontend/tests/test_phase66_continue_watching.js
	node web/frontend/tests/test_phase66_continue_watching_artwork.js
	node web/frontend/tests/test_phase66_continue_watching_home_runtime.js
	node web/frontend/tests/test_phase66_continue_watching_fast_owner_contract.js
	node web/frontend/tests/test_phase66_continue_watching_navigation_contract.js

# Slice 66.4 is part of the ordinary regression and hosted-CI surfaces.
test: test-continue-watching test-continue-watching-security-request test-phase66-continue-watching-frontend
test-ci-fast: test-continue-watching test-continue-watching-security-request test-test-http-server
test-vdr: test-continue-watching test-continue-watching-security-request
test-frontend-contracts: test-phase66-continue-watching-frontend
test-ci-frontend: test-phase66-continue-watching-frontend
test-ci-packaging: test-phase66-continue-watching-install-staging

# Additive install hook keeps the established install-runtime recipe authoritative.
install-runtime: install-phase66-continue-watching-assets

install-phase66-continue-watching-assets:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/api
	$(INSTALL) -m 0644 web/frontend/home-continue-watching.js $(DESTDIR)$(DATADIR)/web/frontend/home-continue-watching.js
	$(INSTALL) -m 0644 web/frontend/api/continue-watching-sync.js $(DESTDIR)$(DATADIR)/web/frontend/api/continue-watching-sync.js

test-phase66-continue-watching-install-staging:
	rm -rf /tmp/vdr-suite-phase66-cw-pkgroot
	$(MAKE) install-phase66-continue-watching-assets DESTDIR=/tmp/vdr-suite-phase66-cw-pkgroot PREFIX=/usr
	test -f /tmp/vdr-suite-phase66-cw-pkgroot/usr/share/vdr-suite/web/frontend/home-continue-watching.js
	test -f /tmp/vdr-suite-phase66-cw-pkgroot/usr/share/vdr-suite/web/frontend/api/continue-watching-sync.js
	grep -F '/api/media/continue-watching' /tmp/vdr-suite-phase66-cw-pkgroot/usr/share/vdr-suite/web/frontend/home-continue-watching.js >/dev/null
	grep -F '/api/media/continue-watching' /tmp/vdr-suite-phase66-cw-pkgroot/usr/share/vdr-suite/web/frontend/api/continue-watching-sync.js >/dev/null
