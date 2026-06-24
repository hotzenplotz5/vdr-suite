.PHONY: test-vdr test-backend-node test-backend-registry test-backend-registry-service test-backend-registry-json-serializer

test-vdr: \
	test-backend-node \
	test-backend-registry \
	test-vdr-config \
	test-external-vdr-adapter \
	test-vdr-adapter-factory \
	test-vdr-service \
	test-vdr-overview-service \
	test-vdr-overview-json-serializer \
	test-vdr-snapshot-builder \
	test-polling-service \
	test-vdr-change-state \
	test-vdr-change-event \
	test-change-detection-service \
	test-snapshot-refresh-decision-service \
	test-snapshot-refresh-planner \
	test-snapshot-update-plan \
	test-snapshot-cache \
	test-snapshot-cache-service \
	test-snapshot-change-feed \
	test-snapshot-change-feed-json-serializer \
	test-snapshot-access-service \
	test-mock-vdr-adapter \
	test-restful-api-status-mapper \
	test-restful-api-event-mapper \
	test-restful-api-channel-mapper \
	test-restful-api-recording-mapper \
	test-restful-api-timer-mapper \
	test-vdr-timer-operation-request \
	test-vdr-timer-action-result \
	test-vdr-timer-action-result-json-serializer \
	test-vdr-timer-action-executor-interface \
	test-mock-vdr-timer-action-executor \
	test-vdr-timer-action-service \
	test-restful-api-vdr-timer-action-request-builder \
	test-restful-api-vdr-timer-action-executor \
	test-vdr-timer-action-request-parser \
	test-vdr-timer-action-controller \
	test-vdr-timer-action-executor-adapter-registry \
	test-restful-api-vdr-timer-action-executor-adapter \
	test-vdr-timer-action-execution-service \
	test-restful-api-vdr-adapter \
	test-restful-api-change-state-adapter \
	test-person-query-matcher \
		test-person-query-result-json-serializer \\
		test-person-search-service \\
		test-recording-person-search-result \\
	test-recording-person-search-service \\
	test-recording-person-search-result-json-serializer \\
	test-recording-person-search-controller \\
		test-person-query-controller \\
		test-genre-classification \
	test-genre-resolver \
	test-genre-resolution-json-serializer \
	test-canonical-genre-registry \
	test-genre-localization \
	test-epg-search-request \
	test-epg-search-matcher \
	test-epg-search-result \
	test-search-timer \
	test-search-timer-workflow-execution-result-json-serializer \
	test-search-timer-workflow-command-dispatch-service \
	test-search-timer-workflow-real-execution-policy \
	test-search-timer-workflow-guarded-executor-invocation \
	test-search-timer-workflow-command-request-mapper \
	test-search-timer-workflow-execution-service \
	test-search-timer-workflow-execution-plan-json-serializer \
	test-search-timer-workflow-planning-service \
	test-search-timer-workflow-execution-plan \
	test-search-timer-workflow-validation-request-parser \
	test-search-timer-workflow-validation-result-json-serializer \
	test-search-timer-workflow-validation-service \
	test-search-timer-workflow-request \
	test-search-timer-query \
	test-search-timer-result \
	test-search-timer-service-interface \
	test-epg-person-search-result \
	test-epg-search-result-json-serializer \
	test-epg-search-service \
	test-vdr-domain-objects

test-backend-node:
	$(CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/tests/test_backend_node.cpp \
		-o /tmp/test_backend_node
	/tmp/test_backend_node

test-backend-registry:
	$(CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/tests/test_backend_registry.cpp \
		-o /tmp/test_backend_registry
	/tmp/test_backend_registry


test-backend-registry-service:
	$(CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/tests/test_backend_registry_service.cpp \
		-o /tmp/test_backend_registry_service
	/tmp/test_backend_registry_service


test-backend-registry-json-serializer:
	$(CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistryJsonSerializer.cpp \
		core/vdr/tests/test_backend_registry_json_serializer.cpp \
		-o /tmp/test_backend_registry_json_serializer
	/tmp/test_backend_registry_json_serializer
