.PHONY: test-phase64-suitebridge-native-timer-modify-command-service test-phase64-suitebridge-native-timer-modify-command-service-architecture

test-phase64-suitebridge-native-timer-modify-command-service-architecture:
	python3 tools/check_phase64_suitebridge_native_timer_modify_command_service.py

test-phase64-suitebridge-native-timer-modify-command-service: test-phase64-suitebridge-native-timer-modify-command-service-architecture
	$(MAKE) -C vdr-plugin-suite-bridge test-native-timer-modify

test-fast: test-phase64-suitebridge-native-timer-modify-command-service
test-architecture: test-phase64-suitebridge-native-timer-modify-command-service-architecture
