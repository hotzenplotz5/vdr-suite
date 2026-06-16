include mk/common.mk

include mk/recording-sources.mk
include mk/action-job-sources.mk
include mk/rest-sources.mk
include mk/vdr-sources.mk
include mk/http-sources.mk
include mk/runtime-sources.mk
include mk/daemon-sources.mk
include mk/local-test-groups.mk


dashboard-cli: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(DASHBOARD_CLI_SRC) \
		apps/dashboard/main.cpp \
		$(LDFLAGS) \
		-o /tmp/vdr-suite-dashboard

test-database:
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/sqlite/tests/test_database.cpp \
		$(LDFLAGS) \
		-o /tmp/test_database
	/tmp/test_database

test-recording-repository: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/RecordingRepository.cpp \
		core/recordings/tests/test_recording_repository.cpp \
		$(LDFLAGS) \
		-o /tmp/test_recording_repository
	/tmp/test_recording_repository

test-recording-service: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(RECORDINGS_SRC) \
		core/recordings/tests/test_recording_service.cpp \
		$(LDFLAGS) \
		-o /tmp/test_recording_service
	/tmp/test_recording_service

test-metadata-service: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(METADATA_SRC) \
		core/recordings/tests/test_metadata_service.cpp \
		$(LDFLAGS) \
		-o /tmp/test_metadata_service
	/tmp/test_metadata_service

test-recording-action-execution-result-json-serializer:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_recording_action_execution_result_json_serializer.cpp \
		-o /tmp/test_recording_action_execution_result_json_serializer
	/tmp/test_recording_action_execution_result_json_serializer

test-recording-action-validation-request-parser:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		api/rest/tests/test_recording_action_validation_request_parser.cpp \
		-o /tmp/test_recording_action_validation_request_parser
	/tmp/test_recording_action_validation_request_parser

test-restfulapi-recording-action-executor-response-contract:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_restfulapi_recording_action_executor_response_contract.cpp \
		-o /tmp/test_restfulapi_recording_action_executor_response_contract
	/tmp/test_restfulapi_recording_action_executor_response_contract

test-restfulapi-recording-action-mapping-contract:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_restfulapi_recording_action_mapping_contract.cpp \
		-o /tmp/test_restfulapi_recording_action_mapping_contract
	/tmp/test_restfulapi_recording_action_mapping_contract

test-recording-action-execution-controller:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		api/rest/src/RecordingActionExecutionController.cpp \
		api/rest/tests/test_recording_action_execution_controller.cpp \
		-o /tmp/test_recording_action_execution_controller
	/tmp/test_recording_action_execution_controller

test-recording-action-validation-controller:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		api/rest/src/RecordingActionValidationController.cpp \
		api/rest/tests/test_recording_action_validation_controller.cpp \
		-o /tmp/test_recording_action_validation_controller
	/tmp/test_recording_action_validation_controller

test-recording-action-validation-service:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_recording_action_validation_service.cpp \
		-o /tmp/test_recording_action_validation_service
	/tmp/test_recording_action_validation_service

test-recording-action-validation-result-json-serializer:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_recording_action_validation_result_json_serializer.cpp \
		-o /tmp/test_recording_action_validation_result_json_serializer
	/tmp/test_recording_action_validation_result_json_serializer

test-recording-action:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_recording_action.cpp \
		-o /tmp/test_recording_action
	/tmp/test_recording_action

test-action-service:
	$(CXX) $(CXXFLAGS) \
		$(ACTION_SERVICE_SRC) \
		core/recordings/tests/test_action_service.cpp \
		-o /tmp/test_action_service
	/tmp/test_action_service

test-job-service:
	$(CXX) $(CXXFLAGS) \
		$(JOB_SERVICE_SRC) \
		core/recordings/tests/test_job_service.cpp \
		-o /tmp/test_job_service
	/tmp/test_job_service

test-job-repository: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(JOB_REPOSITORY_SRC) \
		core/recordings/tests/test_job_repository.cpp \
		$(LDFLAGS) \
		-o /tmp/test_job_repository
	/tmp/test_job_repository

test-job-dashboard-service: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(JOB_DASHBOARD_SRC) \
		core/recordings/tests/test_job_dashboard_service.cpp \
		$(LDFLAGS) \
		-o /tmp/test_job_dashboard_service
	/tmp/test_job_dashboard_service

