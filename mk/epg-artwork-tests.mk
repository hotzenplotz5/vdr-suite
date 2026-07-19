test-suite-bridge-epg-artwork-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeEpgArtworkResolver.cpp \
		core/vdr/tests/test_suite_bridge_epg_artwork_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_epg_artwork_resolver
	$(BUILD_DIR)/test_suite_bridge_epg_artwork_resolver
