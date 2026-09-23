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

.PHONY: test-phase69-operation-read-facade

test-phase69-operation-read-facade:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/operations/src/MutationOperationReadService.cpp \
		core/operations/tests/test_mutation_operation_read_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_mutation_operation_read_service
	$(BUILD_DIR)/test_mutation_operation_read_service

.PHONY: test-phase69-public-operation-resource

test-phase69-public-operation-resource:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_operation_resource.cpp \
		-o $(BUILD_DIR)/test_public_operation_resource
	$(BUILD_DIR)/test_public_operation_resource
