.PHONY: test-phase64-native-timer-create-agent-contract-architecture test-phase64-native-timer-create-agent-contract

test-phase64-native-timer-create-agent-contract-architecture:
	python3 tools/check_phase64_native_timer_create_agent_contract.py

test-phase64-native-timer-create-agent-contract: test-phase64-native-timer-create-agent-contract-architecture
	$(BUILD_CXX) $(CXXFLAGS) \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/src/BackendAgentNativeProbe.cpp \
		core/agent/src/BackendAgentNativeTimerCreate.cpp \
		core/agent/src/BackendAgentNativeTimerCreatePayload.cpp \
		core/agent/src/BackendAgentNativeTimerModify.cpp \
		core/agent/src/BackendAgentNativeTimerModifyPayload.cpp \
		core/agent/src/BackendAgentRecordingMarksModify.cpp \
		core/agent/src/BackendAgentRecordingMarksModifyPayload.cpp \
		core/agent/src/BackendAgentCommand.cpp \
		core/agent/tests/test_backend_agent_native_timer_create_contract.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_create_contract
	$(BUILD_DIR)/test_backend_agent_native_timer_create_contract

test-fast: test-phase64-native-timer-create-agent-contract
test-architecture: test-phase64-native-timer-create-agent-contract-architecture
