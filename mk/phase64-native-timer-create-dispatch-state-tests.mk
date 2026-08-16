.PHONY: test-phase64-native-timer-create-dispatch-state-architecture test-phase64-native-timer-create-dispatch-state

test-phase64-native-timer-create-dispatch-state-architecture:
	python3 tools/check_phase64_native_timer_create_dispatch_state.py

test-phase64-native-timer-create-dispatch-state: test-phase64-native-timer-create-dispatch-state-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/NativeTimerCreateOperationPayload.cpp \
		core/timers/src/NativeTimerCreateDispatchService.cpp \
		core/timers/tests/test_native_timer_create_dispatch_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_create_dispatch_service
	$(BUILD_DIR)/test_native_timer_create_dispatch_service

test-fast: test-phase64-native-timer-create-dispatch-state
test-architecture: test-phase64-native-timer-create-dispatch-state-architecture
