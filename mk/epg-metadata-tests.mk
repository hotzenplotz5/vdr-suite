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
		core/vdr/src/EpgSeriesArtworkFallbackRepository.cpp \
		core/vdr/src/EpgSeriesArtworkFallbackDeliveryService.cpp \
		core/vdr/tests/test_epg_series_artwork_fallback_delivery_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_series_artwork_fallback_delivery_service
	$(BUILD_DIR)/test_epg_series_artwork_fallback_delivery_service
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgSeriesArtworkFallbackRepository.cpp \
		core/vdr/src/EpgSeriesArtworkFallbackOrphanCleaner.cpp \
		core/vdr/tests/test_epg_series_artwork_fallback_orphan_cleaner.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_series_artwork_fallback_orphan_cleaner
	$(BUILD_DIR)/test_epg_series_artwork_fallback_orphan_cleaner
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/TmdbSeriesArtworkIncomingCleaner.cpp \
		core/vdr/tests/test_tmdb_series_artwork_incoming_cleaner.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_tmdb_series_artwork_incoming_cleaner
	$(BUILD_DIR)/test_tmdb_series_artwork_incoming_cleaner
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgArtworkPathPolicy.cpp \
		core/vdr/src/EpgSeriesArtworkFallbackRepository.cpp \
		core/vdr/src/EpgSeriesArtworkFallbackDeliveryService.cpp \
		core/vdr/src/PersistentSeriesArtworkFallbackResolver.cpp \
		core/vdr/tests/test_persistent_series_artwork_fallback_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_persistent_series_artwork_fallback_resolver
	$(BUILD_DIR)/test_persistent_series_artwork_fallback_resolver
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgArtworkPathPolicy.cpp \
		core/vdr/src/EpgArtworkRepository.cpp \
		core/vdr/src/EpgScraperMetadataPublicJsonSerializer.cpp \
		core/vdr/src/PersistentEpgScraperMetadataResolver.cpp \
		core/vdr/tests/test_persistent_epg_scraper_metadata_fallback_artwork.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_persistent_epg_scraper_metadata_fallback_artwork
	$(BUILD_DIR)/test_persistent_epg_scraper_metadata_fallback_artwork
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/CurlExternalArtworkHttpTransport.cpp \
		core/http/tests/test_curl_external_artwork_http_transport.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_curl_external_artwork_http_transport
	$(BUILD_DIR)/test_curl_external_artwork_http_transport
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgSeriesArtworkProviderCacheRepository.cpp \
		core/vdr/tests/test_epg_series_artwork_provider_cache_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_series_artwork_provider_cache_repository
	$(BUILD_DIR)/test_epg_series_artwork_provider_cache_repository
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/TmdbSeriesArtworkJson.cpp \
		core/vdr/src/TmdbSeriesArtworkProvider.cpp \
		core/vdr/tests/test_tmdb_series_artwork_provider.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_tmdb_series_artwork_provider
	$(BUILD_DIR)/test_tmdb_series_artwork_provider
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/TmdbSeriesArtworkJson.cpp \
		core/vdr/src/TmdbSeriesArtworkProvider.cpp \
		core/daemon/src/TmdbSeriesArtworkRuntimeConfig.cpp \
		core/daemon/tests/test_tmdb_series_artwork_runtime_config.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_tmdb_series_artwork_runtime_config
	$(BUILD_DIR)/test_tmdb_series_artwork_runtime_config
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/TmdbSeriesArtworkJson.cpp \
		core/vdr/src/TmdbSeriesArtworkProvider.cpp \
		core/vdr/src/TvmazeSeriesArtworkJson.cpp \
		core/vdr/src/TvmazeSeriesArtworkProvider.cpp \
		core/daemon/src/SeriesArtworkBackendSettingsService.cpp \
		core/daemon/tests/test_series_artwork_backend_settings_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_series_artwork_backend_settings_service
	$(BUILD_DIR)/test_series_artwork_backend_settings_service
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/TvmazeSeriesArtworkJson.cpp \
		core/vdr/src/TvmazeSeriesArtworkProvider.cpp \
		core/vdr/tests/test_tvmaze_series_artwork_provider.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_tvmaze_series_artwork_provider
	$(BUILD_DIR)/test_tvmaze_series_artwork_provider
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/TvmazeSeriesArtworkJson.cpp \
		core/vdr/src/TvmazeSeriesArtworkProvider.cpp \
		core/daemon/src/TvmazeSeriesArtworkRuntimeConfig.cpp \
		core/daemon/tests/test_tvmaze_series_artwork_runtime_config.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_tvmaze_series_artwork_runtime_config
	$(BUILD_DIR)/test_tvmaze_series_artwork_runtime_config
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
	python3 tools/check_epg_series_artwork_orphan_cleanup.py
	python3 tools/check_tmdb_series_artwork_incoming_cleanup.py
	python3 tools/check_tvmaze_series_artwork_provider.py
