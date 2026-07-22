DAEMON_SRC += \
	core/vdr/src/VdrRecordingNativeIdentity.cpp

.PHONY: test-vdr-recording-native-identity test-suite-bridge-svdrp-recording-metadata-transport test-recording-native-metadata-contracts

test-vdr-recording-native-identity:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/vdr/include \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		core/vdr/tests/test_vdr_recording_native_identity.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_native_identity
	$(BUILD_DIR)/test_vdr_recording_native_identity

test-suite-bridge-svdrp-recording-metadata-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		-Icore/agent/include \
		-Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_SRC) \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		core/agent/tests/test_suite_bridge_svdrp_recording_metadata_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_recording_metadata_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_recording_metadata_transport

test-recording-native-metadata-contracts: \
	test-vdr-recording-native-identity \
	test-suite-bridge-svdrp-recording-metadata-transport
