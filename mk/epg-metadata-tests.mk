.PHONY: test-suite-bridge-epg-metadata-resolver test-epg-scraper-metadata-public-json test-epg-scraper-metadata-controller test-epg-scraper-metadata-routes-contract

test-suite-bridge-epg-metadata-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_epg_scraper_metadata_identity.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_scraper_metadata_identity
	$(BUILD_DIR)/test_epg_scraper_metadata_identity
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_series_artwork_fallback_provider_contract.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_series_artwork_fallback_provider_contract
	$(BUILD_DIR)/test_series_artwork_fallback_provider_contract
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_series_artwork_fallback_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_series_artwork_fallback_resolver
	$(BUILD_DIR)/test_series_artwork_fallback_resolver
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_series_artwork_fallback_materializing_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_series_artwork_fallback_materializing_resolver
	$(BUILD_DIR)/test_series_artwork_fallback_materializing_resolver
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgArtworkPathPolicy.cpp \
		core/vdr/src/FilesystemSeriesArtworkFallbackMaterializer.cpp \
		core/vdr/tests/test_filesystem_series_artwork_fallback_materializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_filesystem_series_artwork_fallback_materializer
	$(BUILD_DIR)/test_filesystem_series_artwork_fallback_materializer
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgSeriesArtworkFallbackRepository.cpp \
		core/vdr/tests/test_epg_series_artwork_fallback_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_series_artwork_fallback_repository
	$(BUILD_DIR)/test_epg_series_artwork_fallback_repository
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgArtworkPathPolicy.cpp \
		core/vdr/src/EpgSeriesArtworkFallbackRepository.cpp \
		core/vdr/src/PersistentSeriesArtworkFallbackResolver.cpp \
		core/vdr/tests/test_persistent_series_artwork_fallback_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_persistent_series_artwork_fallback_resolver
	$(BUILD_DIR)/test_persistent_series_artwork_fallback_resolver
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeEpgMetadataResolver.cpp \
		core/vdr/tests/test_suite_bridge_epg_metadata_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_epg_metadata_resolver
	$(BUILD_DIR)/test_suite_bridge_epg_metadata_resolver

test-epg-scraper-metadata-public-json:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgScraperMetadataPublicJsonSerializer.cpp \
		core/vdr/tests/test_epg_scraper_metadata_public_json_serializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_scraper_metadata_public_json
	$(BUILD_DIR)/test_epg_scraper_metadata_public_json

test-epg-scraper-metadata-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgEventRepository.cpp \
		core/vdr/src/EpgArtworkRepository.cpp \
		core/vdr/src/EpgArtworkPublicJsonSerializer.cpp \
		core/vdr/src/EpgArtworkEnrichmentService.cpp \
		core/vdr/src/EpgCacheService.cpp \
		core/vdr/src/VdrService.cpp \
		$(EPG_SCRAPER_METADATA_CONTROLLER_SRC) \
		api/rest/src/EpgArtworkController.cpp \
		api/rest/src/EpgCacheController.cpp \
		api/rest/tests/test_epg_scraper_metadata_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_scraper_metadata_controller
	$(BUILD_DIR)/test_epg_scraper_metadata_controller

test-epg-scraper-metadata-routes-contract:
	python3 tools/check_epg_scraper_metadata_routes.py
