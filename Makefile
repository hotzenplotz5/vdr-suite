CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra \
        -Icore/sqlite/include \
        -Icore/recordings/include \
        -Icore/daemon/include \
        -Icore/vdr/include \
	-Icore/http/include \
        -Iapi/rest/include
        
LDFLAGS := $(shell pkg-config --libs sqlite3)

SQLITE_SRC := core/sqlite/src/Database.cpp

VDR_SRC := \
        core/vdr/src/VdrConfig.cpp \
        core/vdr/src/ExternalVdrAdapter.cpp \
        core/vdr/src/MockVdrAdapter.cpp \
        core/vdr/src/VdrService.cpp \
        core/vdr/src/RestfulApiChannelMapper.cpp \
        core/vdr/src/RestfulApiRecordingMapper.cpp \
        core/vdr/src/RestfulApiTimerMapper.cpp \
        core/vdr/src/RestfulApiEventMapper.cpp \
        core/vdr/src/RestfulApiStatusMapper.cpp \
        core/vdr/src/RestfulApiVdrAdapter.cpp \
        core/vdr/src/VdrOverviewService.cpp \
        core/vdr/src/VdrOverviewJsonSerializer.cpp \
        core/vdr/src/VdrAdapterFactory.cpp


HTTP_SRC := \
        core/http/src/MockHttpClient.cpp

RECORDINGS_SRC := \
	core/recordings/src/RecordingRepository.cpp \
	core/recordings/src/MetadataRepository.cpp \
	core/recordings/src/RecordingService.cpp

METADATA_SRC := \
	core/recordings/src/MetadataRepository.cpp \
	core/recordings/src/MetadataService.cpp

ACTIONS_SRC := \
	core/recordings/src/RecordingActionUtils.cpp

ACTION_SERVICE_SRC := \
	core/recordings/src/ActionService.cpp \
	core/recordings/src/RecordingActionUtils.cpp

JOB_SERVICE_SRC := \
	core/recordings/src/JobService.cpp \
	core/recordings/src/RecordingActionUtils.cpp

JOB_REPOSITORY_SRC := \
	core/recordings/src/JobRepository.cpp \
	core/recordings/src/JobService.cpp \
	core/recordings/src/RecordingActionUtils.cpp

JOB_DASHBOARD_SRC := \
	core/recordings/src/JobRepository.cpp \
	core/recordings/src/JobDashboardService.cpp

RECORDING_DASHBOARD_SRC := \
	core/recordings/src/RecordingRepository.cpp \
	core/recordings/src/MetadataRepository.cpp \
	core/recordings/src/RecordingDashboardService.cpp

DASHBOARD_FACADE_SRC := \
	core/recordings/src/JobRepository.cpp \
	core/recordings/src/JobDashboardService.cpp \
	core/recordings/src/RecordingRepository.cpp \
	core/recordings/src/MetadataRepository.cpp \
	core/recordings/src/RecordingDashboardService.cpp \
	core/recordings/src/DashboardFacade.cpp

DASHBOARD_JSON_SRC := \
	core/recordings/src/DashboardJsonSerializer.cpp

DASHBOARD_CLI_SRC := \
	core/recordings/src/JobRepository.cpp \
	core/recordings/src/JobDashboardService.cpp \
	core/recordings/src/RecordingRepository.cpp \
	core/recordings/src/MetadataRepository.cpp \
	core/recordings/src/RecordingDashboardService.cpp \
	core/recordings/src/DashboardFacade.cpp \
	core/recordings/src/DashboardJsonSerializer.cpp

REST_DASHBOARD_SRC := \
	core/recordings/src/JobRepository.cpp \
	core/recordings/src/JobDashboardService.cpp \
	core/recordings/src/RecordingRepository.cpp \
	core/recordings/src/MetadataRepository.cpp \
	core/recordings/src/RecordingDashboardService.cpp \
	core/recordings/src/DashboardFacade.cpp \
	core/recordings/src/DashboardJsonSerializer.cpp \
	api/rest/src/DashboardController.cpp

REST_ROUTER_SRC := \
        core/recordings/src/JobRepository.cpp \
        core/recordings/src/JobDashboardService.cpp \
        core/recordings/src/RecordingRepository.cpp \
        core/recordings/src/MetadataRepository.cpp \
        core/recordings/src/RecordingDashboardService.cpp \
        core/recordings/src/DashboardFacade.cpp \
        core/recordings/src/DashboardJsonSerializer.cpp \
        api/rest/src/DashboardController.cpp \
        api/rest/src/JobsController.cpp \
        api/rest/src/RecordingsController.cpp \
        api/rest/src/MetadataController.cpp \
        api/rest/src/ApiRouter.cpp

WORKFLOW_SRC := \
	core/recordings/src/RecordingWorkflowService.cpp \
	core/recordings/src/ActionService.cpp \
	core/recordings/src/JobService.cpp \
	core/recordings/src/JobRepository.cpp \
	core/recordings/src/RecordingActionUtils.cpp

WORKER_SRC := \
	core/recordings/src/WorkerSimulator.cpp

