# Optional browser diagnostics. No daemon, API or cache configuration changes.
.PHONY: test-browser-performance-diagnostics install-browser-performance-diagnostics

test-browser-performance-diagnostics:
	node --check tools/build_browser_performance_diagnostics.js
	node --check web/frontend/browser-performance-diagnostics.js
	node --check web/frontend/browser-performance-bridge.js
	node --check web/frontend/browser-artwork-probe.js
	node tools/test_browser_artwork_probe.js
	node tools/test_browser_performance_diagnostics.js

install-browser-performance-diagnostics:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -m 0644 web/frontend/browser-artwork-probe.js $(DESTDIR)$(DATADIR)/web/frontend/browser-artwork-probe.js
	$(INSTALL) -m 0644 web/frontend/browser-performance-bridge.js $(DESTDIR)$(DATADIR)/web/frontend/browser-performance-bridge.js
	$(INSTALL) -m 0644 web/frontend/browser-performance-diagnostics.js $(DESTDIR)$(DATADIR)/web/frontend/browser-performance-diagnostics.js
	$(INSTALL) -m 0644 web/frontend/browser-performance-diagnostics.html $(DESTDIR)$(DATADIR)/web/frontend/browser-performance-diagnostics.html
	node tools/build_browser_performance_diagnostics.js $(DESTDIR)$(DATADIR)/web/browser-performance-diagnostics-app.html
