.PHONY: test-phase64-suitebridge-native-timer-create-command-service-architecture test-phase64-suitebridge-native-timer-create-command-service

test-phase64-suitebridge-native-timer-create-command-service-architecture:
	python3 tools/check_phase64_suitebridge_native_timer_create_command_service.py

test-phase64-suitebridge-native-timer-create-command-service: \
		test-phase64-suitebridge-native-timer-create-command-service-architecture
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Ivdr-plugin-suite-bridge \
		vdr-plugin-suite-bridge/suitebridge_native_timer_create.cpp \
		vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_create.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_suitebridge_native_timer_create
	$(BUILD_DIR)/test_suitebridge_native_timer_create

test-fast: test-phase64-suitebridge-native-timer-create-command-service
test-architecture: test-phase64-suitebridge-native-timer-create-command-service-architecture
