.PHONY: \
	test-phase62-protected-mutation-outcome-harness \
	phase62-protected-mutation-outcome-runtime-acceptance

PHASE62_PROTECTED_MUTATION_OUTCOME_RUNNER := \
	tools/phase62-runtime-acceptance/protected-mutation-outcome-runner.py

PHASE62_PROTECTED_MUTATION_OUTCOME_REPORT ?= \
	$(BUILD_DIR)/phase62-protected-mutation-outcome-runtime-acceptance.json


test-phase62-protected-mutation-outcome-harness:
	mkdir -p "$(BUILD_DIR)/python-cache"
	PYTHONPYCACHEPREFIX="$(BUILD_DIR)/python-cache" \
		python3 -m py_compile \
		"$(PHASE62_PROTECTED_MUTATION_OUTCOME_RUNNER)"
	python3 "$(PHASE62_PROTECTED_MUTATION_OUTCOME_RUNNER)" \
		--validate-only
	python3 "$(PHASE62_PROTECTED_MUTATION_OUTCOME_RUNNER)" \
		--self-test


test-phase62-runtime-acceptance-harness: \
	test-phase62-protected-mutation-outcome-harness


phase62-protected-mutation-outcome-runtime-acceptance: \
	test-phase62-protected-mutation-outcome-harness
	@test -n "$(PHASE62_ACCEPTANCE_BACKUP_DIR)" || \
		{ echo "PHASE62_ACCEPTANCE_BACKUP_DIR is required"; exit 2; }
	@test -n "$(PHASE62_EXPECTED_BRANCH)" || \
		{ echo "PHASE62_EXPECTED_BRANCH is required"; exit 2; }
	@test -n "$(PHASE62_EXPECTED_HEAD)" || \
		{ echo "PHASE62_EXPECTED_HEAD is required"; exit 2; }
	@test -n "$(PHASE62_EXPECTED_DAEMON_SHA256)" || \
		{ echo "PHASE62_EXPECTED_DAEMON_SHA256 is required"; exit 2; }
	@test -n "$(PHASE62_EXPECTED_LOADER_SHA256)" || \
		{ echo "PHASE62_EXPECTED_LOADER_SHA256 is required"; exit 2; }
	mkdir -p "$(BUILD_DIR)"
	python3 "$(PHASE62_PROTECTED_MUTATION_OUTCOME_RUNNER)" \
		--run \
		--base-url "$(PHASE62_ACCEPTANCE_BASE_URL)" \
		--database "$(PHASE62_ACCEPTANCE_DATABASE)" \
		--service "$(PHASE62_ACCEPTANCE_SERVICE)" \
		--daemon "$(PHASE62_ACCEPTANCE_DAEMON)" \
		--loader "$(PHASE62_ACCEPTANCE_LOADER)" \
		--backup-dir "$(PHASE62_ACCEPTANCE_BACKUP_DIR)" \
		--expected-branch "$(PHASE62_EXPECTED_BRANCH)" \
		--expected-head "$(PHASE62_EXPECTED_HEAD)" \
		--expected-daemon-sha256 \
			"$(PHASE62_EXPECTED_DAEMON_SHA256)" \
		--expected-loader-sha256 \
			"$(PHASE62_EXPECTED_LOADER_SHA256)" \
		--report-json \
			"$(PHASE62_PROTECTED_MUTATION_OUTCOME_REPORT)"
