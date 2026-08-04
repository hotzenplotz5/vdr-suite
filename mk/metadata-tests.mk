.PHONY: \
	test-metadata-make-boundary \
	test-metadata-identity \
	test-metadata-schema-contract \
	test-metadata-manual-recording-assignment \
	test-metadata-recording-candidate-provider \
	test-metadata-genres \
	test-metadata-genre-conflicts \
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
		core/metadata/tests/test_tmdb_recording_metadata_candidate_provider.cpp \
		-o $(BUILD_DIR)/test_tmdb_recording_metadata_candidate_provider
	$(BUILD_DIR)/test_tmdb_recording_metadata_candidate_provider

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
	test-metadata-service \
	test-metadata-identity \
	test-metadata-schema-contract \
	test-metadata-manual-recording-assignment \
	test-metadata-recording-candidate-provider \
	test-metadata-genres \
	test-metadata-genre-conflicts

test-fast: test-metadata-foundation
