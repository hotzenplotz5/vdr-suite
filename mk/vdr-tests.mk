.PHONY: test-fast

test-fast: test-capability-resolver test-vdr-capability-set test-runtime-diagnostics test-http-request test-http-response test-backend-node test-backend-registry test-backend-registry-service test-backend-registry-json-serializer test-vdr-config test-snapshot-access-service test-vdr-domain-objects

test-vdr-config:
	$(CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/tests/test_vdr_config.cpp \
		-o /tmp/test_vdr_config
	/tmp/test_vdr_config

test-external-vdr-adapter:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_external_vdr_adapter.cpp \
		-o /tmp/test_external_vdr_adapter
	/tmp/test_external_vdr_adapter

test-mock-vdr-adapter:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_mock_vdr_adapter.cpp \
		-o /tmp/test_mock_vdr_adapter
	/tmp/test_mock_vdr_adapter

test-vdr-adapter-factory:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_adapter_factory.cpp \
		-o /tmp/test_vdr_adapter_factory
	/tmp/test_vdr_adapter_factory

test-vdr-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_service.cpp \
		-o /tmp/test_vdr_service
	/tmp/test_vdr_service

test-vdr-overview-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_overview_service.cpp \
		-o /tmp/test_vdr_overview_service
	/tmp/test_vdr_overview_service

test-vdr-overview-json-serializer:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_overview_json_serializer.cpp \
		-o /tmp/test_vdr_overview_json_serializer
	/tmp/test_vdr_overview_json_serializer

test-vdr-snapshot-read-json-serializer:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_snapshot_read_json_serializer.cpp \
		-o /tmp/test_vdr_snapshot_read_json_serializer
	/tmp/test_vdr_snapshot_read_json_serializer

test-vdr-snapshot-builder:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_snapshot_builder.cpp \
		-o /tmp/test_vdr_snapshot_builder
	/tmp/test_vdr_snapshot_builder

test-polling-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_polling_service.cpp \
		-o /tmp/test_polling_service
	/tmp/test_polling_service

test-vdr-change-state:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_change_state.cpp \
		-o /tmp/test_vdr_change_state
	/tmp/test_vdr_change_state

test-vdr-change-event:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_change_event.cpp \
		-o /tmp/test_vdr_change_event
	/tmp/test_vdr_change_event

test-change-detection-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_change_detection_service.cpp \
		-o /tmp/test_change_detection_service
	/tmp/test_change_detection_service

test-snapshot-refresh-decision-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_snapshot_refresh_decision_service.cpp \
		-o /tmp/test_snapshot_refresh_decision_service
	/tmp/test_snapshot_refresh_decision_service

test-snapshot-refresh-planner:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_snapshot_refresh_planner.cpp \
		-o /tmp/test_snapshot_refresh_planner
	/tmp/test_snapshot_refresh_planner

test-snapshot-update-plan:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_snapshot_update_plan.cpp \
		-o /tmp/test_snapshot_update_plan
	/tmp/test_snapshot_update_plan

test-snapshot-cache:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_snapshot_cache.cpp \
		-o /tmp/test_snapshot_cache
	/tmp/test_snapshot_cache

test-snapshot-cache-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_snapshot_cache_service.cpp \
		-o /tmp/test_snapshot_cache_service
	/tmp/test_snapshot_cache_service

test-snapshot-change-feed:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_snapshot_change_feed.cpp \
		-o /tmp/test_snapshot_change_feed
	/tmp/test_snapshot_change_feed

test-snapshot-change-feed-json-serializer:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_snapshot_change_feed_json_serializer.cpp \
		-o /tmp/test_snapshot_change_feed_json_serializer
	/tmp/test_snapshot_change_feed_json_serializer

test-restful-api-status-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_status_mapper.cpp \
		-o /tmp/test_restful_api_status_mapper
	/tmp/test_restful_api_status_mapper

test-restful-api-event-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_event_mapper.cpp \
		-o /tmp/test_restful_api_event_mapper
	/tmp/test_restful_api_event_mapper

test-restful-api-channel-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_channel_mapper.cpp \
		-o /tmp/test_restful_api_channel_mapper
	/tmp/test_restful_api_channel_mapper

test-restful-api-recording-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_recording_mapper.cpp \
		-o /tmp/test_restful_api_recording_mapper
	/tmp/test_restful_api_recording_mapper

test-restful-api-timer-mapper:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_restful_api_timer_mapper.cpp \
		-o /tmp/test_restful_api_timer_mapper
	/tmp/test_restful_api_timer_mapper

test-restful-api-vdr-adapter:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(HTTP_SRC) \
		core/vdr/tests/test_restful_api_vdr_adapter.cpp \
		-o /tmp/test_restful_api_vdr_adapter
	/tmp/test_restful_api_vdr_adapter

test-restful-api-change-state-adapter:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(HTTP_SRC) \
		core/vdr/tests/test_restful_api_change_state_adapter.cpp \
		-o /tmp/test_restful_api_change_state_adapter
	/tmp/test_restful_api_change_state_adapter

test-vdr-domain-objects:
	$(CXX) $(CXXFLAGS) \
		core/vdr/tests/test_vdr_domain_objects.cpp \
		-o /tmp/test_vdr_domain_objects
	/tmp/test_vdr_domain_objects


test-snapshot-access-service:
	$(CXX) $(CXXFLAGS) \
		core/vdr/src/SnapshotCache.cpp \
		core/vdr/src/SnapshotCacheService.cpp \
		core/vdr/src/SnapshotAccessService.cpp \
		core/vdr/tests/test_snapshot_access_service.cpp \
		-o /tmp/test_snapshot_access_service
	/tmp/test_snapshot_access_service

test-vdr-snapshot-read-service:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_snapshot_read_service.cpp \
		-o /tmp/test_vdr_snapshot_read_service
	/tmp/test_vdr_snapshot_read_service

test-local-restfulapi-integration:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/http/src/BasicHttpClient.cpp \
		core/http/src/MockHttpClient.cpp \
		core/vdr/tests/test_local_restfulapi_integration.cpp \
		-o /tmp/test_local_restfulapi_integration
	/tmp/test_local_restfulapi_integration

test-local-snapshot-runtime-integration:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/http/src/BasicHttpClient.cpp \
		core/http/src/MockHttpClient.cpp \
		core/vdr/tests/test_local_snapshot_runtime_integration.cpp \
		-o /tmp/test_local_snapshot_runtime_integration
	/tmp/test_local_snapshot_runtime_integration

test-local-partial-refresh-validation:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/http/src/BasicHttpClient.cpp \
		core/http/src/MockHttpClient.cpp \
		core/vdr/tests/test_local_partial_refresh_validation.cpp \
		-o /tmp/test_local_partial_refresh_validation
	/tmp/test_local_partial_refresh_validation
