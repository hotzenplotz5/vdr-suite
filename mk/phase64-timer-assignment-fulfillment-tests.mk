.PHONY: test-phase64-timer-assignment-fulfillment-architecture test-phase64-timer-assignment-fulfillment

test-phase64-timer-assignment-fulfillment-architecture:
	python3 tools/check_phase64_timer_assignment_fulfillment.py

test-phase64-timer-assignment-fulfillment: test-phase64-timer-assignment-fulfillment-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/TimerAssignmentFulfillmentService.cpp \
		core/timers/tests/test_timer_assignment_fulfillment_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_assignment_fulfillment_service
	$(BUILD_DIR)/test_timer_assignment_fulfillment_service

test-fast: test-phase64-timer-assignment-fulfillment
test-architecture: test-phase64-timer-assignment-fulfillment-architecture
