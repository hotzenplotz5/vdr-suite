.PHONY: test-phase64-native-timer-create-readback-evidence

test-phase64-native-timer-create-readback-evidence:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/timers/src/NativeTimerCreateReadbackEvidence.cpp \
		core/timers/tests/test_native_timer_create_readback_evidence.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_create_readback_evidence
	$(BUILD_DIR)/test_native_timer_create_readback_evidence

test-fast: test-phase64-native-timer-create-readback-evidence
