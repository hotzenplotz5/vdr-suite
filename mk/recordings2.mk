.PHONY: install-recordings2-runtime test-recordings2-runtime test-recordings2-install-staging

install-runtime: install-recordings2-runtime

install-recordings2-runtime:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -m 0644 web/frontend/recordings2-shared.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-shared.js
	$(INSTALL) -m 0644 web/frontend/recordings2-folder-artwork.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-folder-artwork.js
	$(INSTALL) -m 0644 web/frontend/recordings2-actions.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-actions.js
	$(INSTALL) -m 0644 web/frontend/recordings2-browser-view.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-browser-view.js
	$(INSTALL) -m 0644 web/frontend/recordings2-person-search-view.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-person-search-view.js
	$(INSTALL) -m 0644 web/frontend/recordings2-metadata-view.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-metadata-view.js
	$(INSTALL) -m 0644 web/frontend/recordings2-metadata-assignment.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-metadata-assignment.js
	$(INSTALL) -m 0644 web/frontend/recordings2-metadata-detail.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-metadata-detail.js
	$(INSTALL) -m 0644 web/frontend/recordings2.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2.js

test-frontend-i18n: test-recordings2-runtime

test-recordings2-runtime:
	node --check web/frontend/recordings2-shared.js
	node --check web/frontend/recordings2-folder-artwork.js
	node --check web/frontend/recordings2-actions.js
	node --check web/frontend/recordings2-browser-view.js
	node --check web/frontend/recordings2-person-search-view.js
	node --check web/frontend/recordings2-metadata-view.js
	node --check web/frontend/recordings2-metadata-assignment.js
	node --check web/frontend/recordings2-metadata-detail.js
	node --check web/frontend/recordings2.js
	node web/frontend/tests/test_recordings2_runtime.js
	node web/frontend/tests/test_recordings2_actions_genre.js
	node web/frontend/tests/test_recordings2_metadata_detail.js
	node web/frontend/tests/test_recordings2_metadata_assignment.js
	python3 tools/check_recordings2_runtime_wiring.py

test-recordings2-install-staging: test-install-staging
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-shared.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-folder-artwork.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-actions.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-browser-view.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-person-search-view.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-metadata-view.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-metadata-assignment.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-metadata-detail.js
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-shared.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-folder-artwork.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-actions.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-browser-view.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-person-search-view.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-metadata-view.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-metadata-assignment.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-metadata-detail.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2.js
