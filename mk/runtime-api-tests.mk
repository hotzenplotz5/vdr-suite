test-jobs-controller: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/JobRepository.cpp \
		api/rest/src/JobsController.cpp \
		api/rest/tests/test_jobs_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_jobs_controller
	$(BUILD_DIR)/test_jobs_controller

test-recordings-controller: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/RecordingRepository.cpp \
		api/rest/src/RecordingsController.cpp \
		api/rest/tests/test_recordings_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recordings_controller
	$(BUILD_DIR)/test_recordings_controller

test-metadata-controller: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/MetadataRepository.cpp \
		api/rest/src/MetadataController.cpp \
		api/rest/tests/test_metadata_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_metadata_controller
	$(BUILD_DIR)/test_metadata_controller

test-api-router: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(REST_ROUTER_SRC) \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStaleProbeAdministrationService.cpp \
		api/rest/src/EpgSearchNativeFuzzyStaleProbeAdministrationController.cpp \
		core/vdr/src/EpgSearchNativeFuzzyOperatorRefreshService.cpp \
		api/rest/src/EpgSearchNativeFuzzyOperatorRefreshController.cpp \
                api/rest/src/SearchTimerController.cpp \
                api/rest/src/SearchTimerDiscoveryController.cpp \
                core/vdr/src/SearchTimerResultJsonSerializer.cpp \
                core/vdr/src/SearchTimerService.cpp \
                api/rest/src/SearchTimerCreateRequestParser.cpp \
                api/rest/src/SearchTimerUpdateRequestParser.cpp \
                api/rest/src/SearchTimerDeleteRequestParser.cpp \
		api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp \
		api/rest/src/VdrController.cpp \
		api/rest/src/VdrRecordingQueryController.cpp \
		api/rest/src/VdrRecordingFolderController.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		api/rest/tests/test_api_router.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_api_router
	$(BUILD_DIR)/test_api_router

test-workflow-service: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(WORKFLOW_SRC) \
		core/recordings/tests/test_workflow_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_workflow_service
	$(BUILD_DIR)/test_workflow_service

test-worker-simulator: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(JOB_REPOSITORY_SRC) \
		$(WORKER_SRC) \
		core/recordings/tests/test_worker_simulator.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_worker_simulator
	$(BUILD_DIR)/test_worker_simulator

test-rectools-adapter:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECTOOLS_ADAPTER_SRC) \
		core/recordings/tests/test_rectools_adapter.cpp \
		-o $(BUILD_DIR)/test_rectools_adapter
	$(BUILD_DIR)/test_rectools_adapter

test-http-request:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/tests/test_http_request.cpp \
		-o $(BUILD_DIR)/test_http_request
	$(BUILD_DIR)/test_http_request

test-http-response:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/tests/test_http_response.cpp \
		-o $(BUILD_DIR)/test_http_response
	$(BUILD_DIR)/test_http_response

test-http-server-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/tests/test_http_server_contract.cpp \
		-o $(BUILD_DIR)/test_http_server_contract
	$(BUILD_DIR)/test_http_server_contract

test-test-http-server: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		$(BROWSER_SESSION_HTTP_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		$(RUNTIME_SRC) \
		$(REST_ROUTER_SRC) \
		api/rest/src/VdrRecordingFolderController.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStaleProbeAdministrationService.cpp \
		api/rest/src/EpgSearchNativeFuzzyStaleProbeAdministrationController.cpp \
		core/vdr/src/EpgSearchNativeFuzzyOperatorRefreshService.cpp \
		api/rest/src/EpgSearchNativeFuzzyOperatorRefreshController.cpp \
		api/rest/src/SearchTimerController.cpp \
		api/rest/src/SearchTimerDiscoveryController.cpp \
		core/vdr/src/SearchTimerResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerService.cpp \
                api/rest/src/SearchTimerCreateRequestParser.cpp \
                api/rest/src/SearchTimerUpdateRequestParser.cpp \
                api/rest/src/SearchTimerDeleteRequestParser.cpp \
		api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp \
		api/rest/src/VdrController.cpp \
		api/rest/src/VdrRecordingQueryController.cpp \
		core/http/src/TestHttpServer.cpp \
		core/http/tests/test_test_http_server.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_test_http_server
	$(BUILD_DIR)/test_test_http_server

test-mock-http-client:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/MockHttpClient.cpp \
		core/http/tests/test_mock_http_client.cpp \
		-o $(BUILD_DIR)/test_mock_http_client
	$(BUILD_DIR)/test_mock_http_client

daemon:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(AGENT_SRC) \
		$(DAEMON_SRC) \
		apps/daemon/main.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/vdr-suite-daemon


test-backend-runtime-context:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(AGENT_SRC) \
		$(VDR_RECORDING_NATIVE_METADATA_SRC) \
		core/daemon/src/RestfulApiEventStreamClient.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_backend_runtime_context.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_runtime_context
	$(BUILD_DIR)/test_backend_runtime_context

include mk/vdr-tests.mk

test-runtime-diagnostics:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/runtime/tests/test_runtime_diagnostics.cpp \
		-o $(BUILD_DIR)/test_runtime_diagnostics
	$(BUILD_DIR)/test_runtime_diagnostics

test-runtime-diagnostics-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/runtime/tests/test_runtime_diagnostics_service.cpp \
		-o $(BUILD_DIR)/test_runtime_diagnostics_service
	$(BUILD_DIR)/test_runtime_diagnostics_service

test-runtime-diagnostics-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RUNTIME_SRC) \
		core/runtime/tests/test_runtime_diagnostics_json_serializer.cpp \
		-o $(BUILD_DIR)/test_runtime_diagnostics_json_serializer
	$(BUILD_DIR)/test_runtime_diagnostics_json_serializer

