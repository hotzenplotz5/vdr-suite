.PHONY: test-phase67-hbbtv-suitebridge \
	test-phase67-hbbtv-agent-transport \
	test-phase67-hbbtv-resolver \
	test-phase67-hbbtv-discovery-foundation

test-phase67-hbbtv-suitebridge:
	$(MAKE) -C vdr-plugin-suite-bridge test-hbbtv
	$(MAKE) -C vdr-plugin-suite-bridge check-hbbtv-adapter-wiring
	$(MAKE) -C vdr-plugin-suite-bridge check-hbbtv-svdrp-boundary
	$(MAKE) -C vdr-plugin-suite-bridge check-build

test-phase67-hbbtv-agent-transport:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/agent/src/SuiteBridgeSvdrpTransport.cpp \
		core/agent/src/SuiteBridgeSvdrpHbbtvTransport.cpp \
		core/agent/tests/test_suite_bridge_svdrp_hbbtv_transport.cpp \
		-pthread $(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_hbbtv_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_hbbtv_transport

test-phase67-hbbtv-resolver:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SuiteBridgeHbbtvResolver.cpp \
		core/vdr/tests/test_suite_bridge_hbbtv_resolver.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suite_bridge_hbbtv_resolver
	$(BUILD_DIR)/test_suite_bridge_hbbtv_resolver

test-phase67-hbbtv-discovery-foundation: \
	test-phase67-hbbtv-suitebridge \
	test-phase67-hbbtv-agent-transport \
	test-phase67-hbbtv-resolver
