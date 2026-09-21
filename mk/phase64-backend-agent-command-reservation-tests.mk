.PHONY: test-phase64-backend-agent-command-reservation-architecture test-phase64-backend-agent-command-reservation

test-phase64-backend-agent-command-reservation-architecture:
	python3 tools/check_phase64_backend_agent_command_reservation.py

test-phase64-backend-agent-command-reservation: test-phase64-backend-agent-command-reservation-architecture
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		$(AGENT_CONTROL_PLANE_DOMAIN_SRC) \
		core/agent/src/BackendAgentNativeTimerDelete.cpp \
		core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp \
		core/agent/src/BackendAgentCommandReservation.cpp \
		core/agent/tests/test_backend_agent_command_reservation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_command_reservation
	$(BUILD_DIR)/test_backend_agent_command_reservation

test-fast: test-phase64-backend-agent-command-reservation
test-architecture: test-phase64-backend-agent-command-reservation-architecture
