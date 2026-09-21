test-search-timer-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerResultJsonSerializer.cpp \
                core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/EpgSearchMatcher.cpp \
		core/vdr/src/EpgSearchRequestMapper.cpp core/vdr/src/EpgSearchService.cpp \
		core/vdr/src/EpgSearchResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerCreateService.cpp \
		core/vdr/src/SearchTimerCreateResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerUpdateService.cpp \
		core/vdr/src/SearchTimerUpdateResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerDeleteService.cpp \
		core/vdr/src/SearchTimerDeleteResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerPreviewService.cpp \
		core/vdr/src/SearchTimerPreviewResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWriteAllowlist.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWritePermissionGate.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/src/SearchTimerWorkflowRealExecutionPolicy.cpp \
		core/vdr/src/SearchTimerWorkflowGuardedExecutorInvocation.cpp \
		core/vdr/src/SearchTimerWorkflowExecutorInvocationKillSwitch.cpp \
		core/vdr/src/SearchTimerWorkflowExecutorResultMapper.cpp \
		core/vdr/src/SearchTimerWorkflowCreateReadbackVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowUpdateReadbackVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowDeleteAbsenceVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowCommandDispatchService.cpp \
		core/vdr/src/SearchTimerWorkflowExecutionService.cpp \
		core/vdr/src/SearchTimerWorkflowExecutionResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerWorkflowExecutionPlanJsonSerializer.cpp \
		core/vdr/src/SearchTimerWorkflowValidationResultJsonSerializer.cpp \
		api/rest/src/SearchTimerCreateRequestParser.cpp \
                api/rest/src/SearchTimerUpdateRequestParser.cpp \
                api/rest/src/SearchTimerDeleteRequestParser.cpp \
		api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp \
		api/rest/src/SearchTimerController.cpp \
		api/rest/tests/test_search_timer_controller.cpp \
		-o $(BUILD_DIR)/test_search_timer_controller
	$(BUILD_DIR)/test_search_timer_controller
test-search-timer-discovery-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerDiscoveryJsonSerializer.cpp \
		core/vdr/src/SearchTimerDiscoveryService.cpp \
		api/rest/src/SearchTimerDiscoveryController.cpp \
		api/rest/tests/test_search_timer_discovery_controller.cpp \
		-o $(BUILD_DIR)/test_search_timer_discovery_controller
	$(BUILD_DIR)/test_search_timer_discovery_controller

test-search-timer-automation-preview-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerAutomationDryRunResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerAutomationReadOnlyService.cpp \
		api/rest/src/SearchTimerAutomationPreviewController.cpp \
		api/rest/tests/test_search_timer_automation_preview_controller.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_preview_controller
	$(BUILD_DIR)/test_search_timer_automation_preview_controller
test-search-timer-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerResultJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_result_json_serializer
	$(BUILD_DIR)/test_search_timer_result_json_serializer

test-restful-api-search-timer-adapter:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/MockHttpClient.cpp \
		core/vdr/src/RestfulApiSearchTimerMapper.cpp \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/RestfulApiSearchTimerAdapter.cpp \
		core/vdr/tests/test_restful_api_search_timer_adapter.cpp \
		-o $(BUILD_DIR)/test_restful_api_search_timer_adapter
	$(BUILD_DIR)/test_restful_api_search_timer_adapter
test-restful-api-search-timer-command-executor:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiSearchTimerCommandExecutor.cpp \
		core/vdr/tests/test_restful_api_search_timer_command_executor.cpp \
		-o $(BUILD_DIR)/test_restful_api_search_timer_command_executor
	$(BUILD_DIR)/test_restful_api_search_timer_command_executor

test-restful-api-search-timer-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiSearchTimerMapper.cpp \
		core/vdr/tests/test_restful_api_search_timer_mapper.cpp \
		-o $(BUILD_DIR)/test_restful_api_search_timer_mapper
	$(BUILD_DIR)/test_restful_api_search_timer_mapper
test-search-timer-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/tests/test_search_timer_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_service
	$(BUILD_DIR)/test_search_timer_service
test-search-timer-service-interface:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_service_interface.cpp \
		-o $(BUILD_DIR)/test_search_timer_service_interface
	$(BUILD_DIR)/test_search_timer_service_interface
test-search-timer-result:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_result.cpp \
		-o $(BUILD_DIR)/test_search_timer_result
	$(BUILD_DIR)/test_search_timer_result
test-search-timer-workflow-delete-absence-verification-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/SearchTimerWorkflowDeleteAbsenceVerificationService.cpp \
		core/vdr/tests/test_search_timer_workflow_delete_absence_verification_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_delete_absence_verification_service
	$(BUILD_DIR)/test_search_timer_workflow_delete_absence_verification_service

test-search-timer-workflow-update-readback-verification-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/SearchTimerWorkflowUpdateReadbackVerificationService.cpp \
		core/vdr/tests/test_search_timer_workflow_update_readback_verification_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_update_readback_verification_service
	$(BUILD_DIR)/test_search_timer_workflow_update_readback_verification_service

