test-epgsearch-request-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgSearchRequestMapper.cpp \
		core/vdr/tests/test_epgsearch_request_mapper.cpp \
		-o $(BUILD_DIR)/test_epgsearch_request_mapper
	$(BUILD_DIR)/test_epgsearch_request_mapper

.PHONY: test-epgsearch-request-mapper
test-epgsearch-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgSearchResultJsonSerializer.cpp \
		core/vdr/tests/test_epgsearch_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_epgsearch_result_json_serializer
	$(BUILD_DIR)/test_epgsearch_result_json_serializer

.PHONY: test-epgsearch-result-json-serializer
test-epgsearch-matcher:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgSearchMatcher.cpp \
		core/vdr/tests/test_epgsearch_matcher.cpp \
		-o $(BUILD_DIR)/test_epgsearch_matcher
	$(BUILD_DIR)/test_epgsearch_matcher

.PHONY: test-epgsearch-matcher
test-epgsearch-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgSearchMatcher.cpp \
		core/vdr/src/EpgSearchService.cpp \
		core/vdr/tests/test_epgsearch_service.cpp \
		-o $(BUILD_DIR)/test_epgsearch_service
	$(BUILD_DIR)/test_epgsearch_service

.PHONY: test-epgsearch-service
.PHONY: test-epgsearch-query
test-epgsearch-query:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_epgsearch_query.cpp \
		-o $(BUILD_DIR)/test_epgsearch_query
	$(BUILD_DIR)/test_epgsearch_query

.PHONY: test-epgsearch-native-fuzzy-runtime-capability-wiring
test-epgsearch-native-fuzzy-runtime-capability-wiring:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_runtime_capability_wiring.cpp \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_runtime_capability_wiring
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_runtime_capability_wiring

.PHONY: test-epgsearch-native-fuzzy-capability-repository test-epgsearch-native-fuzzy-capability-restore-service test-epgsearch-native-fuzzy-startup-restore-service test-epgsearch-native-fuzzy-startup-restore-diagnostics test-epgsearch-native-fuzzy-capability-freshness-policy
test-epgsearch-native-fuzzy-capability-repository:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/sqlite/src/Database.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_capability_repository.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_capability_repository
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_capability_repository

.PHONY: test-epgsearch-native-fuzzy-capability-restore-service
test-epgsearch-native-fuzzy-capability-restore-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/sqlite/src/Database.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRestoreService.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_capability_restore_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_capability_restore_service
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_capability_restore_service

.PHONY: test-epgsearch-native-fuzzy-startup-restore-service
test-epgsearch-native-fuzzy-startup-restore-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/sqlite/src/Database.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStartupRestoreService.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_startup_restore_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_startup_restore_service
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_startup_restore_service

.PHONY: test-epgsearch-native-fuzzy-startup-restore-diagnostics
test-epgsearch-native-fuzzy-startup-restore-diagnostics:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgSearchNativeFuzzyStartupRestoreDiagnostics.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStartupRestoreDiagnosticsJsonSerializer.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_startup_restore_diagnostics.cpp \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_startup_restore_diagnostics
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_startup_restore_diagnostics

.PHONY: test-epgsearch-native-fuzzy-capability-freshness-policy
test-epgsearch-native-fuzzy-capability-freshness-policy:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_capability_freshness_policy.cpp \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_capability_freshness_policy
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_capability_freshness_policy

.PHONY: test-epgsearch-native-fuzzy-stale-probe-administration-service
test-epgsearch-native-fuzzy-stale-probe-administration-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/sqlite/src/Database.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStaleProbeAdministrationService.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_stale_probe_administration_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_stale_probe_administration_service
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_stale_probe_administration_service

.PHONY: test-epgsearch-native-fuzzy-stale-probe-administration-controller
test-epgsearch-native-fuzzy-stale-probe-administration-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/sqlite/src/Database.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityFreshnessPolicy.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyStaleProbeAdministrationService.cpp \
		api/rest/src/EpgSearchNativeFuzzyStaleProbeAdministrationController.cpp \
		api/rest/tests/test_epgsearch_native_fuzzy_stale_probe_administration_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_stale_probe_administration_controller
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_stale_probe_administration_controller

.PHONY: test-epgsearch-native-fuzzy-operator-refresh-service
test-epgsearch-native-fuzzy-operator-refresh-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/sqlite/src/Database.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyOperatorRefreshService.cpp \
		core/vdr/tests/test_epgsearch_native_fuzzy_operator_refresh_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_operator_refresh_service
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_operator_refresh_service

