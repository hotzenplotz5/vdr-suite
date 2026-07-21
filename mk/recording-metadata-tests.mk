.PHONY: \
	test-restful-api-recording-metadata-mapper \
	test-restful-api-recording-metadata-enricher \
	test-vdr-recording-metadata-cache-codec \
	test-vdr-recording-cache-metadata-persistence \
	test-vdr-recording-metadata-json-serializer \
	test-vdr-recording-artwork-service \
	test-recording-artwork-http-server \
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

test-vdr-recording-cache-metadata-persistence:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_recording_cache_metadata_persistence.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_cache_metadata_persistence
	$(BUILD_DIR)/test_vdr_recording_cache_metadata_persistence

test-vdr-recording-metadata-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrRecordingArtworkIdentity.cpp \
		core/vdr/src/VdrRecordingMetadataJsonSerializer.cpp \
		core/vdr/tests/test_vdr_recording_metadata_json_serializer.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_metadata_json_serializer
	$(BUILD_DIR)/test_vdr_recording_metadata_json_serializer

test-vdr-recording-artwork-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/src/VdrRecordingArtworkIdentity.cpp \
		core/vdr/src/VdrRecordingArtworkService.cpp \
		core/vdr/tests/test_vdr_recording_artwork_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_artwork_service
	$(BUILD_DIR)/test_vdr_recording_artwork_service

test-recording-artwork-http-server:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/src/VdrRecordingArtworkIdentity.cpp \
		core/vdr/src/VdrRecordingArtworkService.cpp \
		api/rest/src/RestQueryParameters.cpp \
		core/daemon/src/RecordingArtworkHttpServer.cpp \
		core/daemon/tests/test_recording_artwork_http_server.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recording_artwork_http_server
	$(BUILD_DIR)/test_recording_artwork_http_server

test-recording-metadata-foundation: \
	test-restful-api-recording-metadata-mapper \
	test-restful-api-recording-metadata-enricher \
	test-vdr-recording-metadata-cache-codec \
	test-vdr-recording-cache-metadata-persistence \
	test-vdr-recording-metadata-json-serializer \
	test-vdr-recording-artwork-service \
	test-recording-artwork-http-server

# Focused targets which compile RestfulApiVdrAdapter.cpp or the Recording cache
# without the shared VDR_SRC aggregate need the new metadata translation units.
test-restful-api-vdr-adapter-timer-conflicts: CXXFLAGS += \
	core/vdr/src/RestfulApiRecordingMetadataMapper.cpp \
	core/vdr/src/RestfulApiRecordingMetadataEnricher.cpp

test-vdr-recording-cache-repository \
 test-vdr-recording-query-service-cache: CXXFLAGS += \
	core/vdr/src/VdrRecordingMetadataCacheCodec.cpp

test-vdr-recording-folder-controller: CXXFLAGS += \
	core/vdr/src/VdrRecordingMetadataCacheCodec.cpp \
	core/vdr/src/VdrRecordingArtworkIdentity.cpp

test-vdr-recording-query-controller \
 test-vdr-recording-query-result-json-serializer: CXXFLAGS += \
	core/vdr/src/VdrRecordingArtworkIdentity.cpp

# Keep the metadata contract in the focused Recording path and the established
# fast regression graph without creating a second central test-list owner.
test-restful-api-recording-mapper: test-recording-metadata-foundation
test-fast: test-recording-metadata-foundation
