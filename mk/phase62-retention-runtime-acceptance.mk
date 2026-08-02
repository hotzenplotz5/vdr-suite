.PHONY: \
	test-phase62-retention-runtime-acceptance-runner \
	phase62-runtime-acceptance-retention-cleanup

PHASE62_RETENTION_ACCEPTANCE_RUNNER := \
	tools/phase62-runtime-acceptance/retention-cleanup-runner.py

PHASE62_RETENTION_ACCEPTANCE_IMPLEMENTATION := \
	tools/phase62-runtime-acceptance/retention_cleanup_runtime_runner.py

PHASE62_RETENTION_ACCEPTANCE_EXECUTION := \
	tools/phase62-runtime-acceptance/retention_cleanup_runtime_execution.py

PHASE62_RETENTION_ACCEPTANCE_SUPPORT := \
	tools/phase62-runtime-acceptance/retention_cleanup_runtime_support.py

PHASE62_RETENTION_ACCEPTANCE_PROCESS := \
	tools/phase62-runtime-acceptance/retention_cleanup_runtime_process.py

PHASE62_RETENTION_ACCEPTANCE_SCENARIOS := \
	tools/phase62-runtime-acceptance/retention_cleanup_runtime_scenarios.py

PHASE62_RETENTION_SYSTEMD_OVERRIDE := \
	tools/phase62-runtime-acceptance/retention_cleanup_systemd_override.py


test-phase62-runtime-acceptance-harness: \
	test-phase62-retention-runtime-acceptance-runner


test-phase62-retention-runtime-acceptance-runner:
	mkdir -p "$(BUILD_DIR)/python-cache"
	PYTHONPYCACHEPREFIX="$(BUILD_DIR)/python-cache" \
		python3 -m py_compile \
		"$(PHASE62_RETENTION_ACCEPTANCE_RUNNER)" \
		"$(PHASE62_RETENTION_ACCEPTANCE_IMPLEMENTATION)" \
		"$(PHASE62_RETENTION_ACCEPTANCE_EXECUTION)" \
		"$(PHASE62_RETENTION_ACCEPTANCE_SUPPORT)" \
		"$(PHASE62_RETENTION_ACCEPTANCE_PROCESS)" \
		"$(PHASE62_RETENTION_ACCEPTANCE_SCENARIOS)" \
		"$(PHASE62_RETENTION_SYSTEMD_OVERRIDE)"
	python3 "$(PHASE62_RETENTION_SYSTEMD_OVERRIDE)" --self-test
	PYTHONPATH="tools/phase62-runtime-acceptance" \
		python3 "$(PHASE62_RETENTION_ACCEPTANCE_PROCESS)" --self-test
	PYTHONPATH="tools/phase62-runtime-acceptance" \
		python3 "$(PHASE62_RETENTION_ACCEPTANCE_RUNNER)" --self-test


phase62-runtime-acceptance-retention-cleanup: \
	test-phase62-retention-runtime-acceptance-runner
	@test -n "$(PHASE62_RETENTION_EXPECTED_BRANCH)" || \
		{ echo "PHASE62_RETENTION_EXPECTED_BRANCH is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_EXPECTED_REMOTE_REF)" || \
		{ echo "PHASE62_RETENTION_EXPECTED_REMOTE_REF is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_EXPECTED_HEAD)" || \
		{ echo "PHASE62_RETENTION_EXPECTED_HEAD is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_EXPECTED_OLD_DAEMON_SHA256)" || \
		{ echo "PHASE62_RETENTION_EXPECTED_OLD_DAEMON_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_EXPECTED_NEW_DAEMON_SHA256)" || \
		{ echo "PHASE62_RETENTION_EXPECTED_NEW_DAEMON_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_EXPECTED_CONFIG_SHA256)" || \
		{ echo "PHASE62_RETENTION_EXPECTED_CONFIG_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_EXPECTED_LOADER_SHA256)" || \
		{ echo "PHASE62_RETENTION_EXPECTED_LOADER_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_EXPECTED_SERVICE_PID)" || \
		{ echo "PHASE62_RETENTION_EXPECTED_SERVICE_PID is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_SOURCE_CI_RUN)" || \
		{ echo "PHASE62_RETENTION_SOURCE_CI_RUN is required"; exit 2; }
	@test -n "$(PHASE62_RETENTION_SOURCE_CI_RUN_ID)" || \
		{ echo "PHASE62_RETENTION_SOURCE_CI_RUN_ID is required"; exit 2; }
	PYTHONPATH="tools/phase62-runtime-acceptance" \
		python3 "$(PHASE62_RETENTION_ACCEPTANCE_RUNNER)" \
			--run \
			--expected-branch \
				"$(PHASE62_RETENTION_EXPECTED_BRANCH)" \
			--expected-remote-ref \
				"$(PHASE62_RETENTION_EXPECTED_REMOTE_REF)" \
			--expected-head \
				"$(PHASE62_RETENTION_EXPECTED_HEAD)" \
			--expected-old-daemon-sha256 \
				"$(PHASE62_RETENTION_EXPECTED_OLD_DAEMON_SHA256)" \
			--expected-new-daemon-sha256 \
				"$(PHASE62_RETENTION_EXPECTED_NEW_DAEMON_SHA256)" \
			--expected-config-sha256 \
				"$(PHASE62_RETENTION_EXPECTED_CONFIG_SHA256)" \
			--expected-loader-sha256 \
				"$(PHASE62_RETENTION_EXPECTED_LOADER_SHA256)" \
			--expected-service-pid \
				"$(PHASE62_RETENTION_EXPECTED_SERVICE_PID)" \
			--source-ci-run \
				"$(PHASE62_RETENTION_SOURCE_CI_RUN)" \
			--source-ci-run-id \
				"$(PHASE62_RETENTION_SOURCE_CI_RUN_ID)"
