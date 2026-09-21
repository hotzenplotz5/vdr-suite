test-backend-polling-coordinator:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_backend_polling_coordinator.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_polling_coordinator
	$(BUILD_DIR)/test_backend_polling_coordinator

test-vdr-config:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/tests/test_vdr_config.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_config
	$(BUILD_DIR)/test_vdr_config

test-external-vdr-adapter:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_external_vdr_adapter.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_external_vdr_adapter
	$(BUILD_DIR)/test_external_vdr_adapter

test-mock-vdr-adapter:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_mock_vdr_adapter.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_mock_vdr_adapter
	$(BUILD_DIR)/test_mock_vdr_adapter

test-vdr-adapter-factory:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_adapter_factory.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_adapter_factory
	$(BUILD_DIR)/test_vdr_adapter_factory

test-vdr-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_service
	$(BUILD_DIR)/test_vdr_service

test-vdr-overview-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_overview_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_overview_service
	$(BUILD_DIR)/test_vdr_overview_service

test-vdr-overview-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_overview_json_serializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_overview_json_serializer
	$(BUILD_DIR)/test_vdr_overview_json_serializer

test-vdr-snapshot-read-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_snapshot_read_json_serializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_snapshot_read_json_serializer
	$(BUILD_DIR)/test_vdr_snapshot_read_json_serializer

test-vdr-snapshot-builder:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_snapshot_builder.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_snapshot_builder
	$(BUILD_DIR)/test_vdr_snapshot_builder

test-polling-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_polling_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_polling_service
	$(BUILD_DIR)/test_polling_service

test-vdr-change-state:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_change_state.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_change_state
	$(BUILD_DIR)/test_vdr_change_state

test-vdr-change-event:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_vdr_change_event.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_vdr_change_event
	$(BUILD_DIR)/test_vdr_change_event

test-change-detection-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_change_detection_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_change_detection_service
	$(BUILD_DIR)/test_change_detection_service

test-snapshot-refresh-decision-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_snapshot_refresh_decision_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_refresh_decision_service
	$(BUILD_DIR)/test_snapshot_refresh_decision_service

test-snapshot-refresh-planner:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_snapshot_refresh_planner.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_refresh_planner
	$(BUILD_DIR)/test_snapshot_refresh_planner

test-snapshot-update-plan:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_snapshot_update_plan.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_update_plan
	$(BUILD_DIR)/test_snapshot_update_plan

test-snapshot-cache:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_snapshot_cache.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_cache
	$(BUILD_DIR)/test_snapshot_cache

test-snapshot-cache-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_snapshot_cache_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_cache_service
	$(BUILD_DIR)/test_snapshot_cache_service

test-snapshot-change-feed:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_snapshot_change_feed.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_change_feed
	$(BUILD_DIR)/test_snapshot_change_feed

test-snapshot-change-feed-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_snapshot_change_feed_json_serializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_snapshot_change_feed_json_serializer
	$(BUILD_DIR)/test_snapshot_change_feed_json_serializer

test-live-update-event:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_live_update_event.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_live_update_event
	$(BUILD_DIR)/test_live_update_event

test-live-update-event-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_live_update_event_json_serializer.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_live_update_event_json_serializer
	$(BUILD_DIR)/test_live_update_event_json_serializer

test-live-transport-interface:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_live_transport_interface.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_live_transport_interface
	$(BUILD_DIR)/test_live_transport_interface

test-test-live-transport:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_test_live_transport.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_test_live_transport
	$(BUILD_DIR)/test_test_live_transport

test-live-transport-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_live_transport_service.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_live_transport_service
	$(BUILD_DIR)/test_live_transport_service

test-live-transport-factory:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_live_transport_factory.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_live_transport_factory
	$(BUILD_DIR)/test_live_transport_factory

test-sse-live-transport:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_sse_live_transport.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_sse_live_transport
	$(BUILD_DIR)/test_sse_live_transport

test-restful-api-status-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_restful_api_status_mapper.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_restful_api_status_mapper
	$(BUILD_DIR)/test_restful_api_status_mapper

test-restful-api-event-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_restful_api_event_mapper.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_restful_api_event_mapper
	$(BUILD_DIR)/test_restful_api_event_mapper

test-restful-api-channel-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_restful_api_channel_mapper.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_restful_api_channel_mapper
	$(BUILD_DIR)/test_restful_api_channel_mapper

test-restful-api-recording-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/vdr/src/RestfulApiRecordingMapper.cpp \
		core/vdr/tests/test_restful_api_recording_mapper.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_restful_api_recording_mapper
	$(BUILD_DIR)/test_restful_api_recording_mapper

test-restful-api-timer-mapper:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/tests/test_restful_api_timer_mapper.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_restful_api_timer_mapper
	$(BUILD_DIR)/test_restful_api_timer_mapper

