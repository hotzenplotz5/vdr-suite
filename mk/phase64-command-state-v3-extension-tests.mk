.PHONY: test-phase64-command-state-v3-extension-architecture test-phase64-command-state-v3-extension

test-phase64-command-state-v3-extension-architecture:
	python3 tools/check_phase64_command_state_v3_extension.py

test-phase64-command-state-v3-extension: \
		test-phase64-command-state-v3-extension-architecture \
		test-phase63-command-delivery-runtime \
		test-phase63-fenced-native-operation-runtime
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_JSON_SRC) \
		$(AGENT_COMMAND_CLIENT_SRC) \
		core/agent/tests/test_backend_agent_command_state_v3.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_command_state_v3
	$(BUILD_DIR)/test_backend_agent_command_state_v3

test-fast: test-phase64-command-state-v3-extension
test-architecture: test-phase64-command-state-v3-extension-architecture
