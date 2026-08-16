.PHONY: test-phase64-vdr-timer-managed-correlation

test-phase64-vdr-timer-managed-correlation:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/vdr/include \
		core/vdr/src/VdrTimerManagedCorrelation.cpp \
		core/vdr/tests/test_vdr_timer_managed_correlation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_timer_managed_correlation
	$(BUILD_DIR)/test_vdr_timer_managed_correlation

test-fast: test-phase64-vdr-timer-managed-correlation
