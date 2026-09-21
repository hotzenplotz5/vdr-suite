.PHONY: test-phase64-native-timer-delete-operation-completion-architecture test-phase64-native-timer-delete-operation-completion

test-phase64-native-timer-delete-operation-completion-architecture:
	python3 tools/check_phase64_native_timer_delete_operation_completion.py

test-phase64-native-timer-delete-operation-completion: test-phase64-native-timer-delete-operation-completion-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerReadbackExpectation.cpp \
		core/timers/src/NativeTimerAbsenceReadbackExpectation.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerDeleteOperationCompletionService.cpp \
		core/timers/tests/test_native_timer_delete_operation_completion_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_delete_operation_completion_service
	$(BUILD_DIR)/test_native_timer_delete_operation_completion_service

test-fast: test-phase64-native-timer-delete-operation-completion
test-architecture: test-phase64-native-timer-delete-operation-completion-architecture
