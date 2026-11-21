.PHONY: test-phase64-native-timer-delete-local-starting-architecture test-phase64-native-timer-delete-local-starting

test-phase64-native-timer-delete-local-starting-architecture:
	python3 tools/check_phase64_native_timer_delete_local_starting.py

test-phase64-native-timer-delete-local-starting: test-phase64-native-timer-delete-local-starting-architecture
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		core/agent/src/BackendAgentNativeTimerDelete.cpp \
		core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp \
		core/agent/tests/test_backend_agent_native_timer_delete_local_state.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_delete_local_state
	$(BUILD_DIR)/test_backend_agent_native_timer_delete_local_state

test-fast: test-phase64-native-timer-delete-local-starting
test-architecture: test-phase64-native-timer-delete-local-starting-architecture
