.PHONY: test-phase69-public-api-contract-root

test-phase69-public-api-contract-root:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_api_contract_runtime.cpp \
		-o $(BUILD_DIR)/test_public_api_contract_runtime
	$(BUILD_DIR)/test_public_api_contract_runtime

.PHONY: test-phase69-public-resource-preconditions

test-phase69-public-resource-preconditions:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/tests/test_public_resource_preconditions.cpp \
		-o $(BUILD_DIR)/test_public_resource_preconditions
	$(BUILD_DIR)/test_public_resource_preconditions
