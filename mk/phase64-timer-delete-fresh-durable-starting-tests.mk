.PHONY: test-phase64-timer-delete-fresh-durable-starting-architecture test-phase64-timer-delete-fresh-durable-starting

test-phase64-timer-delete-fresh-durable-starting-architecture:
	python3 tools/check_phase64_timer_delete_fresh_durable_starting.py

test-phase64-timer-delete-fresh-durable-starting: \
		test-phase64-timer-delete-fresh-durable-starting-architecture \
		test-phase64-timer-delete-local-state-lifecycle
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_COMMAND_JSON_SRC) \
		$(AGENT_COMMAND_CLIENT_SRC) \
		core/agent/tests/test_backend_agent_timer_delete_fresh_durable_starting.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_timer_delete_fresh_durable_starting
	$(BUILD_DIR)/test_backend_agent_timer_delete_fresh_durable_starting

test-fast: test-phase64-timer-delete-fresh-durable-starting
test-architecture: test-phase64-timer-delete-fresh-durable-starting-architecture
