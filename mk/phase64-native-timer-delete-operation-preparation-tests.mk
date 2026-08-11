.PHONY: test-phase64-native-timer-delete-operation-preparation-architecture test-phase64-native-timer-delete-operation-preparation

test-phase64-native-timer-delete-operation-preparation-architecture:
	python3 tools/check_phase64_native_timer_delete_operation_preparation.py

test-phase64-native-timer-delete-operation-preparation: test-phase64-native-timer-delete-operation-preparation-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/operations/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerDeleteOperationPreparationService.cpp \
		core/timers/tests/test_native_timer_delete_operation_preparation_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_delete_operation_preparation_service
	$(BUILD_DIR)/test_native_timer_delete_operation_preparation_service

test-fast: test-phase64-native-timer-delete-operation-preparation
test-architecture: test-phase64-native-timer-delete-operation-preparation-architecture
