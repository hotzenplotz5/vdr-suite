.PHONY: test-phase63-local-provider-ownership-contract-architecture test-phase63-local-provider-ownership-contract

test-phase63-local-provider-ownership-contract-architecture:
	python3 tools/check_phase63_local_provider_ownership_contract.py

test-phase63-local-provider-ownership-contract: test-phase63-local-provider-ownership-contract-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/tests/test_backend_agent_local_provider.cpp \
		-o $(BUILD_DIR)/test_backend_agent_local_provider
	$(BUILD_DIR)/test_backend_agent_local_provider

test-fast: test-phase63-local-provider-ownership-contract
test-architecture: test-phase63-local-provider-ownership-contract-architecture
