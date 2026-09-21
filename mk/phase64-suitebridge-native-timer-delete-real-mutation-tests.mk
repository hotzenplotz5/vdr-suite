.PHONY: test-phase64-suitebridge-native-timer-delete-real-mutation-architecture test-phase64-suitebridge-native-timer-delete-real-mutation

test-phase64-suitebridge-native-timer-delete-real-mutation-architecture:
	python3 tools/check_phase64_suitebridge_native_timer_delete_real_mutation.py

test-phase64-suitebridge-native-timer-delete-real-mutation: \
		test-phase64-suitebridge-native-timer-delete-real-mutation-architecture \
		test-phase64-suitebridge-native-timer-delete-replay-ledger-callback


test-fast: test-phase64-suitebridge-native-timer-delete-real-mutation
test-architecture: test-phase64-suitebridge-native-timer-delete-real-mutation-architecture
