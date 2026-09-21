.PHONY: test-phase64-timer-intent-contract-architecture test-phase64-timer-intent-contract test-phase64-timer-intent-repository test-phase64-timer-assignment-contract test-phase64-timer-assignment-repository-architecture test-phase64-timer-assignment-repository test-phase64-timer-assignment-set-revision-architecture test-phase64-timer-assignment-set-revision test-phase64-timer-assignment-planner-architecture test-phase64-timer-assignment-planner test-phase64-timer-assignment-planner-source-channel test-phase64-timer-assignment-scheduling-architecture test-phase64-timer-assignment-scheduling test-phase64-timer-assignment-replica-scheduling-architecture test-phase64-timer-assignment-replica-scheduling test-phase64-native-timer-binding-contract-architecture test-phase64-native-timer-binding-contract test-phase64-native-timer-binding-repository-architecture test-phase64-native-timer-binding-repository test-phase64-vdr-native-timer-observation-mapper-architecture test-phase64-vdr-native-timer-observation-mapper test-phase64-native-timer-binding-readback-architecture test-phase64-native-timer-binding-readback

test-phase64-timer-intent-contract-architecture:
	python3 tools/check_phase64_timer_intent_contract.py

test-phase64-timer-assignment-repository-architecture:
	python3 tools/check_phase64_timer_assignment_repository.py

test-phase64-timer-assignment-set-revision-architecture:
	python3 tools/check_phase64_timer_assignment_set_revision.py

test-phase64-timer-assignment-planner-architecture:
	python3 tools/check_phase64_timer_assignment_planner.py

test-phase64-timer-assignment-scheduling-architecture:
	python3 tools/check_phase64_timer_assignment_scheduling.py

test-phase64-timer-assignment-replica-scheduling-architecture:
	python3 tools/check_phase64_timer_assignment_replica_scheduling.py

test-phase64-native-timer-binding-contract-architecture:
	python3 tools/check_phase64_native_timer_binding.py

test-phase64-native-timer-binding-repository-architecture:
	python3 tools/check_phase64_native_timer_binding_repository.py

test-phase64-vdr-native-timer-observation-mapper-architecture:
	python3 tools/check_phase64_vdr_native_timer_observation_mapper.py

test-phase64-native-timer-binding-readback-architecture:
	python3 tools/check_phase64_native_timer_binding_readback.py

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

test-phase64-timer-assignment-set-revision: test-phase64-timer-assignment-set-revision-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/TimerAssignmentSetRevisionRepository.cpp \
		core/timers/tests/test_timer_assignment_set_revision_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_assignment_set_revision_repository
	$(BUILD_DIR)/test_timer_assignment_set_revision_repository

test-phase64-timer-assignment-planner: test-phase64-timer-assignment-planner-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentPlanner.cpp \
		core/timers/tests/test_timer_assignment_planner.cpp \
		-o $(BUILD_DIR)/test_timer_assignment_planner
	$(BUILD_DIR)/test_timer_assignment_planner


test-phase64-timer-assignment-planner-source-channel: test-phase64-timer-assignment-planner-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentPlanner.cpp \
		core/timers/tests/test_timer_assignment_planner_source_channel.cpp \
		-o $(BUILD_DIR)/test_timer_assignment_planner_source_channel
	$(BUILD_DIR)/test_timer_assignment_planner_source_channel

test-phase64-timer-assignment-scheduling: test-phase64-timer-assignment-scheduling-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/TimerAssignmentSetRevisionRepository.cpp \
		core/timers/src/TimerAssignmentPlanner.cpp \
		core/timers/src/TimerAssignmentSchedulingService.cpp \
		core/timers/tests/test_timer_assignment_scheduling_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_assignment_scheduling_service
	$(BUILD_DIR)/test_timer_assignment_scheduling_service

test-phase64-timer-assignment-replica-scheduling: test-phase64-timer-assignment-replica-scheduling-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/TimerIntent.cpp \
		core/timers/src/TimerIntentRepository.cpp \
		core/timers/src/TimerAssignment.cpp \
		core/timers/src/TimerAssignmentRepository.cpp \
		core/timers/src/TimerAssignmentSetRevisionRepository.cpp \
		core/timers/src/TimerAssignmentPlanner.cpp \
		core/timers/src/TimerAssignmentSchedulingService.cpp \
		core/timers/tests/test_timer_assignment_replica_scheduling_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_timer_assignment_replica_scheduling_service
	$(BUILD_DIR)/test_timer_assignment_replica_scheduling_service

test-phase64-native-timer-binding-contract: test-phase64-native-timer-binding-contract-architecture test-phase64-native-timer-binding-repository-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/tests/test_native_timer_binding.cpp \
		-o $(BUILD_DIR)/test_native_timer_binding
	$(BUILD_DIR)/test_native_timer_binding

test-phase64-native-timer-binding-repository: test-phase64-native-timer-binding-repository-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/tests/test_native_timer_binding_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_binding_repository
	$(BUILD_DIR)/test_native_timer_binding_repository

test-phase64-vdr-native-timer-observation-mapper: test-phase64-vdr-native-timer-observation-mapper-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/timers/include -Icore/vdr/include \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/vdr/src/VdrNativeTimerObservationMapper.cpp \
		core/vdr/tests/test_vdr_native_timer_observation_mapper.cpp \
		-o $(BUILD_DIR)/test_vdr_native_timer_observation_mapper
	$(BUILD_DIR)/test_vdr_native_timer_observation_mapper

test-phase64-native-timer-binding-readback: test-phase64-native-timer-binding-readback-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerObservation.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerBindingReadbackService.cpp \
		core/timers/tests/test_native_timer_binding_readback_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_binding_readback_service
	$(BUILD_DIR)/test_native_timer_binding_readback_service

test-fast: test-phase64-timer-intent-contract test-phase64-timer-intent-repository test-phase64-timer-assignment-contract test-phase64-timer-assignment-repository test-phase64-timer-assignment-set-revision test-phase64-timer-assignment-planner test-phase64-timer-assignment-planner-source-channel test-phase64-timer-assignment-scheduling test-phase64-timer-assignment-replica-scheduling test-phase64-native-timer-binding-contract test-phase64-native-timer-binding-repository test-phase64-vdr-native-timer-observation-mapper test-phase64-native-timer-binding-readback
test-architecture: test-phase64-timer-intent-contract-architecture test-phase64-timer-assignment-repository-architecture test-phase64-timer-assignment-set-revision-architecture test-phase64-timer-assignment-planner-architecture test-phase64-timer-assignment-scheduling-architecture test-phase64-timer-assignment-replica-scheduling-architecture test-phase64-native-timer-binding-contract-architecture test-phase64-native-timer-binding-repository-architecture test-phase64-vdr-native-timer-observation-mapper-architecture test-phase64-native-timer-binding-readback-architecture
