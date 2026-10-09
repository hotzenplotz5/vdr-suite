.PHONY: test-public-recording-identity-repository

test-public-recording-identity-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/PublicRecordingIdentityRepository.cpp \
		core/recordings/tests/test_public_recording_identity_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_recording_identity_repository
	$(BUILD_DIR)/test_public_recording_identity_repository

.PHONY: test-public-recording-collection-projection

test-public-recording-collection-projection:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/PublicRecordingIdentityRepository.cpp \
		core/recordings/src/PublicRecordingCollectionProjection.cpp \
		core/recordings/tests/test_public_recording_collection_projection.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_recording_collection_projection
	$(BUILD_DIR)/test_public_recording_collection_projection

.PHONY: test-public-recording-collection-api

test-public-recording-collection-api:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_recording_collection.cpp \
		-o $(BUILD_DIR)/test_public_recording_collection
	$(BUILD_DIR)/test_public_recording_collection

.PHONY: test-public-v1-recording-reference-client

test-public-v1-recording-reference-client:
	node clients/reference-js/tests/test_public_v1_recording_client.js

.PHONY: test-public-recording-collection-security

test-public-recording-collection-security:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_public_recording_collection_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_recording_collection_security
	$(BUILD_DIR)/test_public_recording_collection_security

.PHONY: test-public-recording-genre-api
test-public-recording-genre-api:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_recording_genre_collection.cpp \
		-o $(BUILD_DIR)/test_public_recording_genre_collection
	$(BUILD_DIR)/test_public_recording_genre_collection

.PHONY: test-public-recording-playback-target-resolver
test-public-recording-playback-target-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/recordings/src/PublicRecordingIdentityRepository.cpp \
		core/recordings/src/PublicRecordingPlaybackTargetResolver.cpp \
		core/recordings/tests/test_public_recording_playback_target_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_recording_playback_target_resolver
	$(BUILD_DIR)/test_public_recording_playback_target_resolver

.PHONY: test-public-recording-device-playback-admission
test-public-recording-device-playback-admission:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/recordings/src/PublicRecordingIdentityRepository.cpp \
		core/recordings/src/PublicRecordingPlaybackTargetResolver.cpp \
		core/recordings/src/PublicRecordingDevicePlaybackAdmission.cpp \
		core/recordings/tests/test_public_recording_device_playback_admission.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_recording_device_playback_admission
	$(BUILD_DIR)/test_public_recording_device_playback_admission
