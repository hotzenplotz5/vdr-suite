VDR_RECORDING_NATIVE_MARKS_SRC := \
	core/vdr/src/SuiteBridgeRecordingMarksResolver.cpp

RECORDING_NATIVE_EDITING_REST_SRC := \
	api/rest/src/RecordingMarksApiRuntime.cpp

RECORDING_NATIVE_EDITING_ROUTER_SRC := \
	core/vdr/src/VdrRecordingNativeIdentity.cpp \
	$(RECORDING_NATIVE_EDITING_REST_SRC)

DAEMON_SRC += $(VDR_RECORDING_NATIVE_MARKS_SRC)
DAEMON_SRC += $(RECORDING_NATIVE_EDITING_REST_SRC)
REST_ROUTER_SRC += $(RECORDING_NATIVE_EDITING_ROUTER_SRC)

.PHONY: test-suite-bridge-svdrp-recording-marks-transport test-suite-bridge-recording-marks-resolver test-recording-marks-api-runtime test-suitebridge-recording-marks-modify-protocol check-recording-native-editing-runtime-wiring test-recording-native-editing-read-contracts test-recording-native-editing-contracts

test-suite-bridge-svdrp-recording-marks-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		-Icore/agent/include \
		-Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_recording_marks_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_recording_marks_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_recording_marks_transport

test-suite-bridge-recording-marks-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/vdr/include \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		$(VDR_RECORDING_NATIVE_MARKS_SRC) \
		core/vdr/tests/test_suite_bridge_recording_marks_resolver.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_recording_marks_resolver
	$(BUILD_DIR)/test_suite_bridge_recording_marks_resolver

test-recording-marks-api-runtime:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Iapi/rest/include \
		-Icore/vdr/include \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		$(RECORDING_NATIVE_EDITING_REST_SRC) \
		api/rest/tests/test_recording_marks_api_runtime.cpp \
		-o $(BUILD_DIR)/test_recording_marks_api_runtime
	$(BUILD_DIR)/test_recording_marks_api_runtime

test-suitebridge-recording-marks-modify-protocol:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_recording_marks_modify.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_recording_marks_modify.cpp \
		-o $(BUILD_DIR)/test_suitebridge_recording_marks_modify
	$(BUILD_DIR)/test_suitebridge_recording_marks_modify

check-recording-native-editing-runtime-wiring:
	python3 tools/check_recording_native_editing_runtime_wiring.py

test-recording-native-editing-read-contracts: \
	test-suite-bridge-svdrp-recording-marks-transport \
	test-suite-bridge-recording-marks-resolver \
	test-recording-marks-api-runtime \
	check-recording-native-editing-runtime-wiring

test-recording-native-editing-contracts: \
	test-recording-native-editing-read-contracts \
	test-suitebridge-recording-marks-modify-protocol

test-fast: test-recording-native-editing-contracts
