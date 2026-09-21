.PHONY: test-phase64-suitebridge-native-timer-delete-replay-ledger-callback-architecture test-phase64-suitebridge-native-timer-delete-replay-ledger-callback

test-phase64-suitebridge-native-timer-delete-replay-ledger-callback-architecture:
	python3 tools/check_phase64_suitebridge_native_timer_delete_replay_ledger_callback.py

test-phase64-suitebridge-native-timer-delete-replay-ledger-callback: \
		test-phase64-suitebridge-native-timer-delete-replay-ledger-callback-architecture \
		test-phase64-suitebridge-native-timer-delete-disabled-transport


test-fast: test-phase64-suitebridge-native-timer-delete-replay-ledger-callback
test-architecture: test-phase64-suitebridge-native-timer-delete-replay-ledger-callback-architecture