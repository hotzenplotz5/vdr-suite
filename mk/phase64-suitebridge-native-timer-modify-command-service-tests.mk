.PHONY: test-phase64-suitebridge-native-timer-modify-command-service

test-phase64-suitebridge-native-timer-modify-command-service:
	python3 tools/check_phase64_suitebridge_native_timer_modify_command_service.py
	$(MAKE) -C vdr-plugin-suite-bridge test-native-timer-modify
