.PHONY: test-phase64-native-timer-create-local-state-architecture test-phase64-native-timer-create-local-state

test-phase64-native-timer-create-local-state-architecture:
	python3 tools/check_phase64_native_timer_create_local_state.py

test-phase64-native-timer-create-local-state: test-phase64-native-timer-create-local-state-architecture
	$(BUILD_CXX) $(CXXFLAGS) \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/src/BackendAgentNativeProbe.cpp \
		core/agent/src/BackendAgentNativeTimerCreate.cpp \
		core/agent/src/BackendAgentNativeTimerCreatePayload.cpp \
		core/agent/src/BackendAgentNativeTimerCreateLocalState.cpp \
		core/agent/src/BackendAgentNativeTimerCreateRecovery.cpp \
		core/agent/src/BackendAgentCommand.cpp \
		core/agent/tests/test_backend_agent_native_timer_create_local_state.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_create_local_state
	$(BUILD_DIR)/test_backend_agent_native_timer_create_local_state

test-fast: test-phase64-native-timer-create-local-state
test-architecture: test-phase64-native-timer-create-local-state-architecture
