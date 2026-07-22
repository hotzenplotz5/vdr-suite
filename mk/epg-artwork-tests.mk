.PHONY: test-suite-bridge-epg-artwork-resolver test-epg-artwork-repository test-epg-artwork-enrichment-service test-epg-artwork-controller test-epg-artwork-public-json-serializer test-epg-cache-artwork-json

test-suite-bridge-epg-artwork-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeEpgArtworkResolver.cpp \
		core/vdr/tests/test_suite_bridge_epg_artwork_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_epg_artwork_resolver
	$(BUILD_DIR)/test_suite_bridge_epg_artwork_resolver

test-epg-artwork-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgArtworkRepository.cpp \
		core/vdr/tests/test_epg_artwork_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_artwork_repository
	$(BUILD_DIR)/test_epg_artwork_repository

test-epg-artwork-enrichment-service:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		$(SQLITE_SRC) \
		core/vdr/src/EpgArtworkRepository.cpp \
		core/vdr/src/EpgArtworkEnrichmentService.cpp \
		core/vdr/tests/test_epg_artwork_enrichment_service.cpp \
		$(LDFLAGS) -pthread \
		-o $(BUILD_DIR)/test_epg_artwork_enrichment_service
	$(BUILD_DIR)/test_epg_artwork_enrichment_service

test-epg-artwork-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgArtworkRepository.cpp \
		api/rest/src/EpgArtworkController.cpp \
		api/rest/tests/test_epg_artwork_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_artwork_controller
	$(BUILD_DIR)/test_epg_artwork_controller

test-epg-artwork-public-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgArtworkPublicJsonSerializer.cpp \
		core/vdr/tests/test_epg_artwork_public_json_serializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_artwork_public_json_serializer
	$(BUILD_DIR)/test_epg_artwork_public_json_serializer

test-epg-cache-artwork-json:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrChangeState.cpp \
		core/vdr/src/EpgEventRepository.cpp \
		core/vdr/src/EpgArtworkRepository.cpp \
		core/vdr/src/EpgArtworkPublicJsonSerializer.cpp \
		core/vdr/src/EpgArtworkEnrichmentService.cpp \
		core/vdr/src/EpgCacheService.cpp \
		core/vdr/src/VdrService.cpp \
		$(EPG_SCRAPER_METADATA_CONTROLLER_SRC) \
		api/rest/src/EpgArtworkController.cpp \
		api/rest/src/EpgCacheController.cpp \
		api/rest/tests/test_epg_cache_artwork_json.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_cache_artwork_json
	$(BUILD_DIR)/test_epg_cache_artwork_json