RECTOOLS_ADAPTER_SRC := \
	core/recordings/src/RectoolsAdapter.cpp \
	core/recordings/src/JobService.cpp \
	core/recordings/src/RecordingActionUtils.cpp

DAEMON_SRC := \
        core/daemon/src/RuntimeConfig.cpp \
        core/recordings/src/JobRepository.cpp \
        core/recordings/src/JobDashboardService.cpp \
        core/recordings/src/RecordingRepository.cpp \
        core/recordings/src/MetadataRepository.cpp \
        core/recordings/src/RecordingDashboardService.cpp \
        core/recordings/src/DashboardFacade.cpp \
        core/daemon/src/DaemonRuntime.cpp \
        core/daemon/src/DaemonApp.cpp

.PHONY: all test clean prepare-test-db dashboard-cli daemon

all: test

prepare-test-db:
	rm -f /tmp/vdr-suite-test.db
	sqlite3 /tmp/vdr-suite-test.db < database/schema/vdr-suite.sql
	sqlite3 /tmp/vdr-suite-test.db < database/testdata/sample-data.sql

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
		$(REST_ROUTER_SRC) \
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

test-vdr-config:
	$(CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/tests/test_vdr_config.cpp \
		-o /tmp/test_vdr_config
	/tmp/test_vdr_config

test-external-vdr-adapter:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_external_vdr_adapter.cpp \
		-o /tmp/test_external_vdr_adapter
	/tmp/test_external_vdr_adapter

test-mock-vdr-adapter:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_mock_vdr_adapter.cpp \
		-o /tmp/test_mock_vdr_adapter
	/tmp/test_mock_vdr_adapter

test-vdr-adapter-factory:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_adapter_factory.cpp \
		-o /tmp/test_vdr_adapter_factory
	/tmp/test_vdr_adapter_factory

test-vdr-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_service.cpp \
		-o /tmp/test_vdr_service
	/tmp/test_vdr_service

test-vdr-overview-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_overview_service.cpp \
		-o /tmp/test_vdr_overview_service
	/tmp/test_vdr_overview_service

test-vdr-overview-json-serializer:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_overview_json_serializer.cpp \
		-o /tmp/test_vdr_overview_json_serializer
	/tmp/test_vdr_overview_json_serializer

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

test-mock-http-client:
	$(CXX) $(CXXFLAGS) \
		$(HTTP_SRC) \
		core/http/tests/test_mock_http_client.cpp \
		-o /tmp/test_mock_http_client
	/tmp/test_mock_http_client

test-restful-api-status-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_status_mapper.cpp \
		-o /tmp/test_restful_api_status_mapper
	/tmp/test_restful_api_status_mapper

test-restful-api-event-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_event_mapper.cpp \
		-o /tmp/test_restful_api_event_mapper
	/tmp/test_restful_api_event_mapper

test-restful-api-channel-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_channel_mapper.cpp \
		-o /tmp/test_restful_api_channel_mapper
	/tmp/test_restful_api_channel_mapper

test-restful-api-recording-mapper:
	$(CXX) $(CXXFLAGS) \
			$(VDR_SRC) \
			core/vdr/tests/test_restful_api_recording_mapper.cpp \
			-o /tmp/test_restful_api_recording_mapper
	/tmp/test_restful_api_recording_mapper

test-restful-api-timer-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_timer_mapper.cpp \
		-o /tmp/test_restful_api_timer_mapper
	/tmp/test_restful_api_timer_mapper

test-restful-api-vdr-adapter:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(HTTP_SRC) \
		core/vdr/tests/test_restful_api_vdr_adapter.cpp \
		-o /tmp/test_restful_api_vdr_adapter
	/tmp/test_restful_api_vdr_adapter

test-vdr-domain-objects:
	$(CXX) $(CXXFLAGS) \
		core/vdr/tests/test_vdr_domain_objects.cpp \
		-o /tmp/test_vdr_domain_objects
	/tmp/test_vdr_domain_objects

daemon:
	$(CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(DAEMON_SRC) \
		apps/daemon/main.cpp \
		$(LDFLAGS) \
		-o /tmp/vdr-suite-daemon

test: test-database test-recording-repository test-recording-service test-metadata-service test-recording-action test-action-service test-job-service test-job-repository test-job-dashboard-service test-recording-dashboard-service test-dashboard-facade test-dashboard-json-serializer test-dashboard-controller test-jobs-controller test-recordings-controller test-metadata-controller test-api-router test-workflow-service test-worker-simulator test-rectools-adapter test-vdr-config test-external-vdr-adapter test-vdr-adapter-factory test-vdr-service test-vdr-overview-service test-vdr-overview-json-serializer test-mock-vdr-adapter test-http-request test-http-response test-mock-http-client test-restful-api-status-mapper test-restful-api-event-mapper test-restful-api-channel-mapper test-restful-api-recording-mapper test-restful-api-timer-mapper test-restful-api-vdr-adapter test-vdr-domain-objects

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
	rm -f /tmp/test_vdr_domain_objects
	rm -f /tmp/test_vdr_service
	rm -f /tmp/test_vdr_overview_service
	rm -f /tmp/test_vdr_overview_json_serializer
