CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Icore/sqlite/include -Icore/recordings/include
LDFLAGS := $(shell pkg-config --libs sqlite3)

SQLITE_SRC := core/sqlite/src/Database.cpp

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

.PHONY: all test clean prepare-test-db

all: test

prepare-test-db:
	rm -f /tmp/vdr-suite-test.db
	sqlite3 /tmp/vdr-suite-test.db < database/schema/vdr-suite.sql
	sqlite3 /tmp/vdr-suite-test.db < database/testdata/sample-data.sql

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

test: test-database test-recording-repository test-recording-service test-metadata-service test-recording-action test-action-service test-job-service test-job-repository test-job-dashboard-service test-recording-dashboard-service test-dashboard-facade test-workflow-service test-worker-simulator test-rectools-adapter

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
	rm -f /tmp/test_workflow_service
	rm -f /tmp/test_worker_simulator
	rm -f /tmp/test_rectools_adapter
	rm -f /tmp/vdr-suite-test.db
