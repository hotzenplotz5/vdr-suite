.PHONY: test-phase64-native-timer-binding-freshness

test-phase64-native-timer-binding-freshness:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/timers/src/NativeTimerBindingReadbackService.cpp \
		core/timers/tests/test_native_timer_binding_passive_refresh.cpp \
		$(LDFLAGS) -o $(BUILD_DIR)/test_native_timer_binding_passive_refresh
	$(BUILD_DIR)/test_native_timer_binding_passive_refresh

test-fast: test-phase64-native-timer-binding-freshness

test-ci-fast: test-phase64-native-timer-binding-freshness
