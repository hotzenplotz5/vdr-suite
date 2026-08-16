.PHONY: test-phase64-timer-intent-application-architecture test-phase64-timer-intent-application

test-phase64-timer-intent-application-architecture:
	python3 tools/check_phase64_timer_intent_application.py

test-phase64-timer-intent-application: test-phase64-timer-intent-application-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/TimerAssignmentSetRevisionRepository.cpp \
		core/timers/src/TimerAssignmentPlanner.cpp \
		core/timers/src/TimerAssignmentSchedulingService.cpp \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/TimerAssignmentFulfillmentService.cpp \
		core/timers/src/TimerIntentApplicationService.cpp \
		core/timers/tests/test_timer_intent_application_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_intent_application_service
	$(BUILD_DIR)/test_timer_intent_application_service

test-fast: test-phase64-timer-intent-application
test-architecture: test-phase64-timer-intent-application-architecture
