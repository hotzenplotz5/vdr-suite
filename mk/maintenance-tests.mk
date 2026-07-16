clean:
	@build_dir="$(abspath $(BUILD_DIR))"; \
		repo_dir="$(abspath $(CURDIR))"; \
		case "$$build_dir" in \
			"$$repo_dir"/.build|"$$repo_dir"/.build/*) rm -rf -- "$$build_dir" ;; \
			*) echo "Refusing to clean BUILD_DIR outside $$repo_dir/.build: $$build_dir" >&2; exit 2 ;; \
		esac
	rm -f /tmp/vdr-suite-test.db

.PHONY: test-recording-action-executor-interface
test-recording-action-backend-execution-path:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_LEGACY_TEST_SRC) \
		core/recordings/tests/test_recording_action_backend_execution_path.cpp \
		-o $(BUILD_DIR)/test_recording_action_backend_execution_path
	$(BUILD_DIR)/test_recording_action_backend_execution_path

test-recording-action-execution-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_LEGACY_TEST_SRC) \
		core/recordings/tests/test_recording_action_execution_service.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_service
	$(BUILD_DIR)/test_recording_action_execution_service

test-recording-action-job-payload-factory:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_LEGACY_TEST_SRC) \
		core/recordings/tests/test_recording_action_job_payload_factory.cpp \
		-o $(BUILD_DIR)/test_recording_action_job_payload_factory
	$(BUILD_DIR)/test_recording_action_job_payload_factory

test-recording-action-executor-interface:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_LEGACY_TEST_SRC) \
		core/recordings/tests/test_recording_action_executor_interface.cpp \
		-o $(BUILD_DIR)/test_recording_action_executor_interface
	$(BUILD_DIR)/test_recording_action_executor_interface

.PHONY: test-recording-action-execution-result
test-recording-action-execution-result:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/recordings/include \
		core/recordings/tests/test_recording_action_execution_result.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_result
	$(BUILD_DIR)/test_recording_action_execution_result

.PHONY: test-docs
test-docs:
	python3 tools/check_docs.py
	python3 tools/check_doc_indexes.py
	python3 tools/check_doc_reachability.py
	python3 tools/check_doc_entrypoints.py
	python3 tools/check_adr_index.py
	python3 tools/check_completed_phase_markers.py

.PHONY: test-architecture
test-architecture:
	python3 tools/check_architecture.py

.PHONY: test-phase
test-phase:
	python3 tools/check_phase_consistency.py


.PHONY: test-capability-report-service
test-capability-report-service:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/CapabilityReportService.cpp \
		core/vdr/tests/test_capability_report_service.cpp \
		-o $(BUILD_DIR)/test_capability_report_service
	$(BUILD_DIR)/test_capability_report_service

.PHONY: test-capability-controller
test-capability-controller:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include -Iapi/rest/include \
		core/vdr/src/CapabilityStateJsonSerializer.cpp \
		core/vdr/src/CapabilityReportJsonSerializer.cpp \
		core/vdr/src/CapabilityReportService.cpp \
		api/rest/src/CapabilityController.cpp \
		api/rest/tests/test_capability_controller.cpp \
		-o $(BUILD_DIR)/test_capability_controller
	$(BUILD_DIR)/test_capability_controller







.PHONY: test-vdr-recording-query-controller
test-vdr-recording-query-controller:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include -Icore/runtime/include -Iapi/rest/include \
		core/vdr/src/VdrService.cpp \
		core/vdr/src/VdrChangeState.cpp \
		core/vdr/src/MockVdrAdapter.cpp \
		core/vdr/src/VdrRecordingQueryMatcher.cpp \
		core/vdr/src/VdrRecordingQueryService.cpp \
		core/vdr/src/VdrRecordingQueryResultJsonSerializer.cpp \
		api/rest/src/VdrRecordingQueryController.cpp \
		api/rest/tests/test_vdr_recording_query_controller.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_query_controller
	$(BUILD_DIR)/test_vdr_recording_query_controller

.PHONY: test-vdr-recording-query-result-json-serializer
test-vdr-recording-query-result-json-serializer:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/VdrRecordingQueryResultJsonSerializer.cpp \
		core/vdr/tests/test_vdr_recording_query_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_query_result_json_serializer
	$(BUILD_DIR)/test_vdr_recording_query_result_json_serializer

.PHONY: test-vdr-recording-query-matcher
test-vdr-recording-query-matcher:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/VdrRecordingQueryMatcher.cpp \
		core/vdr/tests/test_vdr_recording_query_matcher.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_query_matcher
	$(BUILD_DIR)/test_vdr_recording_query_matcher

.PHONY: test-vdr-recording-query-service
test-vdr-recording-query-service:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include -Icore/runtime/include \
		core/vdr/src/VdrService.cpp \
		core/vdr/src/VdrChangeState.cpp \
		core/vdr/src/MockVdrAdapter.cpp \
		core/vdr/src/VdrRecordingQueryMatcher.cpp \
		core/vdr/src/VdrRecordingQueryService.cpp \
		core/vdr/tests/test_vdr_recording_query_service.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_query_service
	$(BUILD_DIR)/test_vdr_recording_query_service

.PHONY: test-vdr-recording-query-result
test-vdr-recording-query-result:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_vdr_recording_query_result.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_query_result
	$(BUILD_DIR)/test_vdr_recording_query_result

.PHONY: test-vdr-recording-query
test-vdr-recording-query:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_vdr_recording_query.cpp \
		-o $(BUILD_DIR)/test_vdr_recording_query
	$(BUILD_DIR)/test_vdr_recording_query

.PHONY: test-capability-report-json-serializer
test-capability-report-json-serializer:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/CapabilityStateJsonSerializer.cpp \
		core/vdr/src/CapabilityReportJsonSerializer.cpp \
		core/vdr/tests/test_capability_report_json_serializer.cpp \
		-o $(BUILD_DIR)/test_capability_report_json_serializer
	$(BUILD_DIR)/test_capability_report_json_serializer

.PHONY: test-capability-report-builder
test-capability-report-builder:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_capability_report_builder.cpp \
		-o $(BUILD_DIR)/test_capability_report_builder
	$(BUILD_DIR)/test_capability_report_builder

.PHONY: test-capability-report
test-capability-report:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_capability_report.cpp \
		-o $(BUILD_DIR)/test_capability_report
	$(BUILD_DIR)/test_capability_report

.PHONY: test-capability-state-json-serializer
test-capability-state-json-serializer:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/src/CapabilityStateJsonSerializer.cpp \
		core/vdr/tests/test_capability_state_json_serializer.cpp \
		-o $(BUILD_DIR)/test_capability_state_json_serializer
	$(BUILD_DIR)/test_capability_state_json_serializer

.PHONY: test-capability-state
test-capability-state:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_capability_state.cpp \
		-o $(BUILD_DIR)/test_capability_state
	$(BUILD_DIR)/test_capability_state

.PHONY: test-capability-resolver
test-capability-resolver:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_capability_resolver.cpp \
		-o $(BUILD_DIR)/test_capability_resolver
	$(BUILD_DIR)/test_capability_resolver

.PHONY: test-vdr-capability-set
test-vdr-capability-set:
	$(BUILD_CXX) -std=c++17 -Wall -Wextra -Icore/vdr/include \
		core/vdr/tests/test_vdr_capability_set.cpp \
		-o $(BUILD_DIR)/test_vdr_capability_set
	$(BUILD_DIR)/test_vdr_capability_set

test-restfulapi-search-timer-command-executor:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiSearchTimerCommandExecutor.cpp \
		core/vdr/tests/test_restfulapi_search_timer_command_executor.cpp \
		-o $(BUILD_DIR)/test_restfulapi_search_timer_command_executor
	$(BUILD_DIR)/test_restfulapi_search_timer_command_executor


test-vdr-channel-move-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/VdrChannelMoveExecutionService.cpp \
		core/vdr/src/VdrChannelMoveExecutorAdapterRegistry.cpp \
		core/vdr/src/VdrChannelMoveResultJsonSerializer.cpp \
		api/rest/src/VdrChannelMoveRequestParser.cpp \
		api/rest/src/VdrChannelMoveController.cpp \
		api/rest/tests/test_vdr_channel_move_controller.cpp \
		-o $(BUILD_DIR)/test_vdr_channel_move_controller
	$(BUILD_DIR)/test_vdr_channel_move_controller

test-restful-api-timer-conflict-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiTimerConflictMapper.cpp \
		core/vdr/tests/test_restful_api_timer_conflict_mapper.cpp \
		-o $(BUILD_DIR)/test_restful_api_timer_conflict_mapper
	$(BUILD_DIR)/test_restful_api_timer_conflict_mapper

test-restful-api-vdr-adapter-timer-conflicts:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/VdrChangeState.cpp \
		core/vdr/src/RestfulApiChannelMapper.cpp \
		core/vdr/src/RestfulApiRecordingMapper.cpp \
		core/vdr/src/RestfulApiTimerMapper.cpp \
		core/vdr/src/RestfulApiTimerConflictMapper.cpp \
		core/vdr/src/RestfulApiEventMapper.cpp \
		core/vdr/src/RestfulApiStatusMapper.cpp \
		core/vdr/src/RestfulApiVdrAdapter.cpp \
		core/http/src/MockHttpClient.cpp \
		core/vdr/tests/test_restful_api_vdr_adapter_timer_conflicts.cpp \
		-o $(BUILD_DIR)/test_restful_api_vdr_adapter_timer_conflicts
	$(BUILD_DIR)/test_restful_api_vdr_adapter_timer_conflicts

test-vdr-timer-conflict-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrSnapshotReadJsonSerializer.cpp \
		core/vdr/tests/test_vdr_timer_conflict_json_serializer.cpp \
		-o $(BUILD_DIR)/test_vdr_timer_conflict_json_serializer
	$(BUILD_DIR)/test_vdr_timer_conflict_json_serializer

test-vdr-timer-conflicts: test-restful-api-timer-conflict-mapper test-restful-api-vdr-adapter-timer-conflicts test-vdr-timer-conflict-json-serializer

.PHONY: test-vdr-recording-cache-repository
test-vdr-recording-cache-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_recording_cache_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_cache_repository
	$(BUILD_DIR)/test_vdr_recording_cache_repository

.PHONY: test-vdr-recording-query-service-cache
test-vdr-recording-query-service-cache:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/MockVdrAdapter.cpp \
		core/vdr/src/VdrService.cpp \
		core/vdr/src/VdrRecordingQueryMatcher.cpp \
		core/vdr/src/VdrRecordingQueryService.cpp \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_recording_query_service_cache.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_query_service_cache
	$(BUILD_DIR)/test_vdr_recording_query_service_cache

.PHONY: test-vdr-recording-folder-controller
test-vdr-recording-folder-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		api/rest/src/VdrRecordingFolderController.cpp \
		api/rest/tests/test_vdr_recording_folder_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_recording_folder_controller
	$(BUILD_DIR)/test_vdr_recording_folder_controller
