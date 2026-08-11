.PHONY: test-phase64-native-timer-delete-agent-contract-architecture test-phase64-native-timer-delete-agent-contract

test-phase64-native-timer-delete-agent-contract-architecture:
	python3 tools/check_phase64_native_timer_delete_agent_contract.py

test-phase64-native-timer-delete-agent-contract: test-phase64-native-timer-delete-agent-contract-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/src/BackendAgentNativeTimerDelete.cpp \
		core/agent/tests/test_backend_agent_native_timer_delete.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_delete
	$(BUILD_DIR)/test_backend_agent_native_timer_delete

test-fast: test-phase64-native-timer-delete-agent-contract
test-architecture: test-phase64-native-timer-delete-agent-contract-architecture
