.PHONY: test-restful-api-recording-metadata-mapper

test-restful-api-recording-metadata-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiRecordingMetadataMapper.cpp \
		core/vdr/tests/test_restful_api_recording_metadata_mapper.cpp \
		-o $(BUILD_DIR)/test_restful_api_recording_metadata_mapper
	$(BUILD_DIR)/test_restful_api_recording_metadata_mapper

# Keep the new contract in both the focused Recording mapper path and the
# established fast regression graph without rewriting the central test lists.
test-restful-api-recording-mapper: test-restful-api-recording-metadata-mapper
test-fast: test-restful-api-recording-metadata-mapper
