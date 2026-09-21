test-recording-action:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_LEGACY_TEST_SRC) \
		core/recordings/tests/test_recording_action.cpp \
		-o $(BUILD_DIR)/test_recording_action
	$(BUILD_DIR)/test_recording_action

test-action-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(ACTION_SERVICE_SRC) \
		core/recordings/tests/test_action_service.cpp \
		-o $(BUILD_DIR)/test_action_service
	$(BUILD_DIR)/test_action_service

test-job-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(JOB_SERVICE_SRC) \
		core/recordings/tests/test_job_service.cpp \
		-o $(BUILD_DIR)/test_job_service
	$(BUILD_DIR)/test_job_service

test-job-repository: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(JOB_REPOSITORY_SRC) \
		core/recordings/tests/test_job_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_job_repository
	$(BUILD_DIR)/test_job_repository

test-job-dashboard-service: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(JOB_DASHBOARD_SRC) \
		core/recordings/tests/test_job_dashboard_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_job_dashboard_service
	$(BUILD_DIR)/test_job_dashboard_service

test-recording-dashboard-service: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(RECORDING_DASHBOARD_SRC) \
		core/recordings/tests/test_recording_dashboard_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recording_dashboard_service
	$(BUILD_DIR)/test_recording_dashboard_service

test-dashboard-facade: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(DASHBOARD_FACADE_SRC) \
		core/recordings/tests/test_dashboard_facade.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_dashboard_facade
	$(BUILD_DIR)/test_dashboard_facade

test-dashboard-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(DASHBOARD_JSON_SRC) \
		core/recordings/tests/test_dashboard_json_serializer.cpp \
		-o $(BUILD_DIR)/test_dashboard_json_serializer
	$(BUILD_DIR)/test_dashboard_json_serializer

test-dashboard-controller: prepare-test-db
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(REST_DASHBOARD_SRC) \
		api/rest/tests/test_dashboard_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_dashboard_controller
	$(BUILD_DIR)/test_dashboard_controller

test-vdr-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		$(REST_VDR_SRC) \
		api/rest/tests/test_vdr_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_controller
	$(BUILD_DIR)/test_vdr_controller

