.PHONY: test-phase64-native-timer-specification

test-phase64-native-timer-specification:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/timers/tests/test_native_timer_specification.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_specification
	$(BUILD_DIR)/test_native_timer_specification

test-fast: test-phase64-native-timer-specification
