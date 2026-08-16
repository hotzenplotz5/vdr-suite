.PHONY: test-phase64-native-timer-create-activation-architecture test-phase64-native-timer-create-activation

test-phase64-native-timer-create-activation-architecture:
	python3 tools/check_phase64_native_timer_create_activation.py

test-phase64-native-timer-create-activation: test-phase64-native-timer-create-activation-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/security/src/AccountabilityEventRepository.cpp \
		core/security/src/CredentialVerifierRepository.cpp \
		core/security/src/SecurityIdentityRepository.cpp \
		core/security/src/SecurityIdentityProvisioningRepository.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/NativeTimerCreateOperationPayload.cpp \
		core/timers/src/NativeTimerCreateDispatchService.cpp \
		$(AGENT_CONTROL_PLANE_DOMAIN_SRC) \
		core/agent/src/BackendAgentNativeTimerDelete.cpp \
		core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp \
		core/agent/src/BackendAgentCommandReservation.cpp \
		core/agent/src/BackendAgentNativeTimerCreateActivation.cpp \
		core/agent/tests/test_backend_agent_native_timer_create_activation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_agent_native_timer_create_activation
	$(BUILD_DIR)/test_backend_agent_native_timer_create_activation

test-fast: test-phase64-native-timer-create-activation
test-architecture: test-phase64-native-timer-create-activation-architecture
