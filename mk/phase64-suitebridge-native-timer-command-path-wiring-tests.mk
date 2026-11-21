.PHONY: test-phase64-suitebridge-native-timer-command-path-wiring-architecture test-phase64-suitebridge-native-timer-command-path-wiring

test-phase64-suitebridge-native-timer-command-path-wiring-architecture:
	python3 tools/check_phase64_suitebridge_native_timer_command_path_wiring.py

test-phase64-suitebridge-native-timer-command-path-wiring: \
		test-phase64-suitebridge-native-timer-command-path-wiring-architecture \
		test-backend-agent-client \
		test-phase64-suitebridge-native-timer-create-transport \
		test-phase64-suitebridge-native-timer-delete-disabled-transport
	$(MAKE) backend-agent

test-fast: test-phase64-suitebridge-native-timer-command-path-wiring
test-architecture: test-phase64-suitebridge-native-timer-command-path-wiring-architecture
