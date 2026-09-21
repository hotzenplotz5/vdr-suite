audit-doc-sync:
	python3 tools/audit_recent_doc_sync.py --commits 20

audit-doc-sync-tests:
	python3 tools/audit_recent_doc_sync.py --commits 20 --run-tests



searchtimer-yavdr-api-smoke-harness-helper:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/SearchTimerResultJsonSerializer.cpp \
		api/rest/src/SearchTimerCreateRequestParser.cpp \
		api/rest/src/SearchTimerUpdateRequestParser.cpp \
		api/rest/src/SearchTimerDeleteRequestParser.cpp \
		api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp \
		api/rest/src/SearchTimerController.cpp \
		core/http/src/SimpleHttpListener.cpp \
		apps/tools/searchtimer_yavdr_api_smoke_harness.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr_suite_searchtimer_yavdr_api_smoke_harness
	$(BUILD_DIR)/vdr_suite_searchtimer_yavdr_api_smoke_harness --help

searchtimer-yavdr-api-smoke-harness-run: searchtimer-yavdr-api-smoke-harness-helper
	rm -f $(BUILD_DIR)/vdr_suite_searchtimer_yavdr_api_smoke_harness.log
	$(BUILD_DIR)/vdr_suite_searchtimer_yavdr_api_smoke_harness --host 127.0.0.1 --port 18080 > $(BUILD_DIR)/vdr_suite_searchtimer_yavdr_api_smoke_harness.log 2>&1 & \
		harness_pid=$$!; \
		sleep 1; \
		python3 tools/run_searchtimer_yavdr_real_test.py --run --base-url http://127.0.0.1:18080 --backend home-vdr --print-json; \
		smoke_status=$$?; \
		kill $$harness_pid >/dev/null 2>&1 || true; \
		wait $$harness_pid >/dev/null 2>&1 || true; \
		cat $(BUILD_DIR)/vdr_suite_searchtimer_yavdr_api_smoke_harness.log; \
		exit $$smoke_status

searchtimer-yavdr-real-test-smoke-helper:
	python3 -m py_compile tools/run_searchtimer_yavdr_real_test.py
	python3 tools/run_searchtimer_yavdr_real_test.py --help
	python3 tools/run_searchtimer_yavdr_real_test.py --self-test

vdr-suite-native-fuzzy-validation-helpers:
	python3 tools/validate_vdr_suite_native_fuzzy_operator_refresh.py --help
	python3 tools/validate_vdr_suite_native_fuzzy_capability_report.py --help
	python3 tools/validate_vdr_suite_native_fuzzy_persisted_restore.py --help

vdr-suite-native-fuzzy-persisted-restore-helper:
	python3 tools/validate_vdr_suite_native_fuzzy_persisted_restore.py --help

vdr-suite-native-fuzzy-capability-report-helper:
	python3 tools/validate_vdr_suite_native_fuzzy_capability_report.py --help

vdr-suite-native-fuzzy-operator-refresh-helper:
	python3 tools/validate_vdr_suite_native_fuzzy_operator_refresh.py --help

restfulapi-real-delete-smoke-helper:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/BasicHttpClient.cpp \
		apps/tools/restfulapi_recording_action_real_delete_smoke.cpp \
		-o $(BUILD_DIR)/restfulapi_recording_action_real_delete_smoke
	$(BUILD_DIR)/restfulapi_recording_action_real_delete_smoke --help

restfulapi-real-move-smoke-helper:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/BasicHttpClient.cpp \
		apps/tools/restfulapi_recording_action_real_move_smoke.cpp \
		-o $(BUILD_DIR)/restfulapi_recording_action_real_move_smoke
	$(BUILD_DIR)/restfulapi_recording_action_real_move_smoke --help

restfulapi-connectivity-smoke:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/BasicHttpClient.cpp \
		apps/tools/restfulapi_connectivity_smoke.cpp \
		-o $(BUILD_DIR)/restfulapi_connectivity_smoke
	$(BUILD_DIR)/restfulapi_connectivity_smoke

searchtimer-real-vdr-smoke-helper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/BasicHttpClient.cpp \
		core/vdr/src/RestfulApiSearchTimerCommandExecutor.cpp \
		apps/tools/searchtimer_real_vdr_smoke.cpp \
		-o $(BUILD_DIR)/vdr_suite_searchtimer_real_smoke
	$(BUILD_DIR)/vdr_suite_searchtimer_real_smoke --help

vdr-timer-real-lifecycle-smoke-helper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/BasicHttpClient.cpp \
		core/vdr/src/RestfulApiTimerMapper.cpp \
		core/vdr/src/RestfulApiVdrTimerActionExecutor.cpp \
		apps/tools/vdr_timer_real_lifecycle_smoke.cpp \
		-o $(BUILD_DIR)/vdr_suite_timer_lifecycle_smoke
	$(BUILD_DIR)/vdr_suite_timer_lifecycle_smoke --help

real-vdr-readonly-regression-helper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/BasicHttpClient.cpp \
		apps/tools/vdr_real_readonly_regression.cpp \
		-o $(BUILD_DIR)/vdr_suite_real_readonly_regression
	$(BUILD_DIR)/vdr_suite_real_readonly_regression --help

real-vdr-regression: real-vdr-readonly-regression-helper searchtimer-real-vdr-smoke-helper vdr-timer-real-lifecycle-smoke-helper
	@if [ -z "$$VDR_SUITE_TIMER_CHANNEL" ]; then \
		echo "VDR_SUITE_TIMER_CHANNEL must be set to a real VDR channel id for timer lifecycle regression."; \
		echo "Example: VDR_SUITE_TIMER_CHANNEL='C-1-1051-10301' make real-vdr-regression"; \
		exit 2; \
	fi
	$(BUILD_DIR)/vdr_suite_real_readonly_regression --run
	$(BUILD_DIR)/vdr_suite_searchtimer_real_smoke --run
	$(BUILD_DIR)/vdr_suite_timer_lifecycle_smoke --run

dashboard-cli: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(DASHBOARD_CLI_SRC) \
		apps/dashboard/main.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr-suite-dashboard

test-database:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/sqlite/tests/test_database.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_database
	$(BUILD_DIR)/test_database

test-recording-repository: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/RecordingRepository.cpp \
		core/recordings/tests/test_recording_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recording_repository
	$(BUILD_DIR)/test_recording_repository

test-recording-service: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(RECORDINGS_SRC) \
		core/recordings/tests/test_recording_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recording_service
	$(BUILD_DIR)/test_recording_service

test-metadata-service: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_SRC) \
		core/recordings/tests/test_metadata_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_metadata_service
	$(BUILD_DIR)/test_metadata_service

