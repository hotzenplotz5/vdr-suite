.PHONY: \
	test-phase62-runtime-acceptance-harness \
	phase62-runtime-acceptance

PHASE62_ACCEPTANCE_RUNNER := \
	tools/phase62-runtime-acceptance/runner.py

PHASE62_ACCEPTANCE_MANIFEST ?= \
	tools/phase62-runtime-acceptance/slice-2j.json

PHASE62_ACCEPTANCE_BASE_URL ?= \
	http://127.0.0.1:18080

PHASE62_ACCEPTANCE_DATABASE ?= \
	/var/lib/vdr-suite/vdr-suite.db

PHASE62_ACCEPTANCE_SERVICE ?= \
	vdr-suite-daemon.service

PHASE62_ACCEPTANCE_DAEMON ?= \
	/usr/sbin/vdr-suite-daemon

PHASE62_ACCEPTANCE_LOADER ?= \
	/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js

PHASE62_ACCEPTANCE_REPORT ?= \
	$(BUILD_DIR)/phase62-runtime-acceptance.json


test-phase62-runtime-acceptance-harness:
	mkdir -p "$(BUILD_DIR)/python-cache"
	PYTHONPYCACHEPREFIX="$(BUILD_DIR)/python-cache" \
		python3 -m py_compile \
		"$(PHASE62_ACCEPTANCE_RUNNER)"
	python3 "$(PHASE62_ACCEPTANCE_RUNNER)" \
		--manifest "$(PHASE62_ACCEPTANCE_MANIFEST)" \
		--validate-only
	python3 "$(PHASE62_ACCEPTANCE_RUNNER)" \
		--manifest "$(PHASE62_ACCEPTANCE_MANIFEST)" \
		--self-test


phase62-runtime-acceptance: \
	test-phase62-runtime-acceptance-harness
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
	python3 "$(PHASE62_ACCEPTANCE_RUNNER)" \
		--manifest "$(PHASE62_ACCEPTANCE_MANIFEST)" \
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
		--report-json "$(PHASE62_ACCEPTANCE_REPORT)"
