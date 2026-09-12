# Optional browser diagnostics. No API or cache configuration changes.
.PHONY: test-browser-performance-diagnostics install-browser-performance-diagnostics test-browser-performance-diagnostics-install-staging

test-frontend-contracts: test-browser-performance-diagnostics
test-ci-frontend: test-browser-performance-diagnostics
test-ci-packaging: test-browser-performance-diagnostics-install-staging
install-runtime: install-browser-performance-diagnostics

test-browser-performance-diagnostics:
	node --check tools/build_browser_performance_diagnostics.js
	node --check web/frontend/browser-performance-diagnostics.js
	node --check web/frontend/browser-performance-bridge.js
	node --check web/frontend/browser-artwork-probe.js
	node --check tools/test_browser_home_epg_diagnostics.js
	node --check tools/test_browser_home_epg_timeout.js
	node tools/test_browser_artwork_probe.js
	node tools/test_browser_resource_details.js
	node tools/test_browser_performance_diagnostics.js
	node tools/test_browser_home_epg_diagnostics.js
	node tools/test_browser_home_epg_timeout.js
	node tools/test_browser_diagnostics_http_routes.js
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/tests/test_browser_diagnostics_asset_routes.cpp \
		-o $(BUILD_DIR)/test_browser_diagnostics_asset_routes
	$(BUILD_DIR)/test_browser_diagnostics_asset_routes

install-browser-performance-diagnostics:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -m 0644 web/frontend/browser-artwork-probe.js $(DESTDIR)$(DATADIR)/web/frontend/browser-artwork-probe.js
	$(INSTALL) -m 0644 web/frontend/browser-performance-bridge.js $(DESTDIR)$(DATADIR)/web/frontend/browser-performance-bridge.js
	$(INSTALL) -m 0644 web/frontend/browser-performance-diagnostics.js $(DESTDIR)$(DATADIR)/web/frontend/browser-performance-diagnostics.js
	$(INSTALL) -m 0644 web/frontend/browser-performance-diagnostics.html $(DESTDIR)$(DATADIR)/web/frontend/browser-performance-diagnostics.html
	node tools/build_browser_performance_diagnostics.js $(DESTDIR)$(DATADIR)/web/frontend/browser-performance-home.html

# Use a separate staging target instead of a recursive Make invocation hidden
# inside a shell recipe. GNU Make executes recipes containing $(MAKE) even
# under -n; the staging shell must never run during the test-graph dry run.
test-browser-performance-diagnostics-install-staging:
	@stage=$$(mktemp -d); \
	trap 'rm -rf "$$stage"' EXIT; \
	make --no-print-directory install-browser-performance-diagnostics DESTDIR="$$stage" PREFIX=/usr || exit $$?; \
	root="$$stage/usr/share/vdr-suite/web/frontend"; \
	for name in browser-artwork-probe.js browser-performance-bridge.js browser-performance-diagnostics.js browser-performance-diagnostics.html browser-performance-home.html; do \
	  test -f "$$root/$$name" || exit 1; \
	done; \
	grep -F 'browser-performance-bridge.js' "$$root/browser-performance-home.html" >/dev/null || exit 1; \
	grep -F 'browser-performance-home.html' "$$root/browser-performance-diagnostics.js" >/dev/null || exit 1; \
	! grep -F 'browser-performance-bridge.js' web/frontend/index.html >/dev/null
