.PHONY: test-phase64-timer-delete-local-state-lifecycle-architecture test-phase64-timer-delete-local-state-lifecycle

test-phase64-timer-delete-local-state-lifecycle-architecture:
	python3 tools/check_phase64_timer_delete_local_state_lifecycle.py

test-phase64-timer-delete-local-state-lifecycle: \
		test-phase64-timer-delete-local-state-lifecycle-architecture \
		test-phase64-command-state-v3-extension
	@echo "Phase 64 Timer-delete local-state lifecycle regression passed"

test-fast: test-phase64-timer-delete-local-state-lifecycle
test-architecture: test-phase64-timer-delete-local-state-lifecycle-architecture
