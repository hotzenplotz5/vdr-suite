.PHONY: test-suite-bridge-agent-boundary test-suite-bridge-handshake test-suite-bridge-handshake-missing-plugin test-suite-bridge-svdrp-transport-boundary test-suite-bridge-svdrp-transport test-suite-bridge-svdrp-transport-live

test-suite-bridge-agent-boundary:
	python3 tools/check_suite_bridge_agent_boundary.py

test-suite-bridge-handshake:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		core/agent/tests/test_suite_bridge_handshake.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_handshake
	$(BUILD_DIR)/test_suite_bridge_handshake

test-suite-bridge-handshake-missing-plugin:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		core/agent/tests/test_suite_bridge_handshake_missing_plugin.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_handshake_missing_plugin
	$(BUILD_DIR)/test_suite_bridge_handshake_missing_plugin

test-suite-bridge-svdrp-transport-boundary:
	python3 tools/check_suite_bridge_svdrp_transport_boundary.py

test-suite-bridge-svdrp-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SVDRP_TRANSPORT_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_transport

test-suite-bridge-svdrp-transport-live:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_transport_live.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_transport_live
	$(BUILD_DIR)/test_suite_bridge_svdrp_transport_live