test-recording-dashboard-service: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(RECORDING_DASHBOARD_SRC) \
		core/recordings/tests/test_recording_dashboard_service.cpp \
		$(LDFLAGS) \
		-o /tmp/test_recording_dashboard_service
	/tmp/test_recording_dashboard_service

test-dashboard-facade: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(DASHBOARD_FACADE_SRC) \
		core/recordings/tests/test_dashboard_facade.cpp \
		$(LDFLAGS) \
		-o /tmp/test_dashboard_facade
	/tmp/test_dashboard_facade

test-dashboard-json-serializer:
	$(CXX) $(CXXFLAGS) \
		$(DASHBOARD_JSON_SRC) \
		core/recordings/tests/test_dashboard_json_serializer.cpp \
		-o /tmp/test_dashboard_json_serializer
	/tmp/test_dashboard_json_serializer

test-dashboard-controller: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(REST_DASHBOARD_SRC) \
		api/rest/tests/test_dashboard_controller.cpp \
		$(LDFLAGS) \
		-o /tmp/test_dashboard_controller
	/tmp/test_dashboard_controller

test-vdr-controller:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(REST_VDR_SRC) \
		api/rest/tests/test_vdr_controller.cpp \
		-o /tmp/test_vdr_controller
	/tmp/test_vdr_controller

test-backend-registry-controller:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		api/rest/src/BackendRegistryController.cpp \
		api/rest/tests/test_backend_registry_controller.cpp \
		-o /tmp/test_backend_registry_controller
	/tmp/test_backend_registry_controller

test-runtime-diagnostics-controller:
	$(CXX) $(CXXFLAGS) \
		$(RUNTIME_SRC) \
		$(REST_RUNTIME_SRC) \
		api/rest/tests/test_runtime_diagnostics_controller.cpp \
		-o /tmp/test_runtime_diagnostics_controller
	/tmp/test_runtime_diagnostics_controller

test-snapshot-change-feed-controller:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(REST_SNAPSHOT_CHANGE_FEED_SRC) \
		api/rest/tests/test_snapshot_change_feed_controller.cpp \
		-o /tmp/test_snapshot_change_feed_controller
	/tmp/test_snapshot_change_feed_controller

test-live-transport-controller:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(REST_LIVE_TRANSPORT_SRC) \
		api/rest/tests/test_live_transport_controller.cpp \
		-o /tmp/test_live_transport_controller
	/tmp/test_live_transport_controller

test-epg-controller:
	$(CXX) $(CXXFLAGS) \
		api/rest/src/EpgController.cpp \
		api/rest/tests/test_epg_controller.cpp \
		-o /tmp/test_epg_controller
	/tmp/test_epg_controller

test-rest-query-parameters:
	$(CXX) $(CXXFLAGS) \
		api/rest/src/RestQueryParameters.cpp \
		api/rest/tests/test_rest_query_parameters.cpp \
		-o /tmp/test_rest_query_parameters
	/tmp/test_rest_query_parameters

test-jobs-controller: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/JobRepository.cpp \
		api/rest/src/JobsController.cpp \
		api/rest/tests/test_jobs_controller.cpp \
		$(LDFLAGS) \
		-o /tmp/test_jobs_controller
	/tmp/test_jobs_controller

test-recordings-controller: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/RecordingRepository.cpp \
		api/rest/src/RecordingsController.cpp \
		api/rest/tests/test_recordings_controller.cpp \
		$(LDFLAGS) \
		-o /tmp/test_recordings_controller
	/tmp/test_recordings_controller

test-metadata-controller: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/recordings/src/MetadataRepository.cpp \
		api/rest/src/MetadataController.cpp \
		api/rest/tests/test_metadata_controller.cpp \
		$(LDFLAGS) \
		-o /tmp/test_metadata_controller
	/tmp/test_metadata_controller

test-api-router: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(REST_ROUTER_SRC) \
		api/rest/src/VdrController.cpp \
		api/rest/src/VdrRecordingQueryController.cpp \
		api/rest/tests/test_api_router.cpp \
		$(LDFLAGS) \
		-o /tmp/test_api_router
	/tmp/test_api_router

