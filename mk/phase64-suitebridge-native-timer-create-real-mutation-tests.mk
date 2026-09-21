.PHONY: test-phase64-suitebridge-native-timer-create-real-mutation-architecture test-phase64-suitebridge-native-timer-create-real-mutation

test-phase64-suitebridge-native-timer-create-real-mutation-architecture:
	python3 tools/check_phase64_suitebridge_native_timer_create_real_mutation.py

test-phase64-suitebridge-native-timer-create-real-mutation: \
		test-phase64-suitebridge-native-timer-create-command-service \
		test-phase64-suitebridge-native-timer-create-real-mutation-architecture

test-fast: test-phase64-suitebridge-native-timer-create-real-mutation
test-architecture: test-phase64-suitebridge-native-timer-create-real-mutation-architecture
