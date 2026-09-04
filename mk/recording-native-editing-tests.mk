VDR_RECORDING_NATIVE_MARKS_RESOLVER_SRC := \
	core/vdr/src/SuiteBridgeRecordingMarksResolver.cpp

.PHONY: test-suite-bridge-svdrp-recording-marks-transport test-suite-bridge-recording-marks-resolver test-recording-native-editing-read-contracts

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
		$(VDR_RECORDING_NATIVE_MARKS_RESOLVER_SRC) \
		core/vdr/tests/test_suite_bridge_recording_marks_resolver.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_recording_marks_resolver
	$(BUILD_DIR)/test_suite_bridge_recording_marks_resolver

test-recording-native-editing-read-contracts: \
	test-suite-bridge-svdrp-recording-marks-transport \
	test-suite-bridge-recording-marks-resolver
