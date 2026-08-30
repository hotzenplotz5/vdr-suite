CXXFLAGS += -Icore/media/include

DAEMON_SRC += \
	core/media/src/RecentlyWatched.cpp \
	core/media/src/RecentlyWatchedRepository.cpp

# test-test-http-server compiles ContinueWatchingApiRuntime.cpp directly rather
# than through DAEMON_SRC. Extend only that target's existing source list so the
# Slice-66.6 symbols referenced by the shared runtime are linked there as well.
test-test-http-server: REST_ROUTER_SRC += \
	core/media/src/RecentlyWatched.cpp \
	core/media/src/RecentlyWatchedRepository.cpp

.PHONY: test-recently-watched test-phase66-recently-watched-frontend install-phase66-recently-watched-assets test-phase66-recently-watched-install-staging

test-recently-watched:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/media/src/RecentlyWatched.cpp \
		core/media/src/RecentlyWatchedRepository.cpp \
		core/media/tests/test_recently_watched.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recently_watched
	$(BUILD_DIR)/test_recently_watched

test-phase66-recently-watched-frontend:
	node --check web/frontend/home-recently-watched.js
	node --check web/frontend/api/continue-watching-sync.js
	node web/frontend/tests/test_phase66_recently_watched.js

# Slice 66.6 participates in the ordinary backend/frontend/packaging CI surfaces.
test: test-recently-watched test-phase66-recently-watched-frontend
test-ci-fast: test-recently-watched
test-vdr: test-recently-watched
test-frontend-contracts: test-phase66-recently-watched-frontend
test-ci-frontend: test-phase66-recently-watched-frontend
test-ci-packaging: test-phase66-recently-watched-install-staging

# Additive install hook keeps the established install-runtime recipe authoritative.
install-runtime: install-phase66-recently-watched-assets

install-phase66-recently-watched-assets:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -m 0644 web/frontend/home-recently-watched.js $(DESTDIR)$(DATADIR)/web/frontend/home-recently-watched.js

test-phase66-recently-watched-install-staging:
	rm -rf /tmp/vdr-suite-phase66-recently-watched-pkgroot
	$(MAKE) install-phase66-recently-watched-assets DESTDIR=/tmp/vdr-suite-phase66-recently-watched-pkgroot PREFIX=/usr
	test -f /tmp/vdr-suite-phase66-recently-watched-pkgroot/usr/share/vdr-suite/web/frontend/home-recently-watched.js
	grep -F '/api/media/recently-watched' /tmp/vdr-suite-phase66-recently-watched-pkgroot/usr/share/vdr-suite/web/frontend/home-recently-watched.js >/dev/null
	grep -F '/api/media/recently-watched' web/frontend/api/continue-watching-sync.js >/dev/null