test-workflow-service: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(WORKFLOW_SRC) \
		core/recordings/tests/test_workflow_service.cpp \
		$(LDFLAGS) \
		-o /tmp/test_workflow_service
	/tmp/test_workflow_service

test-worker-simulator: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(JOB_REPOSITORY_SRC) \
		$(WORKER_SRC) \
		core/recordings/tests/test_worker_simulator.cpp \
		$(LDFLAGS) \
		-o /tmp/test_worker_simulator
	/tmp/test_worker_simulator

test-rectools-adapter:
	$(CXX) $(CXXFLAGS) \
		$(RECTOOLS_ADAPTER_SRC) \
		core/recordings/tests/test_rectools_adapter.cpp \
		-o /tmp/test_rectools_adapter
	/tmp/test_rectools_adapter

test-http-request:
	$(CXX) $(CXXFLAGS) \
		core/http/tests/test_http_request.cpp \
		-o /tmp/test_http_request
	/tmp/test_http_request

test-http-response:
	$(CXX) $(CXXFLAGS) \
		core/http/tests/test_http_response.cpp \
		-o /tmp/test_http_response
	/tmp/test_http_response

test-http-server-contract:
	$(CXX) $(CXXFLAGS) \
		core/http/tests/test_http_server_contract.cpp \
		-o /tmp/test_http_server_contract
	/tmp/test_http_server_contract

test-test-http-server: prepare-test-db
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(REST_ROUTER_SRC) \
		api/rest/src/VdrController.cpp \
		api/rest/src/VdrRecordingQueryController.cpp \
		core/http/src/TestHttpServer.cpp \
		core/http/tests/test_test_http_server.cpp \
		$(LDFLAGS) \
		-o /tmp/test_test_http_server
	/tmp/test_test_http_server

test-mock-http-client:
	$(CXX) $(CXXFLAGS) \
		$(HTTP_SRC) \
		core/http/tests/test_mock_http_client.cpp \
		-o /tmp/test_mock_http_client
	/tmp/test_mock_http_client

daemon:
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		$(RUNTIME_SRC) \
		$(DAEMON_SRC) \
		apps/daemon/main.cpp \
		$(LDFLAGS) \
		-o /tmp/vdr-suite-daemon


test-backend-runtime-context:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_backend_runtime_context.cpp \
		-o /tmp/test_backend_runtime_context
	/tmp/test_backend_runtime_context

include mk/vdr-tests.mk

test-runtime-diagnostics:
	$(CXX) $(CXXFLAGS) \
		core/runtime/tests/test_runtime_diagnostics.cpp \
		-o /tmp/test_runtime_diagnostics
	/tmp/test_runtime_diagnostics

test-runtime-diagnostics-service:
	$(CXX) $(CXXFLAGS) \
		core/runtime/tests/test_runtime_diagnostics_service.cpp \
		-o /tmp/test_runtime_diagnostics_service
	/tmp/test_runtime_diagnostics_service

test-runtime-diagnostics-json-serializer:
	$(CXX) $(CXXFLAGS) \
		$(RUNTIME_SRC) \
		core/runtime/tests/test_runtime_diagnostics_json_serializer.cpp \
		-o /tmp/test_runtime_diagnostics_json_serializer
	/tmp/test_runtime_diagnostics_json_serializer

test-runtime-diagnostics-summary-builder:
	$(CXX) $(CXXFLAGS) \
		$(RUNTIME_SRC) \
		core/runtime/tests/test_runtime_diagnostics_summary_builder.cpp \
		-o /tmp/test_runtime_diagnostics_summary_builder
	/tmp/test_runtime_diagnostics_summary_builder

