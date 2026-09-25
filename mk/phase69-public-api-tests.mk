.PHONY: test-phase69-public-api-contract-root

test-phase69-public-api-contract-root:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_api_contract_runtime.cpp \
		-o $(BUILD_DIR)/test_public_api_contract_runtime
	$(BUILD_DIR)/test_public_api_contract_runtime

.PHONY: test-phase69-public-resource-preconditions

test-phase69-public-resource-preconditions:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/tests/test_public_resource_preconditions.cpp \
		-o $(BUILD_DIR)/test_public_resource_preconditions
	$(BUILD_DIR)/test_public_resource_preconditions

.PHONY: test-phase69-operation-read-facade

test-phase69-operation-read-facade:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/operations/src/MutationOperationReadService.cpp \
		core/operations/tests/test_mutation_operation_read_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_mutation_operation_read_service
	$(BUILD_DIR)/test_mutation_operation_read_service

.PHONY: test-phase69-public-operation-resource

test-phase69-public-operation-resource:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_operation_resource.cpp \
		-o $(BUILD_DIR)/test_public_operation_resource
	$(BUILD_DIR)/test_public_operation_resource

.PHONY: test-phase69-timer-assignment-read-facade

test-phase69-timer-assignment-read-facade:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/TimerAssignmentDesiredNativeTimerSpecification.cpp \
		core/timers/src/TimerAssignmentReadService.cpp \
		core/timers/tests/test_timer_assignment_read_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_assignment_read_service
	$(BUILD_DIR)/test_timer_assignment_read_service

.PHONY: test-phase69-timer-assignment-runtime-composition

test-phase69-timer-assignment-runtime-composition:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_timer_assignment_lookup.cpp \
		-o $(BUILD_DIR)/test_public_timer_assignment_lookup
	$(BUILD_DIR)/test_public_timer_assignment_lookup
	python3 tools/check_phase69_timer_assignment_runtime_composition.py

.PHONY: test-phase69-public-timer-assignment-resource

test-phase69-public-timer-assignment-resource:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_timer_assignment_resource.cpp \
		-o $(BUILD_DIR)/test_public_timer_assignment_resource
	$(BUILD_DIR)/test_public_timer_assignment_resource
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_public_timer_assignment_read_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_timer_assignment_read_security
	$(BUILD_DIR)/test_public_timer_assignment_read_security
	python3 tools/check_phase69_public_timer_assignment_resource.py

.PHONY: test-phase69-native-timer-create-preparation-runtime

test-phase69-native-timer-create-preparation-runtime:
	python3 tools/check_phase69_native_timer_create_preparation_runtime.py

.PHONY: test-phase69-native-timer-create-dispatch-runtime

test-phase69-native-timer-create-dispatch-runtime:
	python3 tools/check_phase69_native_timer_create_dispatch_runtime.py

.PHONY: test-phase69-timer-assignment-native-specification

test-phase69-timer-assignment-native-specification:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/TimerAssignmentDesiredNativeTimerSpecification.cpp \
		core/timers/tests/test_timer_assignment_desired_native_specification.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_assignment_desired_native_specification
	$(BUILD_DIR)/test_timer_assignment_desired_native_specification
	python3 tools/check_phase69_timer_assignment_native_specification.py

.PHONY: test-phase69-native-timer-create-fulfillment-runtime

test-phase69-native-timer-create-fulfillment-runtime:
	python3 tools/check_phase69_native_timer_create_fulfillment_runtime.py

.PHONY: test-phase69-timer-create-identity-authority

test-phase69-timer-create-identity-authority:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/operations/include -Icore/timers/include \
		core/operations/src/MutationOperationIdentity.cpp \
		core/timers/src/NativeTimerBindingIdentity.cpp \
		core/timers/tests/test_timer_create_identity_authority.cpp \
		-o $(BUILD_DIR)/test_timer_create_identity_authority
	$(BUILD_DIR)/test_timer_create_identity_authority
	python3 tools/check_phase69_timer_create_identity_authority.py

.PHONY: test-phase69-atomic-timer-create-admission

test-phase69-atomic-timer-create-admission:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationIdentity.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentDesiredNativeTimerSpecification.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingIdentity.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/TimerAssignmentFulfillmentService.cpp \
		core/timers/src/NativeTimerCreateOperationPayload.cpp \
		core/timers/src/NativeTimerCreateOperationPreparationService.cpp \
		core/timers/src/NativeTimerCreateAdmissionService.cpp \
		core/timers/tests/test_native_timer_create_admission_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_create_admission_service
	$(BUILD_DIR)/test_native_timer_create_admission_service
	python3 tools/check_phase69_atomic_timer_create_admission.py

.PHONY: test-phase69-public-timer-create-admission

test-phase69-public-timer-create-admission:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/PublicApiRuntime.cpp \
		api/rest/tests/test_public_timer_create_admission.cpp \
		-o $(BUILD_DIR)/test_public_timer_create_admission
	$(BUILD_DIR)/test_public_timer_create_admission
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_public_timer_create_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_public_timer_create_security
	$(BUILD_DIR)/test_public_timer_create_security
	python3 tools/check_phase69_public_timer_create_admission.py

.PHONY: test-phase69-native-timer-create-reconciliation-runtime

test-phase69-native-timer-create-reconciliation-runtime:
	python3 tools/check_phase69_native_timer_create_reconciliation_runtime.py


.PHONY: test-phase69-native-timer-create-outcome-evidence

test-phase69-native-timer-create-outcome-evidence: test-phase64-native-timer-create-delivery
	python3 tools/check_phase69_native_timer_create_outcome_evidence.py


.PHONY: test-phase69-native-timer-create-outcome-application

test-phase69-native-timer-create-outcome-application: test-phase69-native-timer-create-outcome-evidence
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
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
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/NativeTimerCreateOperationPayload.cpp \
		core/timers/src/NativeTimerCreateReadbackExpectation.cpp \
		core/timers/src/NativeTimerCreateDispatchService.cpp \
		core/daemon/src/NativeTimerCreateResultOutcomeApplication.cpp \
		core/daemon/tests/test_native_timer_create_result_outcome_application.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_create_result_outcome_application
	$(BUILD_DIR)/test_native_timer_create_result_outcome_application
	python3 tools/check_phase69_native_timer_create_outcome_application.py

.PHONY: test-phase69-native-timer-create-readback-reconciliation

test-phase69-native-timer-create-readback-reconciliation: test-phase69-native-timer-create-outcome-application
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/TimerAssignmentDesiredNativeTimerSpecification.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/timers/src/NativeTimerCreateOperationPayload.cpp \
		core/timers/src/NativeTimerCreateReadbackExpectation.cpp \
		core/timers/src/NativeTimerCreateReadbackEvidence.cpp \
		core/timers/src/NativeTimerCreateReadbackVerificationService.cpp \
		core/timers/src/NativeTimerCreateDispatchService.cpp \
		core/timers/src/TimerAssignmentFulfillmentService.cpp \
		core/timers/src/NativeTimerCreateOperationCompletionService.cpp \
		core/daemon/src/NativeTimerCreateReadbackReconciliation.cpp \
		core/daemon/tests/test_native_timer_create_readback_reconciliation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_create_readback_reconciliation
	$(BUILD_DIR)/test_native_timer_create_readback_reconciliation
	python3 tools/check_phase69_native_timer_create_readback_reconciliation.py

