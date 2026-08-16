.PHONY: test-phase64-native-timer-create-readback-expectation

test-phase64-native-timer-create-readback-expectation:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/src/NativeTimerCreateReadbackExpectation.cpp \
		core/timers/tests/test_native_timer_create_readback_expectation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_create_readback_expectation
	$(BUILD_DIR)/test_native_timer_create_readback_expectation

test-fast: test-phase64-native-timer-create-readback-expectation
