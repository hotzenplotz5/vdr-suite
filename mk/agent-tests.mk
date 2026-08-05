# Hosted CI builds and validates live probes; real VDR execution remains explicit and opt-in.
.PHONY: test-suite-bridge-agent-boundary test-suite-bridge-handshake test-suite-bridge-handshake-missing-plugin test-suite-bridge-svdrp-transport-boundary test-suite-bridge-svdrp-transport test-suite-bridge-svdrp-artwork-transport test-suite-bridge-svdrp-metadata-transport test-suite-bridge-svdrp-epg-type-snapshot-transport test-suite-bridge-svdrp-transport-live test-suite-bridge-observation-boundary test-suite-bridge-observation-service test-suite-bridge-observation-worker test-suite-bridge-embedded-runtime-boundary test-suite-bridge-embedded-runtime test-suite-bridge-daemon-runtime-wiring test-sb10d-live-acceptance-contract build-suite-bridge-embedded-runtime-live test-real-suite-bridge-embedded-runtime-live test-real-suite-bridge-observation-live

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
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_transport

test-suite-bridge-svdrp-artwork-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_artwork_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_artwork_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_artwork_transport

test-suite-bridge-svdrp-epg-type-snapshot-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_epg_type_snapshot_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_epg_type_snapshot_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_epg_type_snapshot_transport

test-suite-bridge-svdrp-metadata-transport: test-suite-bridge-svdrp-epg-type-snapshot-transport
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		$(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_metadata_transport.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_metadata_transport
	$(BUILD_DIR)/test_suite_bridge_svdrp_metadata_transport

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
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		$(AGENT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_embedded_agent_runtime.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime
	$(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime

test-suite-bridge-daemon-runtime-wiring:
	python3 tools/check_suite_bridge_daemon_runtime_wiring.py

test-sb10d-live-acceptance-contract:
	python3 tools/check_sb10d_live_acceptance_contract.py

$(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime_live:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		$(AGENT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_embedded_agent_runtime_live.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime_live

build-suite-bridge-embedded-runtime-live: $(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime_live

test-real-suite-bridge-embedded-runtime-live: build-suite-bridge-embedded-runtime-live
	$(BUILD_DIR)/test_suite_bridge_embedded_agent_runtime_live

test-suite-bridge-svdrp-transport-live:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		$(AGENT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_svdrp_transport_live.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_svdrp_transport_live
	$(BUILD_DIR)/test_suite_bridge_svdrp_transport_live

test-real-suite-bridge-observation-live:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/agent/include -Icore/vdr/include \
		$(AGENT_STANDALONE_SRC) \
		core/agent/tests/test_suite_bridge_observation_live.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_observation_live
	$(BUILD_DIR)/test_suite_bridge_observation_live

.PHONY: test-backend-agent-foundation test-backend-agent-client test-backend-agent-enrollment-tool test-backend-agent-admin-tool test-backend-agent-foundation-architecture

test-backend-agent-foundation-architecture:
	python3 tools/check_backend_agent_foundation.py

test-backend-agent-foundation: test-backend-agent-foundation-architecture
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		$(AGENT_CONTROL_PLANE_SRC) \
		core/agent/tests/test_backend_agent_lifecycle.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_lifecycle
	$(BUILD_DIR)/test_backend_agent_lifecycle

test-backend-agent-enrollment-tool: backend-agent-enrollment test-backend-agent-foundation-architecture
	python3 tools/test_backend_agent_enrollment_tool.py $(BUILD_DIR)/vdr-suite-backend-agent-enroll

test-backend-agent-admin-tool: backend-agent-admin backend-agent-enrollment test-backend-agent-foundation-architecture
	python3 tools/test_backend_agent_admin_tool.py \
		$(BUILD_DIR)/vdr-suite-backend-agent-admin \
		$(BUILD_DIR)/vdr-suite-backend-agent-enroll

test-backend-agent-client: test-backend-agent-foundation-architecture
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_CLIENT_SRC) \
		$(AGENT_CHANNEL_DOMAIN_SRC) \
		core/agent/tests/test_backend_agent_client.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_client
	$(BUILD_DIR)/test_backend_agent_client

test-fast: test-backend-agent-foundation test-backend-agent-client test-backend-agent-enrollment-tool test-backend-agent-admin-tool
test-architecture: test-backend-agent-foundation-architecture


.PHONY: test-phase63-command-delivery-runtime

test-phase63-command-delivery-runtime:
	python3 tools/check_phase63_command_delivery_runtime.py
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		$(AGENT_CONTROL_PLANE_DOMAIN_SRC) \
		core/agent/tests/test_backend_agent_command_delivery.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_command_delivery
	$(BUILD_DIR)/test_backend_agent_command_delivery

test-fast: test-phase63-command-delivery-runtime
