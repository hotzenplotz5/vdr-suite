.PHONY: test-phase64-native-timer-present-readback-verification-architecture test-phase64-native-timer-present-readback-verification

test-phase64-native-timer-present-readback-verification-architecture:
	python3 tools/check_phase64_native_timer_present_readback_verification.py

test-phase64-native-timer-present-readback-verification: test-phase64-native-timer-present-readback-verification-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/timers/src/NativeTimerReadbackExpectation.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerPresentReadbackVerificationService.cpp \
		core/timers/tests/test_native_timer_present_readback_verification_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_present_readback_verification_service
	$(BUILD_DIR)/test_native_timer_present_readback_verification_service

test-fast: test-phase64-native-timer-present-readback-verification
test-architecture: test-phase64-native-timer-present-readback-verification-architecture
