test-search-timer-workflow-real-execution-policy:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWriteAllowlist.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWritePermissionGate.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/src/SearchTimerWorkflowRealExecutionPolicy.cpp \
		core/vdr/tests/test_search_timer_workflow_real_execution_policy.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_real_execution_policy
	$(BUILD_DIR)/test_search_timer_workflow_real_execution_policy

test-search-timer-workflow-controlled-invocation-audit-trail:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
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
		core/vdr/tests/test_search_timer_workflow_controlled_invocation_audit_trail.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_controlled_invocation_audit_trail
	$(BUILD_DIR)/test_search_timer_workflow_controlled_invocation_audit_trail

test-search-timer-workflow-controlled-test-executor-invocation:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
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
		core/vdr/tests/test_search_timer_workflow_controlled_test_executor_invocation.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_controlled_test_executor_invocation
	$(BUILD_DIR)/test_search_timer_workflow_controlled_test_executor_invocation

test-search-timer-workflow-executor-invocation-kill-switch:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowExecutorInvocationKillSwitch.cpp \
		core/vdr/tests/test_search_timer_workflow_executor_invocation_kill_switch.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_executor_invocation_kill_switch
	$(BUILD_DIR)/test_search_timer_workflow_executor_invocation_kill_switch

test-search-timer-workflow-executor-result-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowExecutorResultMapper.cpp \
		core/vdr/tests/test_search_timer_workflow_executor_result_mapper.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_executor_result_mapper
	$(BUILD_DIR)/test_search_timer_workflow_executor_result_mapper

test-search-timer-workflow-guarded-executor-invocation:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWriteAllowlist.cpp \
		core/vdr/src/SearchTimerWorkflowBackendWritePermissionGate.cpp \
		core/vdr/src/SearchTimerWorkflowProductionPolicyGate.cpp \
		core/vdr/src/SearchTimerWorkflowRealExecutionPolicy.cpp \
		core/vdr/src/SearchTimerWorkflowGuardedExecutorInvocation.cpp \
		core/vdr/tests/test_search_timer_workflow_guarded_executor_invocation.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_guarded_executor_invocation
	$(BUILD_DIR)/test_search_timer_workflow_guarded_executor_invocation

test-search-timer-workflow-command-request-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/tests/test_search_timer_workflow_command_request_mapper.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_command_request_mapper
	$(BUILD_DIR)/test_search_timer_workflow_command_request_mapper

test-search-timer-workflow-execution-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowExecutionService.cpp \
		core/vdr/tests/test_search_timer_workflow_execution_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_execution_service
	$(BUILD_DIR)/test_search_timer_workflow_execution_service

test-search-timer-workflow-execution-plan-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/src/SearchTimerWorkflowExecutionPlanJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_workflow_execution_plan_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_execution_plan_json_serializer
	$(BUILD_DIR)/test_search_timer_workflow_execution_plan_json_serializer

test-search-timer-workflow-planning-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowPlanningService.cpp \
		core/vdr/tests/test_search_timer_workflow_planning_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_planning_service
	$(BUILD_DIR)/test_search_timer_workflow_planning_service

test-search-timer-workflow-execution-plan:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_workflow_execution_plan.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_execution_plan
	$(BUILD_DIR)/test_search_timer_workflow_execution_plan

test-search-timer-workflow-validation-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp \
		api/rest/tests/test_search_timer_workflow_validation_request_parser.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_validation_request_parser
	$(BUILD_DIR)/test_search_timer_workflow_validation_request_parser

test-search-timer-workflow-validation-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/src/SearchTimerWorkflowValidationResultJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_workflow_validation_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_validation_result_json_serializer
	$(BUILD_DIR)/test_search_timer_workflow_validation_result_json_serializer

test-search-timer-workflow-validation-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerWorkflowValidationService.cpp \
		core/vdr/tests/test_search_timer_workflow_validation_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_validation_service
	$(BUILD_DIR)/test_search_timer_workflow_validation_service

