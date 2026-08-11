.PHONY: test-phase64-native-timer-binding-absence-application-architecture test-phase64-native-timer-binding-absence-application

test-phase64-native-timer-binding-absence-application-architecture:
	python3 tools/check_phase64_native_timer_binding_absence_application.py

test-phase64-native-timer-binding-absence-application: test-phase64-native-timer-binding-absence-application-architecture
	$(BUILD_CXX) $(CXXFLAGS) -Icore/sqlite/include -Icore/timers/include \
		$(SQLITE_SRC) \
		core/timers/src/NativeTimerBinding.cpp \
		core/timers/src/NativeTimerInventoryEvidence.cpp \
		core/timers/src/NativeTimerBindingRepository.cpp \
		core/timers/src/NativeTimerBindingReadRepository.cpp \
		core/timers/src/NativeTimerBindingWriteRepository.cpp \
		core/timers/src/NativeTimerBindingAbsenceApplicationService.cpp \
		core/timers/tests/test_native_timer_binding_absence_application_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_native_timer_binding_absence_application_service
	$(BUILD_DIR)/test_native_timer_binding_absence_application_service

test-fast: test-phase64-native-timer-binding-absence-application
test-architecture: test-phase64-native-timer-binding-absence-application-architecture
