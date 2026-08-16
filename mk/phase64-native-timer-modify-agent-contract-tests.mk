.PHONY: test-phase64-native-timer-modify-agent-contract test-phase64-native-timer-modify-agent-contract-architecture

test-phase64-native-timer-modify-agent-contract-architecture:
	python3 tools/check_phase64_native_timer_modify_agent_contract.py

test-phase64-native-timer-modify-agent-contract: test-phase64-native-timer-modify-agent-contract-architecture
	mkdir -p $(BUILD_DIR)
	$(BUILD_CXX) $(CXXFLAGS) \
		core/agent/src/BackendAgentCommand.cpp \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/src/BackendAgentNativeTimerCreate.cpp \
		core/agent/src/BackendAgentNativeTimerDelete.cpp \
		core/agent/src/BackendAgentNativeTimerModify.cpp \
		core/agent/src/BackendAgentNativeTimerModifyPayload.cpp \
		core/agent/src/BackendAgentNativeTimerModifyLocalState.cpp \
		core/agent/src/BackendAgentNativeTimerModifyExecutor.cpp \
		core/agent/tests/test_backend_agent_native_timer_modify.cpp \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_modify
	$(BUILD_DIR)/test_backend_agent_native_timer_modify
	rm -f $(BUILD_DIR)/test_backend_agent_native_timer_modify

test-fast: test-phase64-native-timer-modify-agent-contract
test-architecture: test-phase64-native-timer-modify-agent-contract-architecture
