.PHONY: test-phase64-native-timer-modify-dispatch-architecture test-phase64-native-timer-modify-dispatch

test-phase64-native-timer-modify-dispatch-architecture:
	python3 tools/check_phase64_native_timer_modify_dispatch.py

test-phase64-native-timer-modify-dispatch: test-phase64-native-timer-modify-dispatch-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/timers/src/NativeTimerModifyOperationPayload.cpp \
		core/timers/src/NativeTimerModifyReadbackVerificationService.cpp \
		core/timers/src/NativeTimerModifyDispatchService.cpp \
		core/timers/src/NativeTimerModifyOperationCompletionService.cpp \
		core/timers/tests/test_native_timer_modify_dispatch.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_modify_dispatch
	$(BUILD_DIR)/test_native_timer_modify_dispatch

test-fast: test-phase64-native-timer-modify-dispatch
test-architecture: test-phase64-native-timer-modify-dispatch-architecture
