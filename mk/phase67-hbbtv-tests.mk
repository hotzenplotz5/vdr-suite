.PHONY: test-phase67-hbbtv-suitebridge \
	test-phase67-hbbtv-agent-transport \
	test-phase67-hbbtv-resolver \
	test-phase67-hbbtv-security \
	test-phase67-hbbtv-public-surface \
	test-phase67-hbbtv-frontend \
	test-phase67-hbbtv-daemon-build \
	test-phase67-hbbtv-discovery-foundation

test-phase67-hbbtv-suitebridge:
	$(MAKE) -C vdr-plugin-suite-bridge test-hbbtv
	$(MAKE) -C vdr-plugin-suite-bridge check-hbbtv-adapter-wiring
	$(MAKE) -C vdr-plugin-suite-bridge check-hbbtv-svdrp-boundary
	$(MAKE) -C vdr-plugin-suite-bridge check-build

test-phase67-hbbtv-agent-transport:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_hbbtv_transport.cpp \
		-pthread $(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_hbbtv_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_hbbtv_transport

test-phase67-hbbtv-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeHbbtvResolver.cpp \
		core/vdr/tests/test_suite_bridge_hbbtv_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_hbbtv_resolver
	$(BUILD_DIR)/test_suite_bridge_hbbtv_resolver

test-phase67-hbbtv-security:
	$(MAKE) test-security-hbbtv-read

test-phase67-hbbtv-public-surface:
	python3 tools/check_phase67_hbbtv_discovery_surface.py

test-phase67-hbbtv-frontend:
	node --check web/frontend/api/client-api.js
	node --check web/frontend/live-tv-view.js
	node web/frontend/tests/test_phase67_hbbtv_discovery.js
	python3 tools/check_frontend_ownership_contracts.py
	$(MAKE) test-phase67-teletext-frontend

test-phase67-hbbtv-daemon-build:
	$(MAKE) daemon

test-phase67-hbbtv-discovery-foundation: \
	test-phase67-hbbtv-suitebridge \
	test-phase67-hbbtv-agent-transport \
	test-phase67-hbbtv-resolver \
	test-phase67-hbbtv-security \
	test-phase67-hbbtv-public-surface \
	test-phase67-hbbtv-frontend \
	test-phase67-hbbtv-daemon-build
