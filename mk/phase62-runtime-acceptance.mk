.PHONY: \
	test-phase62-runtime-acceptance-harness \
	phase62-runtime-acceptance \
	phase62-runtime-acceptance-static-body \
	phase62-runtime-acceptance-query-cache-batch \
	phase62-runtime-acceptance-global-stale-probe-delete \
	phase62-runtime-acceptance-batch

PHASE62_ACCEPTANCE_RUNNER := \
	tools/phase62-runtime-acceptance/runner.py

PHASE62_STATIC_BODY_RUNNER := \
	tools/phase62-runtime-acceptance/static-body-runner.py

PHASE62_SAFE_POST_RUNNER := \
	tools/phase62-runtime-acceptance/safe-post-runner.py

PHASE62_GLOBAL_STALE_PROBE_DELETE_RUNNER := \
	tools/phase62-runtime-acceptance/global-stale-probe-delete-runner.py

PHASE62_ACCEPTANCE_MANIFEST ?= \
	tools/phase62-runtime-acceptance/slice-2j.json

PHASE62_ACCEPTANCE_MANIFESTS := \
	tools/phase62-runtime-acceptance/slice-2j.json \
	tools/phase62-runtime-acceptance/slice-2l-searchtimer-update.json \
	tools/phase62-runtime-acceptance/slice-2l-searchtimer-delete.json \
	tools/phase62-runtime-acceptance/slice-2n-searchtimer-execution.json

PHASE62_STATIC_BODY_MANIFEST ?= \
	tools/phase62-runtime-acceptance/slice-2o-native-fuzzy-refresh.json

PHASE62_QUERY_CACHE_MANIFESTS := \
	tools/phase62-runtime-acceptance/slice-2p-searchtimer-preview-cache-refresh.json \
	tools/phase62-runtime-acceptance/slice-2p-epg-cache-refresh.json

PHASE62_STATIC_BODY_MANIFESTS := \
	tools/phase62-runtime-acceptance/slice-2o-native-fuzzy-refresh.json \
	$(PHASE62_QUERY_CACHE_MANIFESTS)

PHASE62_ACCEPTANCE_BATCH_MANIFESTS := \
	tools/phase62-runtime-acceptance/slice-2l-searchtimer-update.json \
	tools/phase62-runtime-acceptance/slice-2l-searchtimer-delete.json

PHASE62_SAFE_POST_MANIFEST := \
	tools/phase62-runtime-acceptance/slice-2m-safe-post.json

PHASE62_GLOBAL_STALE_PROBE_DELETE_MANIFEST := \
	tools/phase62-runtime-acceptance/slice-2q-native-fuzzy-stale-probe-delete.json

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

PHASE62_ACCEPTANCE_BATCH_REPORT_DIR ?= \
	$(BUILD_DIR)/phase62-runtime-acceptance-batch

PHASE62_QUERY_CACHE_REPORT_DIR ?= \
	$(BUILD_DIR)/phase62-runtime-acceptance-slice2p

PHASE62_GLOBAL_STALE_PROBE_DELETE_REPORT ?= \
	$(BUILD_DIR)/phase62-runtime-acceptance-slice2q.json


test-phase62-runtime-acceptance-harness:
	mkdir -p "$(BUILD_DIR)/python-cache"
	PYTHONPYCACHEPREFIX="$(BUILD_DIR)/python-cache" \
		python3 -m py_compile \
		"$(PHASE62_ACCEPTANCE_RUNNER)" \
		"$(PHASE62_STATIC_BODY_RUNNER)" \
		"$(PHASE62_SAFE_POST_RUNNER)" \
		"$(PHASE62_GLOBAL_STALE_PROBE_DELETE_RUNNER)"
	@set -e; \
	for manifest in $(PHASE62_ACCEPTANCE_MANIFESTS); do \
		python3 "$(PHASE62_ACCEPTANCE_RUNNER)" \
			--manifest "$$manifest" \
			--validate-only; \
		python3 "$(PHASE62_ACCEPTANCE_RUNNER)" \
			--manifest "$$manifest" \
			--self-test; \
	done
	@set -e; \
	for manifest in $(PHASE62_STATIC_BODY_MANIFESTS); do \
		python3 "$(PHASE62_STATIC_BODY_RUNNER)" \
			--manifest "$$manifest" \
			--validate-only; \
		python3 "$(PHASE62_STATIC_BODY_RUNNER)" \
			--manifest "$$manifest" \
			--self-test; \
	done
	python3 "$(PHASE62_SAFE_POST_RUNNER)" \
		--manifest "$(PHASE62_SAFE_POST_MANIFEST)" \
		--validate-only
	python3 "$(PHASE62_SAFE_POST_RUNNER)" \
		--manifest "$(PHASE62_SAFE_POST_MANIFEST)" \
		--self-test
	python3 "$(PHASE62_GLOBAL_STALE_PROBE_DELETE_RUNNER)" \
		--manifest "$(PHASE62_GLOBAL_STALE_PROBE_DELETE_MANIFEST)" \
		--validate-only
	python3 "$(PHASE62_GLOBAL_STALE_PROBE_DELETE_RUNNER)" \
		--manifest "$(PHASE62_GLOBAL_STALE_PROBE_DELETE_MANIFEST)" \
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


phase62-runtime-acceptance-static-body: \
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
	python3 "$(PHASE62_STATIC_BODY_RUNNER)" \
		--manifest "$(PHASE62_STATIC_BODY_MANIFEST)" \
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


phase62-runtime-acceptance-query-cache-batch: \
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
	mkdir -p "$(PHASE62_QUERY_CACHE_REPORT_DIR)"
	@set -e; \
	for manifest in $(PHASE62_QUERY_CACHE_MANIFESTS); do \
		name="$${manifest##*/}"; \
		name="$${name%.json}"; \
		python3 "$(PHASE62_STATIC_BODY_RUNNER)" \
			--manifest "$$manifest" \
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
				"$(PHASE62_QUERY_CACHE_REPORT_DIR)/$$name.json"; \
	done


phase62-runtime-acceptance-global-stale-probe-delete: \
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
	python3 "$(PHASE62_GLOBAL_STALE_PROBE_DELETE_RUNNER)" \
		--manifest "$(PHASE62_GLOBAL_STALE_PROBE_DELETE_MANIFEST)" \
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
			"$(PHASE62_GLOBAL_STALE_PROBE_DELETE_REPORT)"


phase62-runtime-acceptance-batch: \
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
	mkdir -p "$(PHASE62_ACCEPTANCE_BATCH_REPORT_DIR)"
	@set -e; \
	for manifest in $(PHASE62_ACCEPTANCE_BATCH_MANIFESTS); do \
		name="$${manifest##*/}"; \
		name="$${name%.json}"; \
		python3 "$(PHASE62_ACCEPTANCE_RUNNER)" \
			--manifest "$$manifest" \
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
				"$(PHASE62_ACCEPTANCE_BATCH_REPORT_DIR)/$$name.json"; \
	done
	python3 "$(PHASE62_SAFE_POST_RUNNER)" \
		--manifest "$(PHASE62_SAFE_POST_MANIFEST)" \
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
			"$(PHASE62_ACCEPTANCE_BATCH_REPORT_DIR)/slice-2m-safe-post.json"