test: test-restfulapi-recording-action-executor-response-contract test-restfulapi-recording-action-mapping-contract test-recording-action-execution-controller test-recording-action-backend-execution-path test-recording-action-execution-service test-recording-action-job-payload-factory test-recording-action-executor-interface test-recording-action-execution-result test-recording-action-execution-result-json-serializer test-recording-action-validation-request-parser test-recording-action-validation-controller test-recording-action-validation-service test-recording-action-validation-result-json-serializer test-vdr-recording-query-controller test-vdr-recording-query-result-json-serializer test-vdr-recording-query-matcher test-vdr-recording-query-service test-vdr-recording-query-result test-vdr-recording-query test-rest-query-parameters test-epg-controller test-backend-registry-controller test-capability-report-service test-capability-controller test-capability-report-json-serializer test-capability-report-builder test-capability-report test-capability-state-json-serializer test-capability-state test-capability-resolver test-vdr-capability-set test-runtime-diagnostics test-runtime-diagnostics-service test-runtime-diagnostics-json-serializer test-runtime-diagnostics-summary-builder test-runtime-diagnostics-controller test-database test-recording-repository test-recording-service test-metadata-service test-recording-action test-action-service test-job-service test-job-repository test-job-dashboard-service test-recording-dashboard-service test-dashboard-facade test-dashboard-json-serializer test-dashboard-controller test-vdr-controller test-snapshot-change-feed-controller test-live-transport-controller test-jobs-controller test-recordings-controller test-metadata-controller test-api-router test-workflow-service test-worker-simulator test-rectools-adapter test-vdr-config test-external-vdr-adapter test-vdr-adapter-factory test-vdr-service test-vdr-overview-service test-vdr-overview-json-serializer test-vdr-snapshot-builder test-polling-service test-vdr-change-state test-vdr-change-event test-change-detection-service test-snapshot-refresh-decision-service test-snapshot-refresh-planner test-snapshot-update-plan test-snapshot-cache test-snapshot-cache-service test-snapshot-change-feed test-snapshot-change-feed-json-serializer test-live-update-event test-live-update-event-json-serializer test-live-transport-interface test-test-live-transport test-live-transport-service test-live-transport-factory test-sse-live-transport test-snapshot-access-service test-mock-vdr-adapter test-http-request test-http-response test-http-server-contract test-test-http-server test-mock-http-client test-restful-api-status-mapper test-restful-api-event-mapper test-restful-api-channel-mapper test-restful-api-recording-mapper test-restful-api-timer-mapper test-restful-api-vdr-adapter test-restful-api-change-state-adapter test-vdr-domain-objects

clean:
	rm -f /tmp/test_database
	rm -f /tmp/test_recording_repository
	rm -f /tmp/test_recording_service
	rm -f /tmp/test_metadata_service
	rm -f /tmp/test_recording_action
	rm -f /tmp/test_action_service
	rm -f /tmp/test_job_service
	rm -f /tmp/test_job_repository
	rm -f /tmp/test_job_dashboard_service
	rm -f /tmp/test_recording_dashboard_service
	rm -f /tmp/test_dashboard_facade
	rm -f /tmp/test_dashboard_json_serializer
	rm -f /tmp/test_dashboard_controller
	rm -f /tmp/test_jobs_controller
	rm -f /tmp/test_recordings_controller
	rm -f /tmp/test_api_router
	rm -f /tmp/test_workflow_service
	rm -f /tmp/test_worker_simulator
	rm -f /tmp/test_rectools_adapter
	rm -f /tmp/vdr-suite-dashboard
	rm -f /tmp/vdr-suite-test.db
	rm -f /tmp/test_metadata_controller
	rm -f /tmp/test_vdr_config
	rm -f /tmp/test_external_vdr_adapter
	rm -f /tmp/test_vdr_adapter_factory
	rm -f /tmp/test_mock_vdr_adapter
	rm -f /tmp/test_http_request
	rm -f /tmp/test_http_response
	rm -f /tmp/test_mock_http_client
	rm -f /tmp/test_restful_api_status_mapper
	rm -f /tmp/test_restful_api_channel_mapper
	rm -f /tmp/test_restful_api_recording_mapper
	rm -f /tmp/test_restful_api_vdr_adapter
	rm -f /tmp/test_restful_api_change_state_adapter
	rm -f /tmp/test_vdr_domain_objects
	rm -f /tmp/test_vdr_recording_query
	rm -f /tmp/test_vdr_recording_query_result
	rm -f /tmp/test_vdr_recording_query_service
	rm -f /tmp/test_vdr_recording_query_matcher
	rm -f /tmp/test_vdr_recording_query_result_json_serializer
	rm -f /tmp/test_vdr_recording_query_controller
	rm -f /tmp/test_vdr_service
	rm -f /tmp/test_vdr_overview_service
	rm -f /tmp/test_vdr_overview_json_serializer
	rm -f /tmp/test_vdr_snapshot_builder
	rm -f /tmp/test_polling_service
	rm -f /tmp/test_vdr_change_state
	rm -f /tmp/test_vdr_change_event
	rm -f /tmp/test_change_detection_service
	rm -f /tmp/test_snapshot_refresh_decision_service
	rm -f /tmp/test_snapshot_cache
	rm -f /tmp/test_snapshot_cache_service
	rm -f /tmp/test_snapshot_change_feed
	rm -f /tmp/test_snapshot_change_feed_json_serializer
	rm -f /tmp/test_live_update_event
	rm -f /tmp/test_live_update_event_json_serializer
	rm -f /tmp/test_live_transport_interface
	rm -f /tmp/test_test_live_transport
	rm -f /tmp/test_live_transport_service
	rm -f /tmp/test_live_transport_factory
	rm -f /tmp/test_sse_live_transport
	rm -f /tmp/test_snapshot_change_feed_controller
	rm -f /tmp/test_live_transport_controller
	rm -f /tmp/test_epg_controller
	rm -f /tmp/test_rest_query_parameters
	rm -f /tmp/test_snapshot_access_service
	rm -f /tmp/test_http_server_contract
	rm -f /tmp/test_test_http_server
	rm -f /tmp/test_runtime_diagnostics
	rm -f /tmp/test_runtime_diagnostics_service
	rm -f /tmp/test_runtime_diagnostics_json_serializer
	rm -f /tmp/test_runtime_diagnostics_controller
	rm -f /tmp/test_backend_registry_controller
	rm -f /tmp/test_capability_state
	rm -f /tmp/test_capability_state_json_serializer
	rm -f /tmp/test_capability_report
	rm -f /tmp/test_capability_report_builder
	rm -f /tmp/test_capability_report_json_serializer
	rm -f /tmp/test_capability_controller
	rm -f /tmp/test_capability_report_service

