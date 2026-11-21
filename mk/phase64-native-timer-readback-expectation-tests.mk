.PHONY: test-phase64-native-timer-readback-expectation-architecture test-phase64-native-timer-readback-expectation

test-phase64-native-timer-readback-expectation-architecture:
	python3 tools/check_phase64_native_timer_readback_expectation.py

test-phase64-native-timer-readback-expectation: test-phase64-native-timer-readback-expectation-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerReadbackExpectation.cpp \
		core/timers/tests/test_native_timer_readback_expectation.cpp \
		-o $(BUILD_DIR)/test_native_timer_readback_expectation
	$(BUILD_DIR)/test_native_timer_readback_expectation

test-fast: test-phase64-native-timer-readback-expectation
test-architecture: test-phase64-native-timer-readback-expectation-architecture
