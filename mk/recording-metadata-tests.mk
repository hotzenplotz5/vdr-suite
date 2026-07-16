.PHONY: test-restful-api-recording-metadata-mapper

test-restful-api-recording-metadata-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiRecordingMetadataMapper.cpp \
		core/vdr/tests/test_restful_api_recording_metadata_mapper.cpp \
		-o $(BUILD_DIR)/test_restful_api_recording_metadata_mapper
	$(BUILD_DIR)/test_restful_api_recording_metadata_mapper
