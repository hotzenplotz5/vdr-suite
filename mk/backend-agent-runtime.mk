.PHONY: backend-agent backend-agent-enrollment backend-agent-admin backend-agent-command-admin

backend-agent:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(AGENT_CLIENT_SRC) \
		$(AGENT_CHANNEL_DOMAIN_SRC) \
		$(AGENT_COMMAND_DOMAIN_SRC) \
		apps/agent/main.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr-suite-backend-agent

backend-agent-enrollment:
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
		$(AGENT_CLIENT_SRC) \
		apps/tools/backend_agent_enrollment_create.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr-suite-backend-agent-enroll

backend-agent-admin:
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
		apps/tools/backend_agent_admin.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr-suite-backend-agent-admin

backend-agent-command-admin:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		$(AGENT_CONTROL_PLANE_DOMAIN_SRC) \
		apps/tools/backend_agent_command_admin.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr-suite-backend-agent-command-admin
