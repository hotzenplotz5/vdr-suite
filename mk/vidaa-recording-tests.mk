# VIDAA Recording R2: independent persistent identity acceptance.
.PHONY: test-vdr-public-recording-identity
test-vdr-public-recording-identity:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrPublicRecordingIdentityRepository.cpp \
		core/vdr/tests/test_vdr_public_recording_identity_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_public_recording_identity_repository
	$(BUILD_DIR)/test_vdr_public_recording_identity_repository

.PHONY: test-vdr-public-recording-collection
test-vdr-public-recording-collection:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrPublicRecordingCollection.cpp \
		core/vdr/tests/test_vdr_public_recording_collection.cpp \
		-o $(BUILD_DIR)/test_vdr_public_recording_collection
	$(BUILD_DIR)/test_vdr_public_recording_collection

.PHONY: test-vdr-public-recording-runtime
test-vdr-public-recording-runtime:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_recording_collection.cpp \
		-o $(BUILD_DIR)/test_public_recording_collection
	$(BUILD_DIR)/test_public_recording_collection
