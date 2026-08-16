.PHONY: test-phase64-command-state-extension-architecture test-phase64-command-state-extension

test-phase64-command-state-extension-architecture:
	python3 tools/check_phase64_command_state_extension.py

test-phase64-command-state-extension: test-phase64-command-state-extension-architecture
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		core/agent/src/BackendAgentNativeTimerDelete.cpp \
		core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp \
		core/agent/src/BackendAgentNativeTimerCreateLocalState.cpp \
		core/agent/src/BackendAgentNativeTimerCreateRecovery.cpp \
		core/agent/src/BackendAgentCommandStateExtension.cpp \
		core/agent/tests/test_backend_agent_command_state_extension.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_command_state_extension
	$(BUILD_DIR)/test_backend_agent_command_state_extension
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		core/agent/src/BackendAgentNativeTimerDelete.cpp \
		core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp \
		core/agent/src/BackendAgentNativeTimerCreateLocalState.cpp \
		core/agent/src/BackendAgentNativeTimerCreateRecovery.cpp \
		core/agent/src/BackendAgentCommandStateExtension.cpp \
		core/agent/tests/test_backend_agent_native_timer_create_state_extension.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_create_state_extension
	$(BUILD_DIR)/test_backend_agent_native_timer_create_state_extension

test-fast: test-phase64-command-state-extension
test-architecture: test-phase64-command-state-extension-architecture
