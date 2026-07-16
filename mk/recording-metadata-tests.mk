.PHONY: \
	test-restful-api-recording-metadata-mapper \
	test-restful-api-recording-metadata-enricher \
	test-vdr-recording-metadata-cache-codec \
	test-vdr-recording-metadata-json-serializer \
	test-recording-metadata-foundation

test-restful-api-recording-metadata-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiRecordingMetadataMapper.cpp \
		core/vdr/tests/test_restful_api_recording_metadata_mapper.cpp \
		-o $(BUILD_DIR)/test_restful_api_recording_metadata_mapper
	$(BUILD_DIR)/test_restful_api_recording_metadata_mapper

test-restful-api-recording-metadata-enricher:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiRecordingMetadataMapper.cpp \
		core/vdr/src/RestfulApiRecordingMetadataEnricher.cpp \
		core/vdr/tests/test_restful_api_recording_metadata_enricher.cpp \
		-o $(BUILD_DIR)/test_restful_api_recording_metadata_enricher
	$(BUILD_DIR)/test_restful_api_recording_metadata_enricher

test-vdr-recording-metadata-cache-codec:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/tests/test_vdr_recording_metadata_cache_codec.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_metadata_cache_codec
	$(BUILD_DIR)/test_vdr_recording_metadata_cache_codec

test-vdr-recording-metadata-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrRecordingMetadataJsonSerializer.cpp \
		core/vdr/tests/test_vdr_recording_metadata_json_serializer.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_metadata_json_serializer
	$(BUILD_DIR)/test_vdr_recording_metadata_json_serializer

test-recording-metadata-foundation: \
	test-restful-api-recording-metadata-mapper \
	test-restful-api-recording-metadata-enricher \
	test-vdr-recording-metadata-cache-codec \
	test-vdr-recording-metadata-json-serializer

# The focused Timer-conflict adapter target compiles RestfulApiVdrAdapter.cpp
# without the shared VDR_SRC aggregate. Add the new metadata translation units
# only for that target while preserving its existing owner and recipe.
test-restful-api-vdr-adapter-timer-conflicts: CXXFLAGS += \
	core/vdr/src/RestfulApiRecordingMetadataMapper.cpp \
	core/vdr/src/RestfulApiRecordingMetadataEnricher.cpp

# Keep the metadata contract in the focused Recording path and the established
# fast regression graph without creating a second central test-list owner.
test-restful-api-recording-mapper: test-recording-metadata-foundation
test-fast: test-recording-metadata-foundation
