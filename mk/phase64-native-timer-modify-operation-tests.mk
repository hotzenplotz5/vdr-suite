.PHONY: test-phase64-native-timer-modify-operation-architecture test-phase64-native-timer-modify-operation

test-phase64-native-timer-modify-operation-architecture:
	python3 tools/check_phase64_native_timer_modify_operation.py

test-phase64-native-timer-modify-operation: test-phase64-native-timer-modify-operation-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/timers/src/NativeTimerModifyOperationPayload.cpp \
		core/timers/src/NativeTimerModifyOperationPreparationService.cpp \
		core/timers/src/NativeTimerModifyReadbackVerificationService.cpp \
		core/timers/tests/test_native_timer_modify_operation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_modify_operation
	$(BUILD_DIR)/test_native_timer_modify_operation

test-fast: test-phase64-native-timer-modify-operation
test-architecture: test-phase64-native-timer-modify-operation-architecture
