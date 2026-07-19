.PHONY: test-epg-artwork test-suite-bridge-epg-artwork-resolver test-epg-artwork-repository test-epg-artwork-enrichment-service

test-epg-artwork: \
	test-suite-bridge-svdrp-artwork-transport \
	test-suite-bridge-epg-artwork-resolver \
	test-epg-artwork-repository \
	test-epg-artwork-enrichment-service

# Keep the feature tests attached to both canonical backend test entrypoints
# without duplicating their recipes in the central inventory lists.
test-ci-fast test-vdr: test-epg-artwork

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
