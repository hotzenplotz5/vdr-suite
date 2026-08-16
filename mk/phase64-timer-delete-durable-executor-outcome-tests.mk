.PHONY: test-phase64-timer-delete-durable-executor-outcome-architecture test-phase64-timer-delete-durable-executor-outcome

test-phase64-timer-delete-durable-executor-outcome-architecture:
	python3 tools/check_phase64_timer_delete_durable_executor_outcome.py

test-phase64-timer-delete-durable-executor-outcome: \
		test-phase64-timer-delete-durable-executor-outcome-architecture \
		test-phase64-timer-delete-fenced-executor
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_JSON_SRC) \
		$(AGENT_COMMAND_CLIENT_SRC) \
		core/agent/tests/test_backend_agent_timer_delete_durable_executor_outcome.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_timer_delete_durable_executor_outcome
	$(BUILD_DIR)/test_backend_agent_timer_delete_durable_executor_outcome

test-fast: test-phase64-timer-delete-durable-executor-outcome
test-architecture: test-phase64-timer-delete-durable-executor-outcome-architecture
