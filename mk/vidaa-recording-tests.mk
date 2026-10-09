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
