.PHONY: test-systemd-unit-contract

test-ci-fast: test-systemd-unit-contract

test-systemd-unit-contract:
	python3 tools/check_systemd_unit_contract.py
