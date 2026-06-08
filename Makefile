include mk/common.mk

include mk/recording-sources.mk
include mk/action-job-sources.mk
include mk/rest-sources.mk
include mk/vdr-sources.mk
include mk/http-sources.mk
include mk/runtime-sources.mk
include mk/daemon-sources.mk


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

test: test-runtime-diagnostics test-runtime-diagnostics-service test-runtime-diagnostics-json-serializer test-runtime-diagnostics-controller test-database test-recording-repository test-recording-service test-metadata-service test-recording-action test-action-service test-job-service test-job-repository test-job-dashboard-service test-recording-dashboard-service test-dashboard-facade test-dashboard-json-serializer test-dashboard-controller test-vdr-controller test-snapshot-change-feed-controller test-jobs-controller test-recordings-controller test-metadata-controller test-api-router test-workflow-service test-worker-simulator test-rectools-adapter test-vdr-config test-external-vdr-adapter test-vdr-adapter-factory test-vdr-service test-vdr-overview-service test-vdr-overview-json-serializer test-vdr-snapshot-builder test-polling-service test-vdr-change-state test-vdr-change-event test-change-detection-service test-snapshot-refresh-decision-service test-snapshot-refresh-planner test-snapshot-update-plan test-snapshot-cache test-snapshot-cache-service test-snapshot-change-feed test-snapshot-change-feed-json-serializer test-snapshot-access-service test-mock-vdr-adapter test-http-request test-http-response test-http-server-contract test-test-http-server test-mock-http-client test-restful-api-status-mapper test-restful-api-event-mapper test-restful-api-channel-mapper test-restful-api-recording-mapper test-restful-api-timer-mapper test-restful-api-vdr-adapter test-restful-api-change-state-adapter test-vdr-domain-objects

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
	rm -f /tmp/test_snapshot_change_feed_controller
	rm -f /tmp/test_snapshot_access_service
	rm -f /tmp/test_http_server_contract
	rm -f /tmp/test_test_http_server
	rm -f /tmp/test_runtime_diagnostics
	rm -f /tmp/test_runtime_diagnostics_service
	rm -f /tmp/test_runtime_diagnostics_json_serializer
	rm -f /tmp/test_runtime_diagnostics_controller

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
