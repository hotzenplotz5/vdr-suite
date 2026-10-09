.PHONY: test-public-recording-identity-repository

test-public-recording-identity-repository:
	$(BUILD_CXX) $(CXXFLAGS) \\
		$(SQLITE_SRC) \\
		core/recordings/src/PublicRecordingIdentityRepository.cpp \\
		core/recordings/tests/test_public_recording_identity_repository.cpp \\
		$(LDFLAGS) \\
		-o $(BUILD_DIR)/test_public_recording_identity_repository
	$(BUILD_DIR)/test_public_recording_identity_repository
