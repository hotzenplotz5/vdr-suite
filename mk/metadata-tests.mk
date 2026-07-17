.PHONY: \
	test-metadata-make-boundary \
	test-metadata-identity \
	test-metadata-schema-contract \
	test-metadata-foundation

test-metadata-make-boundary:
	python3 tools/check_metadata_make_boundary.py

test-metadata-identity: CXXFLAGS += -Icore/metadata/include
test-metadata-identity:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(METADATA_PLATFORM_SRC) \
		core/metadata/tests/test_metadata_identity.cpp \
		-o $(BUILD_DIR)/test_metadata_identity
	$(BUILD_DIR)/test_metadata_identity

test-metadata-schema-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/metadata/tests/test_metadata_schema_contract.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_metadata_schema_contract
	$(BUILD_DIR)/test_metadata_schema_contract

test-metadata-foundation: \
	test-metadata-make-boundary \
	test-metadata-service \
	test-metadata-identity \
	test-metadata-schema-contract

test-fast: test-metadata-foundation
