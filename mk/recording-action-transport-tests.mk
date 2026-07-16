test-basic-http-client-socket-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/BasicHttpClient.cpp \
		core/http/tests/test_basic_http_client_socket_contract.cpp \
		-o $(BUILD_DIR)/test_basic_http_client_socket_contract
	$(BUILD_DIR)/test_basic_http_client_socket_contract

test-real-client-readonly-recording-action-executor-gate:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_EXECUTOR_ADAPTER_SRC) \
		core/http/src/BasicHttpClient.cpp \
		core/recordings/tests/test_real_client_readonly_recording_action_executor_gate.cpp \
		-o $(BUILD_DIR)/test_real_client_readonly_recording_action_executor_gate
	$(BUILD_DIR)/test_real_client_readonly_recording_action_executor_gate

test-recording-action-request-preview-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/src/RecordingActionUtils.cpp \
		core/recordings/src/RecordingActionRequestPreviewResultJsonSerializer.cpp \
		core/recordings/tests/test_recording_action_request_preview_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_recording_action_request_preview_result_json_serializer
	$(BUILD_DIR)/test_recording_action_request_preview_result_json_serializer

test-recording-action-preview-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_PREVIEW_SRC) \
		$(RECORDING_ACTION_REST_PARSER_SRC) \
		core/recordings/src/RecordingActionRequestPreviewResultJsonSerializer.cpp \
		api/rest/src/RecordingActionPreviewController.cpp \
		api/rest/tests/test_recording_action_preview_controller.cpp \
		-o $(BUILD_DIR)/test_recording_action_preview_controller
	$(BUILD_DIR)/test_recording_action_preview_controller

test-recording-action-request-preview-service-json-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_PREVIEW_SRC) \
		core/recordings/src/RecordingActionRequestPreviewResultJsonSerializer.cpp \
		core/recordings/tests/test_recording_action_request_preview_service_json_contract.cpp \
		-o $(BUILD_DIR)/test_recording_action_request_preview_service_json_contract
	$(BUILD_DIR)/test_recording_action_request_preview_service_json_contract

test-recording-action-request-preview-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_PREVIEW_SRC) \
		core/recordings/tests/test_recording_action_request_preview_service.cpp \
		-o $(BUILD_DIR)/test_recording_action_request_preview_service
	$(BUILD_DIR)/test_recording_action_request_preview_service

test-restfulapi-rename-live-error-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/BasicHttpClient.cpp \
		core/recordings/tests/test_restfulapi_rename_live_error_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_rename_live_error_contract
	$(BUILD_DIR)/test_restfulapi_rename_live_error_contract

test-restfulapi-delete-live-error-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/BasicHttpClient.cpp \
		core/recordings/tests/test_restfulapi_delete_live_error_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_delete_live_error_contract
	$(BUILD_DIR)/test_restfulapi_delete_live_error_contract

test-restfulapi-executor-basic-http-client-socket-smoke:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/BasicHttpClient.cpp \
		core/recordings/tests/test_restfulapi_executor_basic_http_client_socket_smoke.cpp \
		-o $(BUILD_DIR)/test_restfulapi_executor_basic_http_client_socket_smoke
	$(BUILD_DIR)/test_restfulapi_executor_basic_http_client_socket_smoke

test-preview-execution-restfulapi-request-equivalence:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_preview_execution_restfulapi_request_equivalence.cpp \
		-o $(BUILD_DIR)/test_preview_execution_restfulapi_request_equivalence
	$(BUILD_DIR)/test_preview_execution_restfulapi_request_equivalence

test-execution-service-restfulapi-executor-integration:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_execution_service_restfulapi_executor_integration.cpp \
		-o $(BUILD_DIR)/test_execution_service_restfulapi_executor_integration
	$(BUILD_DIR)/test_execution_service_restfulapi_executor_integration

test-restfulapi-executor-http-result-mapping:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_restfulapi_executor_http_result_mapping.cpp \
		-o $(BUILD_DIR)/test_restfulapi_executor_http_result_mapping
	$(BUILD_DIR)/test_restfulapi_executor_http_result_mapping

test-restfulapi-executor-http-transport-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_restfulapi_executor_http_transport_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_executor_http_transport_contract
	$(BUILD_DIR)/test_restfulapi_executor_http_transport_contract

test-restfulapi-execution-dispatch-allowed:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_restfulapi_execution_dispatch_allowed.cpp \
		-o $(BUILD_DIR)/test_restfulapi_execution_dispatch_allowed
	$(BUILD_DIR)/test_restfulapi_execution_dispatch_allowed

test-restfulapi-execution-gate-blocks-dispatch:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_restfulapi_execution_gate_blocks_dispatch.cpp \
		-o $(BUILD_DIR)/test_restfulapi_execution_gate_blocks_dispatch
	$(BUILD_DIR)/test_restfulapi_execution_gate_blocks_dispatch

test-restfulapi-upstream-action-endpoint-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_upstream_action_endpoint_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_upstream_action_endpoint_contract
	$(BUILD_DIR)/test_restfulapi_upstream_action_endpoint_contract

test-restfulapi-move-tilde-mapping-regression:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_move_tilde_mapping_regression.cpp \
		-o $(BUILD_DIR)/test_restfulapi_move_tilde_mapping_regression
	$(BUILD_DIR)/test_restfulapi_move_tilde_mapping_regression

test-restfulapi-action-request-preview-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_action_request_preview_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_action_request_preview_contract
	$(BUILD_DIR)/test_restfulapi_action_request_preview_contract

test-restfulapi-recording-action-empty-basepath-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_recording_action_empty_basepath_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_recording_action_empty_basepath_contract
	$(BUILD_DIR)/test_restfulapi_recording_action_empty_basepath_contract

test-restfulapi-recording-action-executor-transport-smoke:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_restfulapi_recording_action_executor_transport_smoke.cpp \
		-o $(BUILD_DIR)/test_restfulapi_recording_action_executor_transport_smoke
	$(BUILD_DIR)/test_restfulapi_recording_action_executor_transport_smoke

test-restfulapi-recording-action-executor-response-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_restfulapi_recording_action_executor_response_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_recording_action_executor_response_contract
	$(BUILD_DIR)/test_restfulapi_recording_action_executor_response_contract

test-restfulapi-recording-action-mapping-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_recording_action_mapping_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_recording_action_mapping_contract
	$(BUILD_DIR)/test_restfulapi_recording_action_mapping_contract

test-recording-action-execution-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_REST_CONTROLLER_SRC) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		api/rest/src/RecordingActionExecutionController.cpp \
		core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
		api/rest/tests/test_recording_action_execution_controller.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_controller
	$(BUILD_DIR)/test_recording_action_execution_controller

test-recording-action-validation-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_REST_CONTROLLER_SRC) \
		api/rest/src/RecordingActionValidationController.cpp \
		api/rest/tests/test_recording_action_validation_controller.cpp \
		-o $(BUILD_DIR)/test_recording_action_validation_controller
	$(BUILD_DIR)/test_recording_action_validation_controller

test-recording-action-validation-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_recording_action_validation_service.cpp \
		-o $(BUILD_DIR)/test_recording_action_validation_service
	$(BUILD_DIR)/test_recording_action_validation_service

test-recording-action-validation-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_recording_action_validation_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_recording_action_validation_result_json_serializer
	$(BUILD_DIR)/test_recording_action_validation_result_json_serializer

