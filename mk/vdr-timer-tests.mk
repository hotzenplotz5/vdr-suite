test-vdr-timer-operation-request:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_vdr_timer_operation_request.cpp \
		-o $(BUILD_DIR)/test_vdr_timer_operation_request
	$(BUILD_DIR)/test_vdr_timer_operation_request

test-vdr-timer-action-result:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_vdr_timer_action_result.cpp \
		-o $(BUILD_DIR)/test_vdr_timer_action_result
	$(BUILD_DIR)/test_vdr_timer_action_result

test-vdr-timer-action-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrTimerActionResultJsonSerializer.cpp \
		core/vdr/tests/test_vdr_timer_action_result_json_serializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_timer_action_result_json_serializer
	$(BUILD_DIR)/test_vdr_timer_action_result_json_serializer

test-vdr-timer-action-executor-adapter-registry:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/MockVdrTimerActionExecutor.cpp \
		core/vdr/tests/test_vdr_timer_action_executor_adapter_registry.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_timer_action_executor_adapter_registry
	$(BUILD_DIR)/test_vdr_timer_action_executor_adapter_registry

test-restful-api-vdr-timer-action-executor-adapter:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/MockHttpClient.cpp \
		core/vdr/src/RestfulApiVdrTimerActionExecutor.cpp \
		core/vdr/src/RestfulApiVdrTimerActionExecutorAdapter.cpp \
		core/vdr/tests/test_restful_api_vdr_timer_action_executor_adapter.cpp \
		-o $(BUILD_DIR)/test_restful_api_vdr_timer_action_executor_adapter
	$(BUILD_DIR)/test_restful_api_vdr_timer_action_executor_adapter

test-vdr-timer-action-execution-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/MockVdrTimerActionExecutor.cpp \
		core/vdr/src/VdrTimerActionExecutionService.cpp \
		core/vdr/tests/test_vdr_timer_action_execution_service.cpp \
		-o $(BUILD_DIR)/test_vdr_timer_action_execution_service
	$(BUILD_DIR)/test_vdr_timer_action_execution_service

test-vdr-timer-action-executor-interface:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_vdr_timer_action_executor_interface.cpp \
		-o $(BUILD_DIR)/test_vdr_timer_action_executor_interface
	$(BUILD_DIR)/test_vdr_timer_action_executor_interface

test-mock-vdr-timer-action-executor:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/MockVdrTimerActionExecutor.cpp \
		core/vdr/tests/test_mock_vdr_timer_action_executor.cpp \
		-o $(BUILD_DIR)/test_mock_vdr_timer_action_executor
	$(BUILD_DIR)/test_mock_vdr_timer_action_executor

test-vdr-timer-action-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/MockVdrTimerActionExecutor.cpp \
		core/vdr/src/VdrTimerActionService.cpp \
		core/vdr/tests/test_vdr_timer_action_service.cpp \
		-o $(BUILD_DIR)/test_vdr_timer_action_service
	$(BUILD_DIR)/test_vdr_timer_action_service

test-restful-api-vdr-timer-action-request-builder:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_restful_api_vdr_timer_action_request_builder.cpp \
		-o $(BUILD_DIR)/test_restful_api_vdr_timer_action_request_builder
	$(BUILD_DIR)/test_restful_api_vdr_timer_action_request_builder

test-restful-api-vdr-timer-action-executor:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/MockHttpClient.cpp \
		core/vdr/src/RestfulApiVdrTimerActionExecutor.cpp \
		core/vdr/tests/test_restful_api_vdr_timer_action_executor.cpp \
		-o $(BUILD_DIR)/test_restful_api_vdr_timer_action_executor
	$(BUILD_DIR)/test_restful_api_vdr_timer_action_executor

test-restful-api-vdr-adapter:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/http/src/MockHttpClient.cpp \
		core/vdr/tests/test_restful_api_vdr_adapter.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_restful_api_vdr_adapter
	$(BUILD_DIR)/test_restful_api_vdr_adapter

test-restful-api-change-state-adapter:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/http/src/MockHttpClient.cpp \
		core/vdr/tests/test_restful_api_change_state_adapter.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_restful_api_change_state_adapter
	$(BUILD_DIR)/test_restful_api_change_state_adapter


test-epg-event-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/EpgEventRepository.cpp \
		core/vdr/tests/test_epg_event_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_event_repository
	$(BUILD_DIR)/test_epg_event_repository


test-vdr-domain-objects:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/tests/test_vdr_domain_objects.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_domain_objects
	$(BUILD_DIR)/test_vdr_domain_objects


test-domain-refresh-policy:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/tests/test_domain_refresh_policy.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_domain_refresh_policy
	$(BUILD_DIR)/test_domain_refresh_policy


test-snapshot-access-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SnapshotCache.cpp \
		core/vdr/src/SnapshotCacheService.cpp \
		core/vdr/src/SnapshotAccessService.cpp \
		core/vdr/tests/test_snapshot_access_service.cpp \
		-o $(BUILD_DIR)/test_snapshot_access_service
	$(BUILD_DIR)/test_snapshot_access_service
