.PHONY: test-phase64-native-timer-modify-command-path test-phase64-native-timer-modify-command-path-architecture

test-phase64-native-timer-modify-command-path:
	python3 tools/check_phase64_native_timer_modify_command_path.py

test-phase64-native-timer-modify-command-path-architecture: test-phase64-native-timer-modify-command-path

test-fast: test-phase64-native-timer-modify-command-path
test-architecture: test-phase64-native-timer-modify-command-path-architecture
