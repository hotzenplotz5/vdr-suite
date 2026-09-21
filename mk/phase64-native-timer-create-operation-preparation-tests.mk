.PHONY: test-phase64-native-timer-create-operation-preparation-architecture test-phase64-native-timer-create-operation-preparation

test-phase64-native-timer-create-operation-preparation-architecture:
	python3 tools/check_phase64_native_timer_create_operation_preparation.py

test-phase64-native-timer-create-operation-preparation: test-phase64-native-timer-create-operation-preparation-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/NativeTimerCreateOperationPayload.cpp \
		core/timers/src/NativeTimerCreateOperationPreparationService.cpp \
		core/timers/tests/test_native_timer_create_operation_preparation_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_create_operation_preparation_service
	$(BUILD_DIR)/test_native_timer_create_operation_preparation_service

test-fast: test-phase64-native-timer-create-operation-preparation
test-architecture: test-phase64-native-timer-create-operation-preparation-architecture
