.PHONY: test-phase64-suitebridge-native-timer-create-transport-architecture test-phase64-suitebridge-native-timer-create-transport

test-phase64-suitebridge-native-timer-create-transport-architecture:
	python3 tools/check_phase64_suitebridge_native_timer_create_transport.py

test-phase64-suitebridge-native-timer-create-transport: \
		test-phase64-suitebridge-native-timer-create-transport-architecture \
		test-phase64-native-timer-create-executor
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		core/agent/src/BackendAgentCommand.cpp \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/src/BackendAgentNativeProbe.cpp \
		core/agent/src/BackendAgentNativeTimerCreate.cpp \
		core/agent/src/BackendAgentNativeTimerCreatePayload.cpp \
		core/agent/src/BackendAgentNativeTimerModify.cpp \
		core/agent/src/BackendAgentNativeTimerModifyPayload.cpp \
		core/agent/src/BackendAgentRecordingMarksModify.cpp \
		core/agent/src/BackendAgentRecordingMarksModifyPayload.cpp \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		$(AGENT_NATIVE_TIMER_CREATE_TRANSPORT_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_native_timer_create_transport.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_native_timer_create_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_native_timer_create_transport

test-fast: test-phase64-suitebridge-native-timer-create-transport
test-architecture: test-phase64-suitebridge-native-timer-create-transport-architecture
