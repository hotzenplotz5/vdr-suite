.PHONY: test-phase66-recording-discovery-frontend install-phase66-recording-discovery-assets test-phase66-recording-discovery-install-staging

test-phase66-recording-discovery-frontend:
	node --check web/frontend/home-recording-discovery-bootstrap.js
	node --check web/frontend/home-recording-discovery.js
	node web/frontend/tests/test_phase66_recording_discovery.js
	node web/frontend/tests/test_phase66_home_initial_load.js
	node web/frontend/tests/test_phase66_series_native_group_priority.js
	node web/frontend/tests/test_phase66_recording_discovery_progressive.js
	node web/frontend/tests/test_phase66_recording_discovery_contract.js
	node web/frontend/tests/test_phase66_home_mouse_drag_rails.js
	node web/frontend/tests/test_phase66_home_inline_discovery.js
	node web/frontend/tests/test_phase66_home_root_random_genre.js
	node web/frontend/tests/test_phase66_home_random_genre_placement.js
	node web/frontend/tests/test_phase66_home_random_folder_autoopen.js

# Slice 66.5 is part of the ordinary frontend and packaging regression surfaces.
test-frontend-contracts: test-phase66-recording-discovery-frontend
test-ci-frontend: test-phase66-recording-discovery-frontend
test-ci-packaging: test-phase66-recording-discovery-install-staging

# Additive install hook keeps the established install-runtime recipe authoritative.
install-runtime: install-phase66-recording-discovery-assets

install-phase66-recording-discovery-assets:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -m 0644 web/frontend/home-recording-discovery-bootstrap.js $(DESTDIR)$(DATADIR)/web/frontend/home-recording-discovery-bootstrap.js
	$(INSTALL) -m 0644 web/frontend/home-recording-discovery.js $(DESTDIR)$(DATADIR)/web/frontend/home-recording-discovery.js

test-phase66-recording-discovery-install-staging:
	rm -rf /tmp/vdr-suite-phase66-recording-discovery-pkgroot
	$(MAKE) install-phase66-recording-discovery-assets DESTDIR=/tmp/vdr-suite-phase66-recording-discovery-pkgroot PREFIX=/usr
	test -f /tmp/vdr-suite-phase66-recording-discovery-pkgroot/usr/share/vdr-suite/web/frontend/home-recording-discovery-bootstrap.js
	test -f /tmp/vdr-suite-phase66-recording-discovery-pkgroot/usr/share/vdr-suite/web/frontend/home-recording-discovery.js
	grep -F '/frontend/home-recording-discovery.js' /tmp/vdr-suite-phase66-recording-discovery-pkgroot/usr/share/vdr-suite/web/frontend/home-recording-discovery-bootstrap.js >/dev/null
	grep -F '/api/vdr/recordings/query' web/frontend/api/client-api.js >/dev/null
	grep -F '/api/vdr/recordings/folder' web/frontend/api/client-api.js >/dev/null
	grep -F '/api/metadata/genres/recordings' web/frontend/api/genre-client-api.js >/dev/null
