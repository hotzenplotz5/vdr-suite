.PHONY: test-phase67-hbbtv-suitebridge \
	test-phase67-hbbtv-agent-transport \
	test-phase67-hbbtv-resolver \
	test-phase67-hbbtv-runtime-resolver \
	test-phase67-hbbtv-presentation-resolver \
	test-phase67-hbbtv-media-resolver \
	test-phase67-hbbtv-session-service \
	test-phase67-hbbtv-session-foundation \
	test-phase67-hbbtv-session-api \
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

test-phase67-hbbtv-runtime-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeHbbtvRuntimeResolver.cpp \
		core/vdr/tests/test_suite_bridge_hbbtv_runtime_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_hbbtv_runtime_resolver
	$(BUILD_DIR)/test_suite_bridge_hbbtv_runtime_resolver

test-phase67-hbbtv-presentation-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeHbbtvPresentationResolver.cpp \
		core/vdr/tests/test_suite_bridge_hbbtv_presentation_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_hbbtv_presentation_resolver
	$(BUILD_DIR)/test_suite_bridge_hbbtv_presentation_resolver

test-phase67-hbbtv-media-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeHbbtvMediaResolver.cpp \
		core/vdr/tests/test_suite_bridge_hbbtv_media_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_hbbtv_media_resolver
	$(BUILD_DIR)/test_suite_bridge_hbbtv_media_resolver

test-phase67-hbbtv-session-service:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		core/daemon/src/HbbtvApplicationSessionService.cpp \
		core/daemon/tests/test_hbbtv_application_session_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_hbbtv_application_session_service
	$(BUILD_DIR)/test_hbbtv_application_session_service

test-phase67-hbbtv-session-foundation: \
	test-phase67-hbbtv-suitebridge \
	test-phase67-hbbtv-agent-transport \
	test-phase67-hbbtv-runtime-resolver \
	test-phase67-hbbtv-presentation-resolver \
	test-phase67-hbbtv-media-resolver \
	test-phase67-hbbtv-session-service

test-phase67-hbbtv-session-api:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		api/rest/src/HbbtvApiRuntime.cpp \
		core/daemon/src/HbbtvApplicationSessionService.cpp \
		api/rest/tests/test_hbbtv_session_api_runtime.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_hbbtv_session_api_runtime
	$(BUILD_DIR)/test_hbbtv_session_api_runtime

test-phase67-hbbtv-security:
	$(MAKE) test-security-hbbtv-read
	$(MAKE) test-security-hbbtv-session

test-phase67-hbbtv-public-surface:
	python3 tools/check_phase67_hbbtv_discovery_surface.py
	python3 tools/check_phase67_hbbtv_session_http_surface.py

test-phase67-hbbtv-frontend:
	node --check web/frontend/api/client-api.js
	node --check web/frontend/hbbtv-qoi.js
	node --check web/frontend/live-tv-view.js
	node web/frontend/tests/test_phase67_hbbtv_discovery.js
	node web/frontend/tests/test_phase67_hbbtv_qoi.js
	node web/frontend/tests/test_phase67_hbbtv_browser_transport.js
	node web/frontend/tests/test_phase67_hbbtv_overlay.js
	python3 tools/check_phase67_hbbtv_overlay_frontend.py
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

.PHONY: test-phase67-hbbtv-session-http-foundation

test-phase67-hbbtv-session-http-foundation: \
	test-phase67-hbbtv-session-foundation \
	test-phase67-hbbtv-session-api \
	test-phase67-hbbtv-security \
	test-phase67-hbbtv-public-surface \
	test-phase67-hbbtv-daemon-build