.PHONY: test-recording-action-executor-interface
test-recording-action-backend-execution-path:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_recording_action_backend_execution_path.cpp \
		-o /tmp/test_recording_action_backend_execution_path
	/tmp/test_recording_action_backend_execution_path

test-recording-action-execution-service:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_recording_action_execution_service.cpp \
		-o /tmp/test_recording_action_execution_service
	/tmp/test_recording_action_execution_service

test-recording-action-job-payload-factory:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_recording_action_job_payload_factory.cpp \
		-o /tmp/test_recording_action_job_payload_factory
	/tmp/test_recording_action_job_payload_factory

test-recording-action-executor-interface:
	$(CXX) $(CXXFLAGS) \
		$(ACTIONS_SRC) \
		core/recordings/tests/test_recording_action_executor_interface.cpp \
		-o /tmp/test_recording_action_executor_interface
	/tmp/test_recording_action_executor_interface

.PHONY: test-recording-action-execution-result
test-recording-action-execution-result:
	g++ -std=c++17 -Wall -Wextra -Icore/recordings/include \
		core/recordings/tests/test_recording_action_execution_result.cpp \
		-o /tmp/test_recording_action_execution_result
	/tmp/test_recording_action_execution_result

.PHONY: test-docs
test-docs:
	python3 tools/check_docs.py
	python3 tools/check_doc_indexes.py
	python3 tools/check_doc_reachability.py

.PHONY: test-architecture
test-architecture:
	python3 tools/check_architecture.py

.PHONY: test-phase
test-phase:
	python3 tools/check_phase_consistency.py


.PHONY: test-capability-report-service
test-capability-report-service:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/CapabilityReportService.cpp \
		core/vdr/tests/test_capability_report_service.cpp \
		-o /tmp/test_capability_report_service
	/tmp/test_capability_report_service

.PHONY: test-capability-controller
test-capability-controller:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include -Iapi/rest/include \
		core/vdr/src/CapabilityStateJsonSerializer.cpp \
		core/vdr/src/CapabilityReportJsonSerializer.cpp \
		core/vdr/src/CapabilityReportService.cpp \
		api/rest/src/CapabilityController.cpp \
		api/rest/tests/test_capability_controller.cpp \
		-o /tmp/test_capability_controller
	/tmp/test_capability_controller







