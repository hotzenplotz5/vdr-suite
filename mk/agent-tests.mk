.PHONY: test-suite-bridge-agent-boundary test-suite-bridge-handshake test-suite-bridge-handshake-missing-plugin test-suite-bridge-svdrp-transport-boundary test-suite-bridge-svdrp-transport test-suite-bridge-svdrp-transport-live test-suite-bridge-observation-boundary test-suite-bridge-observation-service test-suite-bridge-observation-worker test-suite-bridge-embedded-runtime-boundary test-suite-bridge-embedded-runtime test-suite-bridge-daemon-runtime-wiring test-real-suite-bridge-observation-live

test-suite-bridge-agent-boundary:
	python3 tools/check_suite_bridge_agent_boundary.py

test-suite-bridge-handshake: test-suite-bridge-handshake-missing-plugin
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		core/agent/tests/test_suite_bridge_handshake.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_handshake
	$(BUILD_DIR)/test_suite_bridge_handshake

test-suite-bridge-handshake-missing-plugin:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		core/agent/tests/test_suite_bridge_handshake_missing_plugin.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_handshake_missing_plugin
	$(BUILD_DIR)/test_suite_bridge_handshake_missing_plugin

test-suite-bridge-svdrp-transport-boundary:
	python3 tools/check_suite_bridge_svdrp_transport_boundary.py

test-suite-bridge-svdrp-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SVDRP_TRANSPORT_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_transport

test-suite-bridge-observation-boundary:
	python3 tools/check_suite_bridge_observation_boundary.py

test-suite-bridge-observation-service:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		core/agent/src/SuiteBridgeObservation.cpp \
		core/agent/src/SuiteBridgeObservationService.cpp \
		core/agent/tests/test_suite_bridge_observation_service.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_observation_service
	$(BUILD_DIR)/test_suite_bridge_observation_service

test-suite-bridge-observation-worker:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_HANDSHAKE_SRC) \
		$(AGENT_OBSERVATION_SRC) \
		core/agent/tests/test_suite_bridge_observation_worker.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_observation_worker
	$(BUILD_DIR)/test_suite_bridge_observation_worker

test-suite-bridge-embedded-runtime-boundary:
	python3 tools/check_suite_bridge_embedded_runtime_boundary.py

test-suite-bridge-embedded-runtime:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SRC) \
		core/agent/tests/test_suite_bridge_embedded_agent_runtime.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime
	$(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime

test-suite-bridge-daemon-runtime-wiring:
	python3 tools/check_suite_bridge_daemon_runtime_wiring.py

test-suite-bridge-svdrp-transport-live:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_transport_live.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_transport_live
	$(BUILD_DIR)/test_suite_bridge_svdrp_transport_live

test-real-suite-bridge-observation-live:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include \
		$(AGENT_SRC) \
		core/agent/tests/test_suite_bridge_observation_live.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_observation_live
	$(BUILD_DIR)/test_suite_bridge_observation_live
