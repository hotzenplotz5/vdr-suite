.PHONY: test-phase64-suitebridge-native-timer-delete-disabled-transport-architecture test-phase64-suitebridge-native-timer-delete-disabled-transport

test-phase64-suitebridge-native-timer-delete-disabled-transport-architecture:
	python3 tools/check_phase64_suitebridge_native_timer_delete_disabled_transport.py

test-phase64-suitebridge-native-timer-delete-disabled-transport: \
		test-phase64-suitebridge-native-timer-delete-disabled-transport-architecture \
		test-phase64-timer-delete-durable-executor-outcome
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/src/BackendAgentNativeTimerDelete.cpp \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_native_timer_delete_transport.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_native_timer_delete_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_native_timer_delete_transport
	$(BUILD_CXX) $(CXXFLAGS) -Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_native_timer_delete.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_delete.cpp \
		-o $(BUILD_DIR)/test_suitebridge_native_timer_delete
	$(BUILD_DIR)/test_suitebridge_native_timer_delete


test-fast: test-phase64-suitebridge-native-timer-delete-disabled-transport
test-architecture: test-phase64-suitebridge-native-timer-delete-disabled-transport-architecture
