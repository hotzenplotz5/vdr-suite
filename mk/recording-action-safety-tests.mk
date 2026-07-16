test-recording-action-execution-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_recording_action_execution_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_result_json_serializer
	$(BUILD_DIR)/test_recording_action_execution_result_json_serializer

test-recording-action-validation-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		$(RECORDING_ACTION_REST_PARSER_SRC) \
		api/rest/tests/test_recording_action_validation_request_parser.cpp \
		-o $(BUILD_DIR)/test_recording_action_validation_request_parser
	$(BUILD_DIR)/test_recording_action_validation_request_parser

test-vdr-timer-action-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(VDR_TIMER_ACTION_REST_PARSER_SRC) \
		api/rest/tests/test_vdr_timer_action_request_parser.cpp \
		-o $(BUILD_DIR)/test_vdr_timer_action_request_parser
	$(BUILD_DIR)/test_vdr_timer_action_request_parser

test-vdr-timer-action-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/MockVdrTimerActionExecutor.cpp \
		core/vdr/src/VdrTimerActionService.cpp \
		core/vdr/src/VdrTimerActionExecutionService.cpp \
		core/vdr/src/VdrTimerActionResultJsonSerializer.cpp \
		api/rest/src/VdrTimerActionRequestParser.cpp \
		api/rest/src/VdrTimerActionController.cpp \
		api/rest/tests/test_vdr_timer_action_controller.cpp \
		-o $(BUILD_DIR)/test_vdr_timer_action_controller
	$(BUILD_DIR)/test_vdr_timer_action_controller

test-recording-action-execution-controller-safety-preview:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_REST_CONTROLLER_SRC) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
		core/http/src/MockHttpClient.cpp \
		api/rest/src/RecordingActionExecutionController.cpp \
		api/rest/tests/test_recording_action_execution_controller_safety_preview.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_controller_safety_preview
	$(BUILD_DIR)/test_recording_action_execution_controller_safety_preview

test-recording-action-execution-service-registry-safety:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_EXECUTOR_ADAPTER_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_recording_action_execution_service_registry_safety.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_service_registry_safety
	$(BUILD_DIR)/test_recording_action_execution_service_registry_safety

test-recording-action-backend-executor-registry-capabilities:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_EXECUTOR_ADAPTER_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_recording_action_backend_executor_registry_capabilities.cpp \
		-o $(BUILD_DIR)/test_recording_action_backend_executor_registry_capabilities
	$(BUILD_DIR)/test_recording_action_backend_executor_registry_capabilities

test-restfulapi-recording-action-executor-capabilities:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_EXECUTOR_ADAPTER_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_restfulapi_recording_action_executor_capabilities.cpp \
		-o $(BUILD_DIR)/test_restfulapi_recording_action_executor_capabilities
	$(BUILD_DIR)/test_restfulapi_recording_action_executor_capabilities

test-restfulapi-executor-preserves-http-error-status:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_RESTFULAPI_EXECUTOR_SRC) \
		core/http/src/MockHttpClient.cpp \
		core/recordings/tests/test_restfulapi_executor_preserves_http_error_status.cpp \
		-o $(BUILD_DIR)/test_restfulapi_executor_preserves_http_error_status
	$(BUILD_DIR)/test_restfulapi_executor_preserves_http_error_status

test-recording-action-execution-service-capability-safety:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_recording_action_execution_service_capability_safety.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_service_capability_safety
	$(BUILD_DIR)/test_recording_action_execution_service_capability_safety

test-recording-action-capability-safety-integration:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_recording_action_capability_safety_integration.cpp \
		-o $(BUILD_DIR)/test_recording_action_capability_safety_integration
	$(BUILD_DIR)/test_recording_action_capability_safety_integration

test-recording-action-capability-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_recording_action_capability_contract.cpp \
		-o $(BUILD_DIR)/test_recording_action_capability_contract
	$(BUILD_DIR)/test_recording_action_capability_contract

test-recording-action-safety-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
		core/recordings/tests/test_recording_action_safety_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_recording_action_safety_result_json_serializer
	$(BUILD_DIR)/test_recording_action_safety_result_json_serializer

