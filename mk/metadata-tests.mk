.PHONY: test-metadata-identity test-metadata-foundation

test-metadata-identity:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(METADATA_SRC) \
		core/metadata/tests/test_metadata_identity.cpp \
		-o $(BUILD_DIR)/test_metadata_identity
	$(BUILD_DIR)/test_metadata_identity

test-metadata-foundation: test-metadata-identity

test-fast: test-metadata-foundation
