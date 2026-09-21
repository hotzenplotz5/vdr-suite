.PHONY: test-phase64-native-timer-create-executor-architecture test-phase64-native-timer-create-executor

test-phase64-native-timer-create-executor-architecture:
	python3 tools/check_phase64_native_timer_create_executor.py

test-phase64-native-timer-create-executor: \
		test-phase64-native-timer-create-executor-architecture \
		test-phase64-native-timer-create-local-state
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_STATE_SRC) \
		$(AGENT_TIMER_CREATE_EXECUTOR_SRC) \
		$(AGENT_NATIVE_TIMER_CREATE_COMMAND_HANDLER_SRC) \
		core/agent/tests/test_backend_agent_native_timer_create_executor.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_create_executor
	$(BUILD_DIR)/test_backend_agent_native_timer_create_executor

test-fast: test-phase64-native-timer-create-executor
test-architecture: test-phase64-native-timer-create-executor-architecture
