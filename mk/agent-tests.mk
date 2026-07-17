.PHONY: test-suite-bridge-agent-boundary test-suite-bridge-handshake

test-suite-bridge-agent-boundary:
	python3 tools/check_suite_bridge_agent_boundary.py

test-suite-bridge-handshake:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_SRC) \
		core/agent/tests/test_suite_bridge_handshake.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_handshake
	$(BUILD_DIR)/test_suite_bridge_handshake

test-ci-fast: test-suite-bridge-agent-boundary test-suite-bridge-handshake

test-vdr: test-suite-bridge-agent-boundary test-suite-bridge-handshake
