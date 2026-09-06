.PHONY: check-recording-cut-runtime-wiring test-backend-agent-recording-cut-reconciliation

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

# Additive prerequisites only: keep the existing recording-native-editing
# test-fast recipe/graph untouched while making the Slice-3 boundary mandatory.
test-fast: check-recording-cut-runtime-wiring \
	test-backend-agent-recording-cut-reconciliation