.PHONY: test-suitebridge-control-plane-architecture test-suitebridge-control-plane-agent test-suitebridge-control-plane-plugin test-suitebridge-control-plane

test-suitebridge-control-plane-architecture:
	python3 tools/check_suite_bridge_control_plane_architecture.py

test-suitebridge-control-plane-agent:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		core/agent/src/SuiteBridgeLocalControlTransport.cpp \
		core/agent/tests/test_suite_bridge_local_control_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_local_control_transport
	$(BUILD_DIR)/test_suite_bridge_local_control_transport

test-suitebridge-control-plane-plugin:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_control_plane.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_control_plane.cpp \
		-o $(BUILD_DIR)/test_suitebridge_control_plane
	$(BUILD_DIR)/test_suitebridge_control_plane

test-suitebridge-control-plane: test-suitebridge-control-plane-architecture test-suitebridge-control-plane-agent test-suitebridge-control-plane-plugin

test-fast: test-suitebridge-control-plane
test-architecture: test-suitebridge-control-plane-architecture
