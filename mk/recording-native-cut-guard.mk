VDR_RECORDING_NATIVE_CUT_STATE_SRC := \
	core/vdr/src/SuiteBridgeRecordingCutStateResolver.cpp

RECORDING_NATIVE_CUT_REST_SRC := \
	api/rest/src/RecordingCutApiRuntime.cpp

DAEMON_RECORDING_CUT_SRC := \
	core/daemon/src/DaemonRecordingCutReconciliation.cpp \
	core/daemon/src/DaemonRuntimeRecordingCut.cpp \
	core/daemon/src/DaemonRuntimeRecordingEditing.cpp

DAEMON_SRC += $(VDR_RECORDING_NATIVE_CUT_STATE_SRC)
DAEMON_SRC += $(RECORDING_NATIVE_CUT_REST_SRC)
DAEMON_SRC += $(DAEMON_RECORDING_CUT_SRC)
REST_ROUTER_SRC += $(RECORDING_NATIVE_CUT_REST_SRC)

.PHONY: check-recording-cut-runtime-wiring test-recording-cut-api-runtime test-backend-agent-recording-cut-reconciliation test-suite-bridge-svdrp-recording-cut-state-transport test-suite-bridge-recording-cut-state-resolver test-daemon-recording-cut-reconciliation

check-recording-cut-runtime-wiring:
	python3 tools/check_recording_cut_runtime_wiring.py

test-recording-cut-api-runtime:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Iapi/rest/include \
		-Icore/vdr/include \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		$(RECORDING_NATIVE_CUT_REST_SRC) \
		api/rest/tests/test_recording_cut_api_runtime.cpp \
		-o $(BUILD_DIR)/test_recording_cut_api_runtime
	$(BUILD_DIR)/test_recording_cut_api_runtime

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

test-daemon-recording-cut-reconciliation:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/daemon/include \
		-Icore/vdr/include \
		core/vdr/src/VdrRecordingNativeIdentity.cpp \
		core/daemon/src/DaemonRecordingCutReconciliation.cpp \
		core/daemon/tests/test_daemon_recording_cut_reconciliation.cpp \
		-o $(BUILD_DIR)/test_daemon_recording_cut_reconciliation
	$(BUILD_DIR)/test_daemon_recording_cut_reconciliation

# Additive prerequisites only: keep the existing recording-native-editing
# test-fast recipe/graph untouched while making the Slice-3 boundary mandatory.
test-fast: check-recording-cut-runtime-wiring \
	test-recording-cut-api-runtime \
	test-backend-agent-recording-cut-reconciliation \
	test-suite-bridge-svdrp-recording-cut-state-transport \
	test-suite-bridge-recording-cut-state-resolver \
	test-daemon-recording-cut-reconciliation