test-recording-action-execution-service-safety:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_recording_action_execution_service_safety.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_service_safety
	$(BUILD_DIR)/test_recording_action_execution_service_safety

test-recording-action-execution-controller-execute-body-policy-gate:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_REST_CONTROLLER_SRC) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
		api/rest/src/RecordingActionExecutionController.cpp \
		api/rest/tests/test_recording_action_execution_controller_execute_body_policy_gate.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_controller_execute_body_policy_gate
	$(BUILD_DIR)/test_recording_action_execution_controller_execute_body_policy_gate

test-recording-action-execution-controller-policy-execute:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_REST_CONTROLLER_SRC) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
		api/rest/src/RecordingActionExecutionController.cpp \
		api/rest/tests/test_recording_action_execution_controller_policy_execute.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_controller_policy_execute
	$(BUILD_DIR)/test_recording_action_execution_controller_policy_execute

test-recording-action-policy-gated-execute:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_recording_action_policy_gated_execute.cpp \
		-o $(BUILD_DIR)/test_recording_action_policy_gated_execute
	$(BUILD_DIR)/test_recording_action_policy_gated_execute

test-recording-action-execution-controller-policy-safety:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_REST_CONTROLLER_SRC) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
		api/rest/src/RecordingActionExecutionController.cpp \
		api/rest/tests/test_recording_action_execution_controller_policy_safety.cpp \
		-o $(BUILD_DIR)/test_recording_action_execution_controller_policy_safety
	$(BUILD_DIR)/test_recording_action_execution_controller_policy_safety

test-recording-action-backend-policy-provider:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/recordings/tests/test_recording_action_backend_policy_provider.cpp \
		-o $(BUILD_DIR)/test_recording_action_backend_policy_provider
	$(BUILD_DIR)/test_recording_action_backend_policy_provider

test-recording-action-backend-registry-policy-mapping:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/recordings/tests/test_recording_action_backend_registry_policy_mapping.cpp \
		-o $(BUILD_DIR)/test_recording_action_backend_registry_policy_mapping
	$(BUILD_DIR)/test_recording_action_backend_registry_policy_mapping

test-recording-action-backend-policy-safety-json-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
		core/recordings/tests/test_recording_action_backend_policy_safety_json_contract.cpp \
		-o $(BUILD_DIR)/test_recording_action_backend_policy_safety_json_contract
	$(BUILD_DIR)/test_recording_action_backend_policy_safety_json_contract

test-recording-action-backend-policy-safety:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_recording_action_backend_policy_safety.cpp \
		-o $(BUILD_DIR)/test_recording_action_backend_policy_safety
	$(BUILD_DIR)/test_recording_action_backend_policy_safety

test-recording-action-backend-policy:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_recording_action_backend_policy.cpp \
		-o $(BUILD_DIR)/test_recording_action_backend_policy
	$(BUILD_DIR)/test_recording_action_backend_policy

test-recording-action-permission-safety-json-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/src/RecordingActionSafetyResultJsonSerializer.cpp \
		core/recordings/tests/test_recording_action_permission_safety_json_contract.cpp \
		-o $(BUILD_DIR)/test_recording_action_permission_safety_json_contract
	$(BUILD_DIR)/test_recording_action_permission_safety_json_contract

test-recording-action-permission-safety-integration:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_recording_action_permission_safety_integration.cpp \
		-o $(BUILD_DIR)/test_recording_action_permission_safety_integration
	$(BUILD_DIR)/test_recording_action_permission_safety_integration

test-recording-action-permission-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_recording_action_permission_contract.cpp \
		-o $(BUILD_DIR)/test_recording_action_permission_contract
	$(BUILD_DIR)/test_recording_action_permission_contract

test-recording-action-safety-reason-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_recording_action_safety_reason_contract.cpp \
		-o $(BUILD_DIR)/test_recording_action_safety_reason_contract
	$(BUILD_DIR)/test_recording_action_safety_reason_contract

test-recording-action-safety-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(RECORDING_ACTION_CORE_SRC) \
		core/recordings/tests/test_recording_action_safety_contract.cpp \
		-o $(BUILD_DIR)/test_recording_action_safety_contract
	$(BUILD_DIR)/test_recording_action_safety_contract