test-search-timer-workflow-create-readback-verification-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/SearchTimerWorkflowCreateReadbackVerificationService.cpp \
		core/vdr/tests/test_search_timer_workflow_create_readback_verification_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_create_readback_verification_service
	$(BUILD_DIR)/test_search_timer_workflow_create_readback_verification_service

test-search-timer-workflow-backend-readback-verification-result:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_workflow_backend_readback_verification_result.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_backend_readback_verification_result
	$(BUILD_DIR)/test_search_timer_workflow_backend_readback_verification_result

test-search-timer-workflow-execution-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowExecutionService.cpp \
		core/vdr/src/SearchTimerWorkflowExecutionResultJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_workflow_execution_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_execution_result_json_serializer
	$(BUILD_DIR)/test_search_timer_workflow_execution_result_json_serializer

test-search-timer-workflow-end-to-end-verified-execution:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWriteAllowlist.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWritePermissionGate.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/src/SearchTimerWorkflowRealExecutionPolicy.cpp \
		core/vdr/src/SearchTimerWorkflowGuardedExecutorInvocation.cpp \
		core/vdr/src/SearchTimerWorkflowExecutorInvocationKillSwitch.cpp \
		core/vdr/src/SearchTimerWorkflowExecutorResultMapper.cpp \
		core/vdr/src/SearchTimerWorkflowCreateReadbackVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowUpdateReadbackVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowDeleteAbsenceVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowCommandDispatchService.cpp \
		core/vdr/tests/test_search_timer_workflow_end_to_end_verified_execution.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_end_to_end_verified_execution
	$(BUILD_DIR)/test_search_timer_workflow_end_to_end_verified_execution

test-search-timer-workflow-command-dispatch-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWriteAllowlist.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWritePermissionGate.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/src/SearchTimerWorkflowRealExecutionPolicy.cpp \
		core/vdr/src/SearchTimerWorkflowGuardedExecutorInvocation.cpp \
		core/vdr/src/SearchTimerWorkflowExecutorInvocationKillSwitch.cpp \
		core/vdr/src/SearchTimerWorkflowExecutorResultMapper.cpp \
		core/vdr/src/SearchTimerWorkflowCreateReadbackVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowUpdateReadbackVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowDeleteAbsenceVerificationService.cpp \
		core/vdr/src/SearchTimerWorkflowCommandDispatchService.cpp \
		core/vdr/tests/test_search_timer_workflow_command_dispatch_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_command_dispatch_service
	$(BUILD_DIR)/test_search_timer_workflow_command_dispatch_service

test-search-timer-workflow-production-policy-gate:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/tests/test_search_timer_workflow_production_policy_gate.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_production_policy_gate
	$(BUILD_DIR)/test_search_timer_workflow_production_policy_gate

test-search-timer-workflow-backend-write-permission-gate:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWritePermissionGate.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/tests/test_search_timer_workflow_backend_write_permission_gate.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_backend_write_permission_gate
	$(BUILD_DIR)/test_search_timer_workflow_backend_write_permission_gate

test-search-timer-workflow-backend-write-allowlist:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWriteAllowlist.cpp \
		core/vdr/tests/test_search_timer_workflow_backend_write_allowlist.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_backend_write_allowlist
	$(BUILD_DIR)/test_search_timer_workflow_backend_write_allowlist

test-search-timer-workflow-real-execution-enablement-switch:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowRealExecutionEnablementSwitch.cpp \
		core/vdr/tests/test_search_timer_workflow_real_execution_enablement_switch.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_real_execution_enablement_switch
	$(BUILD_DIR)/test_search_timer_workflow_real_execution_enablement_switch

test-search-timer-workflow-production-executor-hardening-plan:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowProductionExecutorHardeningPlan.cpp \
		core/vdr/tests/test_search_timer_workflow_production_executor_hardening_plan.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_production_executor_hardening_plan
	$(BUILD_DIR)/test_search_timer_workflow_production_executor_hardening_plan

test-search-timer-workflow-production-executor-hardening-plan-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowProductionExecutorHardeningPlan.cpp \
		core/vdr/src/SearchTimerWorkflowProductionExecutorHardeningPlanJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_workflow_production_executor_hardening_plan_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_production_executor_hardening_plan_json_serializer
	$(BUILD_DIR)/test_search_timer_workflow_production_executor_hardening_plan_json_serializer

test-search-timer-workflow-real-execution-readiness-review:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWriteAllowlist.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWritePermissionGate.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/src/SearchTimerWorkflowRealExecutionReadinessReview.cpp \
		core/vdr/tests/test_search_timer_workflow_real_execution_readiness_review.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_real_execution_readiness_review
	$(BUILD_DIR)/test_search_timer_workflow_real_execution_readiness_review

test-search-timer-workflow-real-execution-readiness-review-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWriteAllowlist.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWritePermissionGate.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/src/SearchTimerWorkflowRealExecutionReadinessReview.cpp \
		core/vdr/src/SearchTimerWorkflowRealExecutionReadinessReviewJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_workflow_real_execution_readiness_review_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_real_execution_readiness_review_json_serializer
	$(BUILD_DIR)/test_search_timer_workflow_real_execution_readiness_review_json_serializer

