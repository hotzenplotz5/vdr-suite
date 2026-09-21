.PHONY: test-phase68-legacy-osd-domain \
	test-phase68-suitebridge-osd-observation \
	test-phase68-suitebridge-osd-snapshot-contract \
	test-phase68-agent-osd-local-resync \
	test-phase68-legacy-osd-observation \
	test-phase68-osd-agent-local-resync

test-phase68-legacy-osd-domain:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_legacy_osd_domain.cpp \
		-o $(BUILD_DIR)/test_legacy_osd_domain
	$(BUILD_DIR)/test_legacy_osd_domain

test-phase68-suitebridge-osd-observation:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -pedantic \
		-Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_counter_continuity.cpp \
		vdr-plugin-suite-bridge/suitebridge_osd_state.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_osd_state.cpp \
		-o $(BUILD_DIR)/test_suitebridge_osd_state
	$(BUILD_DIR)/test_suitebridge_osd_state

test-phase68-suitebridge-osd-snapshot-contract:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -pedantic \
		-Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_osd_snapshot_contract.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_osd_snapshot_contract.cpp \
		-o $(BUILD_DIR)/test_suitebridge_osd_snapshot_contract
	$(BUILD_DIR)/test_suitebridge_osd_snapshot_contract

test-phase68-agent-osd-local-resync:
	$(BUILD_CXX) $(CXXFLAGS) \
		-Icore/agent/include \
		-Icore/vdr/include \
		core/agent/src/SuiteBridgeHandshake.cpp \
		$(AGENT_OSD_OBSERVATION_SRC) \
		core/agent/tests/test_suite_bridge_osd_frame_pipeline.cpp \
		-o $(BUILD_DIR)/test_suite_bridge_osd_frame_pipeline
	$(BUILD_DIR)/test_suite_bridge_osd_frame_pipeline

test-phase68-legacy-osd-observation: \
	test-phase68-legacy-osd-domain \
	test-phase68-suitebridge-osd-observation
	python3 tools/check_phase68_legacy_osd_observation.py

test-phase68-osd-agent-local-resync: \
	test-phase68-legacy-osd-observation \
	test-phase68-suitebridge-osd-snapshot-contract \
	test-phase68-agent-osd-local-resync \
	test-suite-bridge-svdrp-transport
	python3 tools/check_phase68_osd_agent_local_resync.py

# Phase 68.A and 68.B are read-only backend/domain slices and belong to
# normal fast/VDR CI. Neither target exposes OSD through HTTP or enables input.

.PHONY: test-phase68-osd-authenticated-transport
test-phase68-osd-authenticated-transport:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		$(sort $(SQLITE_SRC) $(AGENT_CONTROL_PLANE_SRC) $(AGENT_CLIENT_SRC) $(AGENT_OSD_OBSERVATION_SRC) $(AGENT_HANDSHAKE_SRC) $(AGENT_SVDRP_TRANSPORT_STANDALONE_SRC)) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/agent/tests/test_backend_agent_osd_transport.cpp \
		$(LDFLAGS) -o $(BUILD_DIR)/test_backend_agent_osd_transport
	$(BUILD_DIR)/test_backend_agent_osd_transport
