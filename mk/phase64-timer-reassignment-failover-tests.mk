.PHONY: test-phase64-timer-reassignment-failover-architecture test-phase64-timer-reassignment-failover

test-phase64-timer-reassignment-failover-architecture:
	python3 tools/check_phase64_timer_reassignment_failover.py

test-phase64-timer-reassignment-failover: test-phase64-timer-reassignment-failover-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/timers/include -Icore/operations/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/TimerAssignmentSetRevisionRepository.cpp \
		core/timers/src/TimerAssignmentReassignmentRepository.cpp \
		core/timers/src/TimerAssignmentPlanner.cpp \
		core/timers/src/TimerAssignmentSchedulingService.cpp \
		core/timers/src/TimerAssignmentReassignmentService.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/operations/src/MutationOperation.cpp \
		core/operations/src/MutationOperationRepository.cpp \
		core/timers/tests/test_timer_assignment_reassignment_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_assignment_reassignment_service
	$(BUILD_DIR)/test_timer_assignment_reassignment_service

test-fast: test-phase64-timer-reassignment-failover
test-architecture: test-phase64-timer-reassignment-failover-architecture
