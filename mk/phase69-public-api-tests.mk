.PHONY: test-phase69-public-api-contract-root

test-phase69-public-api-contract-root:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiContractRuntime.cpp \
		api/rest/tests/test_public_api_contract_runtime.cpp \
		-o $(BUILD_DIR)/test_public_api_contract_runtime
	$(BUILD_DIR)/test_public_api_contract_runtime
