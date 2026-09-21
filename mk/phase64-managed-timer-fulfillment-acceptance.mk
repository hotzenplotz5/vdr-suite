.PHONY: phase64-managed-timer-fulfillment-acceptance test-phase64-managed-timer-fulfillment-acceptance-architecture

phase64-managed-timer-fulfillment-acceptance:
	tools/run_phase64_managed_timer_fulfillment_acceptance.sh

test-phase64-managed-timer-fulfillment-acceptance-architecture:
	python3 tools/check_phase64_managed_timer_fulfillment_acceptance.py

test-architecture: test-phase64-managed-timer-fulfillment-acceptance-architecture
