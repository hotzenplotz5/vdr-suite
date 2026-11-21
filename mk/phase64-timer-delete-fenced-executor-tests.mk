.PHONY: test-phase64-timer-delete-fenced-executor-architecture test-phase64-timer-delete-fenced-executor

test-phase64-timer-delete-fenced-executor-architecture:
	python3 tools/check_phase64_timer_delete_fenced_executor.py

test-phase64-timer-delete-fenced-executor: \
		test-phase64-timer-delete-fenced-executor-architecture \
		test-phase64-timer-delete-fresh-durable-starting
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_STATE_SRC) \
		$(AGENT_TIMER_DELETE_EXECUTOR_SRC) \
		core/agent/tests/test_backend_agent_timer_delete_fenced_executor.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_timer_delete_fenced_executor
	$(BUILD_DIR)/test_backend_agent_timer_delete_fenced_executor

test-fast: test-phase64-timer-delete-fenced-executor
test-architecture: test-phase64-timer-delete-fenced-executor-architecture
