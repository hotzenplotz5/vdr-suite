VDR_RECORDING_NATIVE_CUT_STATE_SRC := \
	core/vdr/src/SuiteBridgeRecordingCutStateResolver.cpp

DAEMON_SRC += $(VDR_RECORDING_NATIVE_CUT_STATE_SRC)

.PHONY: check-recording-cut-runtime-wiring test-backend-agent-recording-cut-reconciliation test-suite-bridge-svdrp-recording-cut-state-transport test-suite-bridge-recording-cut-state-resolver

check-recording-cut-runtime-wiring:
	python3 tools/check_recording_cut_runtime_wiring.py

test-backend-agent-recording-cut-reconciliation:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/agent/include \
		-Icore/security/include \
		-Icore/vdr/include \
		-Icore/scheduler/include \
		-Icore/config/include \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		$(AGENT_CONTROL_PLANE_DOMAIN_SRC) \
		core/agent/tests/test_backend_agent_recording_cut_reconciliation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_recording_cut_reconciliation
	$(BUILD_DIR)/test_backend_agent_recording_cut_reconciliation

test-suite-bridge-svdrp-recording-cut-state-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		-Icore/agent/include \
		-Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_recording_cut_state_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_recording_cut_state_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_recording_cut_state_transport

test-suite-bridge-recording-cut-state-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/vdr/include \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		$(VDR_RECORDING_NATIVE_CUT_STATE_SRC) \
		core/vdr/tests/test_suite_bridge_recording_cut_state_resolver.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_recording_cut_state_resolver
	$(BUILD_DIR)/test_suite_bridge_recording_cut_state_resolver

# Additive prerequisites only: keep the existing recording-native-editing
# test-fast recipe/graph untouched while making the Slice-3 boundary mandatory.
test-fast: check-recording-cut-runtime-wiring \
	test-backend-agent-recording-cut-reconciliation \
	test-suite-bridge-svdrp-recording-cut-state-transport \
	test-suite-bridge-recording-cut-state-resolver
