.PHONY: test-manual-recording-metadata-revision

test-manual-recording-metadata-revision: CXXFLAGS += -Icore/metadata/include -Icore/recordings/include -Icore/http/include -Icore/vdr/include
test-manual-recording-metadata-revision:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_PLATFORM_SRC) \
		$(METADATA_GENRE_SRC) \
		$(MANUAL_RECORDING_METADATA_SRC) \
		core/recordings/src/MetadataRepository.cpp \
		core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp \
		core/http/src/CurlExternalArtworkHttpTransport.cpp \
		core/recordings/tests/test_manual_recording_metadata_revision.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_manual_recording_metadata_revision
	$(BUILD_DIR)/test_manual_recording_metadata_revision

test-fast: test-manual-recording-metadata-revision
