.PHONY: test-public-searchtimer-identity-lifecycle

test-public-searchtimer-identity-lifecycle: CXXFLAGS += -Icore/vdr/include
test-public-searchtimer-identity-lifecycle:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/PublicSearchTimerIdentityRepository.cpp \
		core/vdr/tests/test_public_search_timer_identity_lifecycle.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_search_timer_identity_lifecycle
	$(BUILD_DIR)/test_public_search_timer_identity_lifecycle

test-fast: test-public-searchtimer-identity-lifecycle

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
		$(REST_PUBLIC_API_SRC) \
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
		$(REST_PUBLIC_API_SRC) \
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

.PHONY: test-public-device-playback-registry
test-public-device-playback-registry:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) $(SECURITY_SRC) \
		core/media/src/MediaSessionRepository.cpp \
		core/media/src/MediaSessionIssuanceService.cpp \
		core/media/tests/test_public_device_playback_registry.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_device_playback_registry
	$(BUILD_DIR)/test_public_device_playback_registry

.PHONY: test-public-recording-metadata-detail-projection
test-public-recording-metadata-detail-projection:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_public_recording_metadata_detail_projection.cpp \
		-o $(BUILD_DIR)/test_public_recording_metadata_detail_projection
	$(BUILD_DIR)/test_public_recording_metadata_detail_projection

# The existing R2 focused CI and CI_FAST_TESTS already invoke this target.
# Reach the M1 projection regression without editing shared test-group files.
test-public-recording-collection-projection: \
	test-public-recording-metadata-detail-projection

# M1 tests deliberately reuse the existing public Recording route and grant
# fixtures, rather than creating a parallel HTTP or security harness.
test-fast: test-public-recording-metadata-detail-projection \
	test-public-recording-collection-api \
	test-public-recording-collection-security
