.PHONY: \
	test-metadata-make-boundary \
	test-metadata-manual-recording-architecture \
	test-metadata-identity \
	test-metadata-schema-contract \
	test-metadata-manual-recording-assignment \
	test-metadata-recording-candidate-provider \
	test-metadata-manual-recording-api \
	test-metadata-manual-recording-read-model \
	test-metadata-genres \
	test-metadata-genre-conflicts \
	test-metadata-foundation

test-metadata-make-boundary:
	python3 tools/check_metadata_make_boundary.py

test-metadata-manual-recording-architecture:
	python3 tools/check_manual_recording_metadata_architecture.py

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

test-metadata-manual-recording-assignment: CXXFLAGS += -Icore/metadata/include -Icore/vdr/include
test-metadata-manual-recording-assignment:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_PLATFORM_SRC) \
		core/metadata/src/ManualRecordingMetadataAssignmentRepository.cpp \
		$(METADATA_GENRE_SRC) \
		core/metadata/tests/test_manual_recording_metadata_assignment_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_manual_recording_metadata_assignment_repository
	$(BUILD_DIR)/test_manual_recording_metadata_assignment_repository

test-metadata-recording-candidate-provider: CXXFLAGS += -Icore/metadata/include -Icore/http/include
test-metadata-recording-candidate-provider:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/metadata/src/RecordingMetadataCandidateProvider.cpp \
		core/metadata/src/TmdbRecordingMetadataCandidateJson.cpp \
		core/metadata/src/TmdbRecordingMetadataCandidateProvider.cpp \
		core/metadata/src/TmdbRecordingMetadataPosterMaterializer.cpp \
		core/metadata/tests/test_tmdb_recording_metadata_candidate_provider.cpp \
		-o $(BUILD_DIR)/test_tmdb_recording_metadata_candidate_provider
	$(BUILD_DIR)/test_tmdb_recording_metadata_candidate_provider
	$(BUILD_CXX) $(CXXFLAGS) \
		core/metadata/src/RecordingMetadataCandidateProvider.cpp \
		core/metadata/src/TmdbRecordingMetadataCandidateJson.cpp \
		core/metadata/src/TmdbRecordingMetadataCandidateProvider.cpp \
		core/metadata/src/TmdbRecordingMetadataPosterMaterializer.cpp \
		core/metadata/tests/test_tmdb_recording_metadata_poster_materializer.cpp \
		-o $(BUILD_DIR)/test_tmdb_recording_metadata_poster_materializer
	$(BUILD_DIR)/test_tmdb_recording_metadata_poster_materializer

test-metadata-manual-recording-api: CXXFLAGS += -Icore/metadata/include -Icore/recordings/include -Icore/http/include -Iapi/rest/include
test-metadata-manual-recording-api:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_PLATFORM_SRC) \
		$(METADATA_GENRE_SRC) \
		$(MANUAL_RECORDING_METADATA_SRC) \
		core/recordings/src/MetadataRepository.cpp \
		core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp \
		core/http/src/CurlExternalArtworkHttpTransport.cpp \
		api/rest/src/MetadataController.cpp \
		api/rest/src/ManualRecordingMetadataApiRuntime.cpp \
		api/rest/tests/test_manual_recording_metadata_api_runtime.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_manual_recording_metadata_api_runtime
	$(BUILD_DIR)/test_manual_recording_metadata_api_runtime

test-metadata-manual-recording-read-model: CXXFLAGS += -Icore/metadata/include -Icore/recordings/include -Icore/http/include -Icore/vdr/include -Iapi/rest/include
test-metadata-manual-recording-read-model:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/src/VdrRecordingArtworkIdentity.cpp \
		core/vdr/src/VdrRecordingMetadataJsonSerializer.cpp \
		core/vdr/src/VdrRecordingNativeMetadataPublicJsonSerializer.cpp \
		core/vdr/src/EpgArtworkPathPolicy.cpp \
		core/vdr/src/EpgArtworkRepository.cpp \
		api/rest/src/EpgArtworkController.cpp \
		api/rest/src/VdrRecordingFolderController.cpp \
		api/rest/tests/test_vdr_recording_folder_manual_metadata.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_folder_manual_metadata
	$(BUILD_DIR)/test_vdr_recording_folder_manual_metadata

test-metadata-genres: CXXFLAGS += -Icore/metadata/include -Icore/vdr/include
test-metadata-genres:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_GENRE_SRC) \
		core/metadata/tests/test_genre_index_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_index_repository
	$(BUILD_DIR)/test_genre_index_repository

test-metadata-genre-conflicts: CXXFLAGS += -Icore/metadata/include -Icore/vdr/include
test-metadata-genre-conflicts:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_GENRE_SRC) \
		core/metadata/tests/test_genre_conflict_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_genre_conflict_repository
	$(BUILD_DIR)/test_genre_conflict_repository

test-metadata-foundation: \
	test-metadata-make-boundary \
	test-metadata-manual-recording-architecture \
	test-metadata-service \
	test-metadata-identity \
	test-metadata-schema-contract \
	test-metadata-manual-recording-assignment \
	test-metadata-recording-candidate-provider \
	test-metadata-manual-recording-api \
	test-metadata-manual-recording-read-model \
	test-metadata-genres \
	test-metadata-genre-conflicts

test-fast: test-metadata-foundation
