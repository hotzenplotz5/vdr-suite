.PHONY: test-suite-bridge-epg-metadata-resolver

test-suite-bridge-epg-metadata-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeEpgMetadataResolver.cpp \
		core/vdr/tests/test_suite_bridge_epg_metadata_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_epg_metadata_resolver
	$(BUILD_DIR)/test_suite_bridge_epg_metadata_resolver

# Keep the new transport and resolver reachable from the existing public groups.
test-ci-fast: test-suite-bridge-svdrp-metadata-transport test-suite-bridge-epg-metadata-resolver

test-vdr: test-suite-bridge-svdrp-metadata-transport test-suite-bridge-epg-metadata-resolver
