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

.PHONY: check-recording-cut-runtime-wiring test-recording-cut-api-runtime test-recording-cut-security test-backend-agent-recording-cut test-backend-agent-recording-cut-local-state test-backend-agent-recording-cut-executor test-backend-agent-recording-cut-reconciliation test-suite-bridge-svdrp-recording-cut-transport test-suite-bridge-svdrp-recording-cut-state-transport test-suite-bridge-recording-cut-state-resolver test-suitebridge-recording-cut-protocol test-daemon-recording-cut-reconciliation

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

test-recording-cut-security:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_recording_cut_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recording_cut_security
	$(BUILD_DIR)/test_recording_cut_security

test-backend-agent-recording-cut:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/agent/include \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		core/agent/tests/test_backend_agent_recording_cut.cpp \
		-o $(BUILD_DIR)/test_backend_agent_recording_cut
	$(BUILD_DIR)/test_backend_agent_recording_cut

test-backend-agent-recording-cut-local-state:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/agent/include \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		core/agent/src/BackendAgentRecordingCutLocalState.cpp \
		core/agent/tests/test_backend_agent_recording_cut_local_state.cpp \
		-o $(BUILD_DIR)/test_backend_agent_recording_cut_local_state
	$(BUILD_DIR)/test_backend_agent_recording_cut_local_state

test-backend-agent-recording-cut-executor:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/agent/include \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		core/agent/src/BackendAgentRecordingCutLocalState.cpp \
		core/agent/src/BackendAgentRecordingCutExecutor.cpp \
		core/agent/tests/test_backend_agent_recording_cut_executor.cpp \
		-o $(BUILD_DIR)/test_backend_agent_recording_cut_executor
	$(BUILD_DIR)/test_backend_agent_recording_cut_executor

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

test-suite-bridge-svdrp-recording-cut-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		-Icore/agent/include \
		-Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		$(AGENT_RECORDING_CUT_TRANSPORT_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_recording_cut_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_recording_cut_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_recording_cut_transport

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

test-suitebridge-recording-cut-protocol:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_recording_cut.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_recording_cut.cpp \
		-o $(BUILD_DIR)/test_suitebridge_recording_cut
	$(BUILD_DIR)/test_suitebridge_recording_cut

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
# test-fast recipe/graph untouched while making the complete Slice-3 boundary
# mandatory.
test-fast: check-recording-cut-runtime-wiring \
	test-recording-cut-api-runtime \
	test-recording-cut-security \
	test-backend-agent-recording-cut \
	test-backend-agent-recording-cut-local-state \
	test-backend-agent-recording-cut-executor \
	test-backend-agent-recording-cut-reconciliation \
	test-suite-bridge-svdrp-recording-cut-transport \
	test-suite-bridge-svdrp-recording-cut-state-transport \
	test-suite-bridge-recording-cut-state-resolver \
	test-suitebridge-recording-cut-protocol \
	test-daemon-recording-cut-reconciliation

RECORDING_CUT_ACCEPTANCE_PREFLIGHT_RUNNER := \
	tools/run_recording_cut_acceptance_preflight.sh
RECORDING_CUT_ACCEPTANCE_PREFLIGHT_GUARD := \
	tools/check_recording_cut_acceptance_preflight.py

RECORDING_CUT_EXPECTED_BRANCH ?= work/post-phase66-native-recording-editing
RECORDING_CUT_EXPECTED_HEAD ?=
RECORDING_CUT_CONTROL_PLANE_URL ?=
RECORDING_CUT_CURL_CONFIG ?=
RECORDING_CUT_CA_CERTIFICATE_PATH ?=
RECORDING_CUT_EVIDENCE_DIR ?=
RECORDING_CUT_BACKEND_ID ?= default
RECORDING_CUT_RECORDING_ID ?=
RECORDING_CUT_DAEMON_SERVICE ?= vdr-suite-daemon.service
RECORDING_CUT_AGENT_SERVICE ?= vdr-suite-backend-agent.service
RECORDING_CUT_VDR_SERVICE ?= vdr.service
RECORDING_CUT_SVDRP_PORT ?= 6419

.PHONY: test-recording-cut-acceptance-preflight-harness recording-cut-acceptance-preflight

test-recording-cut-acceptance-preflight-harness:
	bash -n "$(RECORDING_CUT_ACCEPTANCE_PREFLIGHT_RUNNER)"
	python3 "$(RECORDING_CUT_ACCEPTANCE_PREFLIGHT_GUARD)"

recording-cut-acceptance-preflight: test-recording-cut-acceptance-preflight-harness
	@test -n "$(RECORDING_CUT_EXPECTED_HEAD)" || { echo "RECORDING_CUT_EXPECTED_HEAD is required"; exit 2; }
	@test -n "$(RECORDING_CUT_CONTROL_PLANE_URL)" || { echo "RECORDING_CUT_CONTROL_PLANE_URL is required"; exit 2; }
	@test -n "$(RECORDING_CUT_EVIDENCE_DIR)" || { echo "RECORDING_CUT_EVIDENCE_DIR is required"; exit 2; }
	@test -n "$(RECORDING_CUT_RECORDING_ID)" || { echo "RECORDING_CUT_RECORDING_ID is required"; exit 2; }
	RECORDING_CUT_EXPECTED_BRANCH="$(RECORDING_CUT_EXPECTED_BRANCH)" \
	RECORDING_CUT_EXPECTED_HEAD="$(RECORDING_CUT_EXPECTED_HEAD)" \
	RECORDING_CUT_CONTROL_PLANE_URL="$(RECORDING_CUT_CONTROL_PLANE_URL)" \
	RECORDING_CUT_CURL_CONFIG="$(RECORDING_CUT_CURL_CONFIG)" \
	RECORDING_CUT_CA_CERTIFICATE_PATH="$(RECORDING_CUT_CA_CERTIFICATE_PATH)" \
	RECORDING_CUT_EVIDENCE_DIR="$(RECORDING_CUT_EVIDENCE_DIR)" \
	RECORDING_CUT_BACKEND_ID="$(RECORDING_CUT_BACKEND_ID)" \
	RECORDING_CUT_RECORDING_ID="$(RECORDING_CUT_RECORDING_ID)" \
	RECORDING_CUT_DAEMON_SERVICE="$(RECORDING_CUT_DAEMON_SERVICE)" \
	RECORDING_CUT_AGENT_SERVICE="$(RECORDING_CUT_AGENT_SERVICE)" \
	RECORDING_CUT_VDR_SERVICE="$(RECORDING_CUT_VDR_SERVICE)" \
	RECORDING_CUT_SVDRP_PORT="$(RECORDING_CUT_SVDRP_PORT)" \
	bash "$(RECORDING_CUT_ACCEPTANCE_PREFLIGHT_RUNNER)"

test-fast: test-recording-cut-acceptance-preflight-harness
