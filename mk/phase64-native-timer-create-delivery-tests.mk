.PHONY: test-phase64-native-timer-create-delivery-architecture test-phase64-native-timer-create-delivery

test-phase64-native-timer-create-delivery-architecture:
	python3 tools/check_phase64_native_timer_create_delivery.py

test-phase64-native-timer-create-delivery: test-phase64-native-timer-create-delivery-architecture
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
		core/agent/tests/test_backend_agent_native_timer_create_delivery.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_create_delivery
	$(BUILD_DIR)/test_backend_agent_native_timer_create_delivery

test-fast: test-phase64-native-timer-create-delivery
test-architecture: test-phase64-native-timer-create-delivery-architecture
