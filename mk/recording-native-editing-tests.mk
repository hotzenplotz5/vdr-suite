VDR_RECORDING_NATIVE_MARKS_SRC := \
	core/vdr/src/SuiteBridgeRecordingMarksResolver.cpp

RECORDING_NATIVE_EDITING_REST_SRC := \
	api/rest/src/RecordingMarksApiRuntime.cpp

DAEMON_SRC += $(VDR_RECORDING_NATIVE_MARKS_SRC)
DAEMON_SRC += $(RECORDING_NATIVE_EDITING_REST_SRC)
REST_ROUTER_SRC += $(RECORDING_NATIVE_EDITING_REST_SRC)

.PHONY: test-suite-bridge-svdrp-recording-marks-transport test-suite-bridge-recording-marks-resolver test-recording-marks-api-runtime check-recording-native-editing-runtime-wiring test-recording-native-editing-read-contracts

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

check-recording-native-editing-runtime-wiring:
	python3 tools/check_recording_native_editing_runtime_wiring.py

test-recording-native-editing-read-contracts: \
	test-suite-bridge-svdrp-recording-marks-transport \
	test-suite-bridge-recording-marks-resolver \
	test-recording-marks-api-runtime \
	check-recording-native-editing-runtime-wiring