test-runtime-diagnostics-summary-builder:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RUNTIME_SRC) \
		core/runtime/tests/test_runtime_diagnostics_summary_builder.cpp \
		-o $(BUILD_DIR)/test_runtime_diagnostics_summary_builder
	$(BUILD_DIR)/test_runtime_diagnostics_summary_builder

test: test-restfulapi-safe-trash-workflow test-restfulapi-safe-rename-workflow test-restfulapi-safe-move-workflow test-restfulapi-executor-basic-http-client-socket-smoke test-preview-execution-restfulapi-request-equivalence test-execution-service-restfulapi-executor-integration test-restfulapi-executor-http-result-mapping test-restfulapi-executor-http-transport-contract test-restfulapi-execution-dispatch-allowed test-restfulapi-execution-gate-blocks-dispatch test-restfulapi-upstream-action-endpoint-contract test-recording-action-preview-controller test-recording-action-request-preview-service-json-contract test-recording-action-request-preview-service test-restfulapi-move-tilde-mapping-regression test-recording-action-request-preview-result-json-serializer test-restfulapi-action-request-preview-contract test-real-client-readonly-recording-action-executor-gate test-restfulapi-recording-action-empty-basepath-contract test-basic-http-client-socket-contract test-restfulapi-recording-action-executor-transport-smoke test-recording-action-execution-controller-execute-body-policy-gate test-recording-action-execution-controller-policy-execute test-recording-action-policy-gated-execute test-recording-action-execution-controller-policy-safety test-recording-action-backend-policy-provider test-recording-action-backend-registry-policy-mapping test-recording-action-backend-policy-safety-json-contract test-recording-action-backend-policy-safety test-recording-action-backend-policy test-recording-action-permission-safety-json-contract test-recording-action-permission-safety-integration test-recording-action-permission-contract test-recording-action-safety-reason-contract test-recording-action-execution-controller-safety-preview test-recording-action-execution-service-registry-safety test-recording-action-backend-executor-registry-capabilities test-restfulapi-recording-action-executor-capabilities test-recording-action-execution-service-capability-safety test-recording-action-capability-safety-integration test-recording-action-capability-contract test-recording-action-safety-result-json-serializer test-recording-action-execution-service-safety test-recording-action-safety-contract test-restfulapi-recording-action-executor-response-contract test-restfulapi-recording-action-mapping-contract test-recording-action-execution-controller test-recording-action-backend-execution-path test-recording-action-execution-service test-recording-action-job-payload-factory test-recording-action-executor-interface test-recording-action-execution-result test-recording-action-execution-result-json-serializer test-recording-action-validation-request-parser test-recording-action-validation-controller test-recording-action-validation-service test-recording-action-validation-result-json-serializer test-vdr-recording-query-controller test-vdr-recording-query-result-json-serializer test-vdr-recording-query-matcher test-vdr-recording-query-service test-vdr-recording-query-result test-vdr-recording-query test-rest-query-parameters test-epgsearch-native-fuzzy-capability-detector test-epg-controller test-backend-registry-controller test-capability-report-service test-capability-controller test-capability-report-json-serializer test-capability-report-builder test-capability-report test-capability-state-json-serializer test-capability-state test-capability-resolver test-vdr-capability-set test-runtime-diagnostics test-runtime-diagnostics-service test-runtime-diagnostics-json-serializer test-runtime-diagnostics-summary-builder test-runtime-diagnostics-controller test-database test-recording-repository test-recording-service test-metadata-service test-recording-action test-action-service test-job-service test-job-repository test-job-dashboard-service test-recording-dashboard-service test-dashboard-facade test-dashboard-json-serializer test-dashboard-controller test-vdr-controller test-snapshot-change-feed-controller test-live-transport-controller test-jobs-controller test-recordings-controller test-metadata-controller test-api-router test-workflow-service test-worker-simulator test-rectools-adapter test-vdr-config test-external-vdr-adapter test-vdr-adapter-factory test-vdr-service test-vdr-overview-service test-vdr-overview-json-serializer test-vdr-snapshot-builder test-polling-service test-vdr-change-state test-vdr-change-event test-change-detection-service test-snapshot-refresh-decision-service test-snapshot-refresh-planner test-snapshot-update-plan test-snapshot-cache test-snapshot-cache-service test-snapshot-change-feed test-snapshot-change-feed-json-serializer test-live-update-event test-live-update-event-json-serializer test-live-transport-interface test-test-live-transport test-live-transport-service test-live-transport-factory test-sse-live-transport test-snapshot-access-service test-mock-vdr-adapter test-http-request test-http-response test-http-server-contract test-test-http-server test-mock-http-client test-restful-api-status-mapper test-restful-api-event-mapper test-restful-api-channel-mapper test-restful-api-recording-mapper test-restful-api-timer-mapper test-restful-api-vdr-adapter test-restful-api-change-state-adapter test-vdr-domain-objects

