.PHONY: test-phase64-vdr-managed-timer-create-request-architecture test-phase64-vdr-managed-timer-create-request

test-phase64-vdr-managed-timer-create-request-architecture:
	python3 tools/check_phase64_vdr_managed_timer_create_request.py

test-phase64-vdr-managed-timer-create-request: test-phase64-vdr-managed-timer-create-request-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include -Icore/vdr/include \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerSpecification.cpp \
		core/vdr/src/VdrTimerManagedCorrelation.cpp \
		core/vdr/src/VdrManagedTimerCreateRequestBuilder.cpp \
		core/vdr/tests/test_vdr_managed_timer_create_request_builder.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_managed_timer_create_request_builder
	$(BUILD_DIR)/test_vdr_managed_timer_create_request_builder

test-fast: test-phase64-vdr-managed-timer-create-request
test-architecture: test-phase64-vdr-managed-timer-create-request-architecture