.PHONY: test-epgsearch-native-fuzzy-operator-refresh-controller
test-epgsearch-native-fuzzy-operator-refresh-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/sqlite/src/Database.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerService.cpp \
		core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp \
		core/vdr/src/EpgSearchNativeFuzzyOperatorRefreshService.cpp \
		api/rest/src/EpgSearchNativeFuzzyOperatorRefreshController.cpp \
		api/rest/tests/test_epgsearch_native_fuzzy_operator_refresh_controller.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epgsearch_native_fuzzy_operator_refresh_controller
	$(BUILD_DIR)/test_epgsearch_native_fuzzy_operator_refresh_controller

.PHONY: test-fast

test-fast: test-epg-query-service-restfulapi test-epg-query-service test-epg-query-factory test-domain-refresh-policy test-backend-polling-coordinator test-capability-resolver test-vdr-capability-set test-runtime-diagnostics test-http-request test-http-response test-backend-node test-backend-registry test-backend-registry-service test-epgsearch-native-fuzzy-capability-repository test-epgsearch-native-fuzzy-runtime-capability-wiring test-backend-registry-json-serializer test-vdr-config test-snapshot-access-service test-vdr-snapshot-read-service test-vdr-domain-objects test-epgsearch-native-fuzzy-stale-probe-administration-service test-epgsearch-native-fuzzy-stale-probe-administration-controller test-epgsearch-native-fuzzy-operator-refresh-service test-epgsearch-native-fuzzy-operator-refresh-controller


test-epg-query-service-restfulapi:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/http/src/MockHttpClient.cpp \
		core/vdr/tests/test_epg_query_service_restfulapi.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_query_service_restfulapi
	$(BUILD_DIR)/test_epg_query_service_restfulapi

test-epg-query-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_epg_query_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_query_service
	$(BUILD_DIR)/test_epg_query_service

test-epg-query-factory:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_epg_query_factory.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_epg_query_factory
	$(BUILD_DIR)/test_epg_query_factory

test-search-timer-preview-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_search_timer_preview_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_search_timer_preview_service
	$(BUILD_DIR)/test_search_timer_preview_service

test-search-timer-preview-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_search_timer_preview_result_json_serializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_search_timer_preview_result_json_serializer
	$(BUILD_DIR)/test_search_timer_preview_result_json_serializer

.PHONY: test-search-timer-runtime-mutation-policy-executor
test-search-timer-runtime-mutation-policy-executor:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerRuntimeMutationPolicyExecutor.cpp \
		core/vdr/tests/test_search_timer_runtime_mutation_policy_executor.cpp \
		-o $(BUILD_DIR)/test_search_timer_runtime_mutation_policy_executor
	$(BUILD_DIR)/test_search_timer_runtime_mutation_policy_executor

test-search-timer-create-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerCreateService.cpp \
		core/vdr/tests/test_search_timer_create_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_create_service
	$(BUILD_DIR)/test_search_timer_create_service

test-search-timer-update-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerUpdateService.cpp \
		core/vdr/tests/test_search_timer_update_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_update_service
	$(BUILD_DIR)/test_search_timer_update_service

test-search-timer-delete-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerDeleteService.cpp \
		core/vdr/tests/test_search_timer_delete_service.cpp \
		-o $(BUILD_DIR)/test_search_timer_delete_service
	$(BUILD_DIR)/test_search_timer_delete_service

test-search-timer-create-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerCreateResultJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_create_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_create_result_json_serializer
	$(BUILD_DIR)/test_search_timer_create_result_json_serializer

test-search-timer-create-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/SearchTimerCreateRequestParser.cpp \
		api/rest/tests/test_search_timer_create_request_parser.cpp \
		-o $(BUILD_DIR)/test_search_timer_create_request_parser
	$(BUILD_DIR)/test_search_timer_create_request_parser

test-search-timer-update-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerUpdateResultJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_update_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_update_result_json_serializer
	$(BUILD_DIR)/test_search_timer_update_result_json_serializer

test-search-timer-delete-result-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerDeleteResultJsonSerializer.cpp \
		core/vdr/tests/test_search_timer_delete_result_json_serializer.cpp \
		-o $(BUILD_DIR)/test_search_timer_delete_result_json_serializer
	$(BUILD_DIR)/test_search_timer_delete_result_json_serializer

test-search-timer-delete-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) \
		api/rest/src/SearchTimerDeleteRequestParser.cpp \
		api/rest/tests/test_search_timer_delete_request_parser.cpp \
		-o $(BUILD_DIR)/test_search_timer_delete_request_parser
	$(BUILD_DIR)/test_search_timer_delete_request_parser

test-search-timer-update-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		api/rest/src/SearchTimerUpdateRequestParser.cpp \
		api/rest/tests/test_search_timer_update_request_parser.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_search_timer_update_request_parser
	$(BUILD_DIR)/test_search_timer_update_request_parser