test-search-timer-workflow-request:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_workflow_request.cpp \
		-o $(BUILD_DIR)/test_search_timer_workflow_request
	$(BUILD_DIR)/test_search_timer_workflow_request

test-search-timer-query:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_query.cpp \
		-o $(BUILD_DIR)/test_search_timer_query
	$(BUILD_DIR)/test_search_timer_query
test-search-timer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer.cpp \
		-o $(BUILD_DIR)/test_search_timer
	$(BUILD_DIR)/test_search_timer

test-search-timer-discovery:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_discovery.cpp \
		-o $(BUILD_DIR)/test_search_timer_discovery
	$(BUILD_DIR)/test_search_timer_discovery

test-search-timer-discovery-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerDiscoveryService.cpp \
		core/vdr/tests/test_search_timer_discovery_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_discovery_service
	$(BUILD_DIR)/test_search_timer_discovery_service

test-search-timer-discovery-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerDiscoveryJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_discovery_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_discovery_json_serializer
	$(BUILD_DIR)/test_search_timer_discovery_json_serializer

test-search-timer-discovery-static-provider:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerDiscoveryStaticProvider.cpp \
		core/vdr/tests/test_search_timer_discovery_static_provider.cpp \
		-o $(BUILD_DIR)/test_search_timer_discovery_static_provider
	$(BUILD_DIR)/test_search_timer_discovery_static_provider

test-restfulapi-search-timer-discovery-provider-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RestfulApiSearchTimerDiscoveryProvider.cpp \
		core/vdr/tests/test_restfulapi_search_timer_discovery_provider_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_search_timer_discovery_provider_contract
	$(BUILD_DIR)/test_restfulapi_search_timer_discovery_provider_contract

test-search-timer-automation-evaluation-plan:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_automation_evaluation_plan.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_evaluation_plan
	$(BUILD_DIR)/test_search_timer_automation_evaluation_plan

test-search-timer-automation-match-candidate:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_automation_match_candidate.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_match_candidate
	$(BUILD_DIR)/test_search_timer_automation_match_candidate

test-search-timer-automation-duplicate-detection:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_automation_duplicate_detection.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_duplicate_detection
	$(BUILD_DIR)/test_search_timer_automation_duplicate_detection

test-search-timer-automation-candidate-timer-proposal:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_automation_candidate_timer_proposal.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_candidate_timer_proposal
	$(BUILD_DIR)/test_search_timer_automation_candidate_timer_proposal

test-search-timer-automation-dry-run-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerAutomationDryRunResultJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_automation_dry_run_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_dry_run_result_json_serializer
	$(BUILD_DIR)/test_search_timer_automation_dry_run_result_json_serializer

test-search-timer-automation-read-only-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerAutomationReadOnlyService.cpp \
		core/vdr/tests/test_search_timer_automation_read_only_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_read_only_service
	$(BUILD_DIR)/test_search_timer_automation_read_only_service

test-search-timer-automation-daemon-scheduling-plan:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_search_timer_automation_daemon_scheduling_plan.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_daemon_scheduling_plan
	$(BUILD_DIR)/test_search_timer_automation_daemon_scheduling_plan

test-search-timer-automation-safety-review:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerAutomationSafetyReview.cpp \
		core/vdr/tests/test_search_timer_automation_safety_review.cpp \
		-o $(BUILD_DIR)/test_search_timer_automation_safety_review
	$(BUILD_DIR)/test_search_timer_automation_safety_review
test-epg-person-search-result:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_epg_person_search_result.cpp \
		-o $(BUILD_DIR)/test_epg_person_search_result
	$(BUILD_DIR)/test_epg_person_search_result

test-epg-search-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgSearchResultJsonSerializer.cpp \
		core/vdr/tests/test_epg_search_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_epg_search_result_json_serializer
	$(BUILD_DIR)/test_epg_search_result_json_serializer

test-rest-query-parameters:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/RestQueryParameters.cpp \
		api/rest/tests/test_rest_query_parameters.cpp \
		-o $(BUILD_DIR)/test_rest_query_parameters
	$(BUILD_DIR)/test_rest_query_parameters
