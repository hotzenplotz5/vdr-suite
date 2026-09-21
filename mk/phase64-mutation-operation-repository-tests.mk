.PHONY: test-phase64-mutation-operation-repository-architecture test-phase64-mutation-operation-repository

test-phase64-mutation-operation-repository-architecture:
	python3 tools/check_phase64_mutation_operation_repository.py

test-phase64-mutation-operation-repository: test-phase64-mutation-operation-repository-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/operations/tests/test_mutation_operation_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_mutation_operation_repository
	$(BUILD_DIR)/test_mutation_operation_repository

test-fast: test-phase64-mutation-operation-repository
test-architecture: test-phase64-mutation-operation-repository-architecture