.PHONY: test-vdr-recording-query-controller
test-vdr-recording-query-controller:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include -Icore/runtime/include -Iapi/rest/include \
		core/vdr/src/VdrService.cpp \
		core/vdr/src/VdrChangeState.cpp \
		core/vdr/src/MockVdrAdapter.cpp \
		core/vdr/src/VdrRecordingQueryMatcher.cpp \
		core/vdr/src/VdrRecordingQueryService.cpp \
		core/vdr/src/VdrRecordingQueryResultJsonSerializer.cpp \
		api/rest/src/VdrRecordingQueryController.cpp \
		api/rest/tests/test_vdr_recording_query_controller.cpp \
		-o /tmp/test_vdr_recording_query_controller
	/tmp/test_vdr_recording_query_controller

.PHONY: test-vdr-recording-query-result-json-serializer
test-vdr-recording-query-result-json-serializer:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/VdrRecordingQueryResultJsonSerializer.cpp \
		core/vdr/tests/test_vdr_recording_query_result_json_serializer.cpp \
		-o /tmp/test_vdr_recording_query_result_json_serializer
	/tmp/test_vdr_recording_query_result_json_serializer

.PHONY: test-vdr-recording-query-matcher
test-vdr-recording-query-matcher:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/VdrRecordingQueryMatcher.cpp \
		core/vdr/tests/test_vdr_recording_query_matcher.cpp \
		-o /tmp/test_vdr_recording_query_matcher
	/tmp/test_vdr_recording_query_matcher

.PHONY: test-vdr-recording-query-service
test-vdr-recording-query-service:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include -Icore/runtime/include \
		core/vdr/src/VdrService.cpp \
		core/vdr/src/VdrChangeState.cpp \
		core/vdr/src/MockVdrAdapter.cpp \
		core/vdr/src/VdrRecordingQueryMatcher.cpp \
		core/vdr/src/VdrRecordingQueryService.cpp \
		core/vdr/tests/test_vdr_recording_query_service.cpp \
		-o /tmp/test_vdr_recording_query_service
	/tmp/test_vdr_recording_query_service

.PHONY: test-vdr-recording-query-result
test-vdr-recording-query-result:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_vdr_recording_query_result.cpp \
		-o /tmp/test_vdr_recording_query_result
	/tmp/test_vdr_recording_query_result

.PHONY: test-vdr-recording-query
test-vdr-recording-query:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_vdr_recording_query.cpp \
		-o /tmp/test_vdr_recording_query
	/tmp/test_vdr_recording_query

.PHONY: test-capability-report-json-serializer
test-capability-report-json-serializer:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/CapabilityStateJsonSerializer.cpp \
		core/vdr/src/CapabilityReportJsonSerializer.cpp \
		core/vdr/tests/test_capability_report_json_serializer.cpp \
		-o /tmp/test_capability_report_json_serializer
	/tmp/test_capability_report_json_serializer

.PHONY: test-capability-report-builder
test-capability-report-builder:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_capability_report_builder.cpp \
		-o /tmp/test_capability_report_builder
	/tmp/test_capability_report_builder

.PHONY: test-capability-report
test-capability-report:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_capability_report.cpp \
		-o /tmp/test_capability_report
	/tmp/test_capability_report

.PHONY: test-capability-state-json-serializer
test-capability-state-json-serializer:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/CapabilityStateJsonSerializer.cpp \
		core/vdr/tests/test_capability_state_json_serializer.cpp \
		-o /tmp/test_capability_state_json_serializer
	/tmp/test_capability_state_json_serializer

.PHONY: test-capability-state
test-capability-state:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_capability_state.cpp \
		-o /tmp/test_capability_state
	/tmp/test_capability_state

.PHONY: test-capability-resolver
test-capability-resolver:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_capability_resolver.cpp \
		-o /tmp/test_capability_resolver
	/tmp/test_capability_resolver

.PHONY: test-vdr-capability-set
test-vdr-capability-set:
	g++ -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_vdr_capability_set.cpp \
		-o /tmp/test_vdr_capability_set
	/tmp/test_vdr_capability_set
