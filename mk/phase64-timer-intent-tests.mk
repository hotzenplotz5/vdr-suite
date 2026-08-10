.PHONY: test-phase64-timer-intent-contract-architecture test-phase64-timer-intent-contract test-phase64-timer-intent-repository test-phase64-timer-assignment-contract test-phase64-timer-assignment-repository-architecture test-phase64-timer-assignment-repository test-phase64-timer-assignment-planner-architecture test-phase64-timer-assignment-planner

test-phase64-timer-intent-contract-architecture:
	python3 tools/check_phase64_timer_intent_contract.py

test-phase64-timer-assignment-repository-architecture:
	python3 tools/check_phase64_timer_assignment_repository.py

test-phase64-timer-assignment-planner-architecture:
	python3 tools/check_phase64_timer_assignment_planner.py

test-phase64-timer-intent-contract: test-phase64-timer-intent-contract-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/TimerIntent.cpp \
		core/timers/tests/test_timer_intent.cpp \
		-o $(BUILD_DIR)/test_timer_intent
	$(BUILD_DIR)/test_timer_intent

test-phase64-timer-intent-repository: test-phase64-timer-intent-contract-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/tests/test_timer_intent_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_intent_repository
	$(BUILD_DIR)/test_timer_intent_repository

test-phase64-timer-assignment-contract: test-phase64-timer-intent-contract-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/TimerAssignment.cpp \
		core/timers/tests/test_timer_assignment.cpp \
		-o $(BUILD_DIR)/test_timer_assignment
	$(BUILD_DIR)/test_timer_assignment

test-phase64-timer-assignment-repository: test-phase64-timer-assignment-repository-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/tests/test_timer_assignment_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_assignment_repository
	$(BUILD_DIR)/test_timer_assignment_repository

test-phase64-timer-assignment-planner: test-phase64-timer-assignment-planner-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentPlanner.cpp \
		core/timers/tests/test_timer_assignment_planner.cpp \
		-o $(BUILD_DIR)/test_timer_assignment_planner
	$(BUILD_DIR)/test_timer_assignment_planner

test-fast: test-phase64-timer-intent-contract test-phase64-timer-intent-repository test-phase64-timer-assignment-contract test-phase64-timer-assignment-repository test-phase64-timer-assignment-planner
test-architecture: test-phase64-timer-intent-contract-architecture test-phase64-timer-assignment-repository-architecture test-phase64-timer-assignment-planner-architecture
