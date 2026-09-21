.PHONY: phase64-reassignment-failover-acceptance test-phase64-reassignment-failover-acceptance-architecture

phase64-reassignment-failover-acceptance:
	tools/run_phase64_reassignment_failover_acceptance.sh

test-phase64-reassignment-failover-acceptance-architecture:
	python3 tools/check_phase64_reassignment_failover_acceptance.py

test-architecture: test-phase64-reassignment-failover-acceptance-architecture
