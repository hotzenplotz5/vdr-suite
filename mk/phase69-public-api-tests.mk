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
