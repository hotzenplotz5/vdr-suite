.PHONY: install-recordings2-runtime test-recordings2-runtime test-recordings2-install-staging

install-runtime: install-recordings2-runtime

install-recordings2-runtime:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -m 0644 web/frontend/recordings2.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2.js

test-frontend-i18n: test-recordings2-runtime

test-recordings2-runtime:
	node --check web/frontend/recordings2.js
	node web/frontend/tests/test_recordings2_runtime.js
	python3 tools/check_recordings2_runtime_wiring.py

test-install-staging: test-recordings2-install-staging

test-recordings2-install-staging:
	test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2.js
	node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2.js
