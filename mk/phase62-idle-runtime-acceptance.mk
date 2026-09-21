.PHONY: \
	test-phase62-idle-runtime-acceptance-runner \
	phase62-runtime-acceptance-idle-expiry

PHASE62_IDLE_ACCEPTANCE_RUNNER := \
	tools/phase62-runtime-acceptance/idle-expiry-runner-entry.py

PHASE62_IDLE_ACCEPTANCE_IMPLEMENTATION := \
	tools/phase62-runtime-acceptance/idle-expiry-runner.py

PHASE62_IDLE_ACCOUNTABILITY_CONTRACT := \
	tools/phase62-runtime-acceptance/idle_expiry_audit_contract.py

PHASE62_IDLE_SYSTEMD_OVERRIDE := \
	tools/phase62-runtime-acceptance/idle_expiry_systemd_override.py

PHASE62_IDLE_CRYPT_COMPAT := \
	tools/phase62-runtime-acceptance/crypt.py


test-phase62-runtime-acceptance-harness: \
	test-phase62-idle-runtime-acceptance-runner


test-phase62-idle-runtime-acceptance-runner:
	mkdir -p "$(BUILD_DIR)/python-cache"
	PYTHONPYCACHEPREFIX="$(BUILD_DIR)/python-cache" \
		python3 -m py_compile \
		"$(PHASE62_IDLE_ACCEPTANCE_RUNNER)" \
		"$(PHASE62_IDLE_ACCEPTANCE_IMPLEMENTATION)" \
		"$(PHASE62_IDLE_ACCOUNTABILITY_CONTRACT)" \
		"$(PHASE62_IDLE_SYSTEMD_OVERRIDE)" \
		"$(PHASE62_IDLE_CRYPT_COMPAT)"
	python3 "$(PHASE62_IDLE_SYSTEMD_OVERRIDE)" --self-test
	python3 "$(PHASE62_IDLE_ACCEPTANCE_RUNNER)" --self-test-loader
	python3 "$(PHASE62_IDLE_ACCOUNTABILITY_CONTRACT)" --self-test
	PYTHONPATH="tools/phase62-runtime-acceptance" \
		python3 -c 'import crypt; value = crypt.crypt("phase62-smoke", "$$6$$phase62smoke$$"); assert value and value.startswith("$$6$$")'


phase62-runtime-acceptance-idle-expiry: \
	test-phase62-idle-runtime-acceptance-runner
	@test -n "$(PHASE62_IDLE_EXPECTED_BRANCH)" || \
		{ echo "PHASE62_IDLE_EXPECTED_BRANCH is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_EXPECTED_REMOTE_REF)" || \
		{ echo "PHASE62_IDLE_EXPECTED_REMOTE_REF is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_EXPECTED_HEAD)" || \
		{ echo "PHASE62_IDLE_EXPECTED_HEAD is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_EXPECTED_OLD_DAEMON_SHA256)" || \
		{ echo "PHASE62_IDLE_EXPECTED_OLD_DAEMON_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_EXPECTED_NEW_DAEMON_SHA256)" || \
		{ echo "PHASE62_IDLE_EXPECTED_NEW_DAEMON_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_EXPECTED_CONFIG_SHA256)" || \
		{ echo "PHASE62_IDLE_EXPECTED_CONFIG_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_EXPECTED_LOADER_SHA256)" || \
		{ echo "PHASE62_IDLE_EXPECTED_LOADER_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_EXPECTED_SERVICE_PID)" || \
		{ echo "PHASE62_IDLE_EXPECTED_SERVICE_PID is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_SOURCE_CI_RUN)" || \
		{ echo "PHASE62_IDLE_SOURCE_CI_RUN is required"; exit 2; }
	@test -n "$(PHASE62_IDLE_SOURCE_CI_RUN_ID)" || \
		{ echo "PHASE62_IDLE_SOURCE_CI_RUN_ID is required"; exit 2; }
	python3 "$(PHASE62_IDLE_ACCEPTANCE_RUNNER)" \
		--run \
		--expected-branch \
			"$(PHASE62_IDLE_EXPECTED_BRANCH)" \
		--expected-remote-ref \
			"$(PHASE62_IDLE_EXPECTED_REMOTE_REF)" \
		--expected-head \
			"$(PHASE62_IDLE_EXPECTED_HEAD)" \
		--expected-old-daemon-sha256 \
			"$(PHASE62_IDLE_EXPECTED_OLD_DAEMON_SHA256)" \
		--expected-new-daemon-sha256 \
			"$(PHASE62_IDLE_EXPECTED_NEW_DAEMON_SHA256)" \
		--expected-config-sha256 \
			"$(PHASE62_IDLE_EXPECTED_CONFIG_SHA256)" \
		--expected-loader-sha256 \
			"$(PHASE62_IDLE_EXPECTED_LOADER_SHA256)" \
		--expected-service-pid \
			"$(PHASE62_IDLE_EXPECTED_SERVICE_PID)" \
		--source-ci-run \
			"$(PHASE62_IDLE_SOURCE_CI_RUN)" \
		--source-ci-run-id \
			"$(PHASE62_IDLE_SOURCE_CI_RUN_ID)"
