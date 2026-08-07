.PHONY: test-phase63-local-provider-ownership-contract-architecture test-phase63-local-provider-ownership-contract test-phase63-local-provider-selection-runtime-architecture test-phase63-local-provider-selection-runtime

test-phase63-local-provider-ownership-contract-architecture:
	python3 tools/check_phase63_local_provider_ownership_contract.py

test-phase63-local-provider-ownership-contract: test-phase63-local-provider-ownership-contract-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/tests/test_backend_agent_local_provider.cpp \
		-o $(BUILD_DIR)/test_backend_agent_local_provider
	$(BUILD_DIR)/test_backend_agent_local_provider

test-phase63-local-provider-selection-runtime-architecture:
	python3 tools/check_phase63_local_provider_selection_runtime.py

test-phase63-local-provider-selection-runtime: test-phase63-local-provider-selection-runtime-architecture
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		$(AGENT_CONTROL_PLANE_DOMAIN_SRC) \
		core/agent/tests/test_backend_agent_local_provider_selection_runtime.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_local_provider_selection_runtime
	$(BUILD_DIR)/test_backend_agent_local_provider_selection_runtime

test-fast: test-phase63-local-provider-ownership-contract test-phase63-local-provider-selection-runtime
test-architecture: test-phase63-local-provider-ownership-contract-architecture test-phase63-local-provider-selection-runtime-architecture
