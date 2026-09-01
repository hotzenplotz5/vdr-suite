include mk/test-groups.mk

.PHONY: test-backend-node test-backend-registry test-backend-registry-service test-backend-registry-json-serializer test-search-timer-preview-epg-cache test-vdr-snapshot-read-service test-vdr-snapshot-read-service-searchtimer-preview-epg-cache test-api-router-searchtimer-preview-epg-cache test-json-string-decoder test-searchtimer-discovery-runtime-wiring test-daemon-runtime-modularity test-daemon-runtime-shutdown-resets test-http-listener-bind-failure-handling test-http-listener-partial-request-timeout test-http-listener-image-write-isolation test-real-vdr-acceptance-manifest test-phase-map-coverage test-github-update-safety-handoff test-recording-mutation-safety-policy test-frontend-contracts test-frontend-i18n


test-json-string-decoder:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/tests/test_json_string_decoder.cpp \
		-o $(BUILD_DIR)/test_json_string_decoder
	$(BUILD_DIR)/test_json_string_decoder


test-searchtimer-discovery-runtime-wiring:
	python3 tools/check_searchtimer_discovery_runtime_wiring.py


test-daemon-runtime-modularity:
	python3 tools/check_daemon_runtime_modularity.py


test-daemon-runtime-shutdown-resets:
	python3 tools/check_daemon_runtime_shutdown_resets.py
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		core/daemon/tests/test_daemon_sqlite_shutdown_cancellation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_daemon_sqlite_shutdown_cancellation
	$(BUILD_DIR)/test_daemon_sqlite_shutdown_cancellation


test-http-listener-bind-failure-handling:
	python3 tools/check_http_listener_bind_failure_handling.py


test-http-listener-partial-request-timeout:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		core/http/src/SimpleHttpListener.cpp \
		core/http/tests/test_simple_http_listener_partial_request_timeout.cpp \
		-o $(BUILD_DIR)/test_simple_http_listener_partial_request_timeout
	$(BUILD_DIR)/test_simple_http_listener_partial_request_timeout


test-http-listener-image-write-isolation:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		core/http/src/SimpleHttpListener.cpp \
		core/http/tests/test_simple_http_listener_image_write_isolation.cpp \
		-o $(BUILD_DIR)/test_simple_http_listener_image_write_isolation
	$(BUILD_DIR)/test_simple_http_listener_image_write_isolation


test-real-vdr-acceptance-manifest:
	python3 tools/real-vdr-acceptance/runner.py --validate-only


test-phase-map-coverage:
	python3 tools/check_phase_map_coverage.py


test-github-update-safety-handoff:
	python3 tools/check_github_update_safety_handoff.py


test-recording-mutation-safety-policy:
	python3 tools/check_recording_mutation_safety_policy.py


test-backend-node:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/tests/test_backend_node.cpp \
		-o $(BUILD_DIR)/test_backend_node
	$(BUILD_DIR)/test_backend_node


test-backend-registry:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/tests/test_backend_registry.cpp \
		-o $(BUILD_DIR)/test_backend_registry
	$(BUILD_DIR)/test_backend_registry


test-backend-registry-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/tests/test_backend_registry_service.cpp \
		-o $(BUILD_DIR)/test_backend_registry_service
	$(BUILD_DIR)/test_backend_registry_service


test-backend-registry-json-serializer:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryJsonSerializer.cpp \
		core/vdr/tests/test_backend_registry_json_serializer.cpp \
		-o $(BUILD_DIR)/test_backend_registry_json_serializer
	$(BUILD_DIR)/test_backend_registry_json_serializer


test-search-timer-preview-epg-cache:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SearchTimerPreviewEpgCache.cpp \
		core/vdr/tests/test_search_timer_preview_epg_cache.cpp \
		-o $(BUILD_DIR)/test_search_timer_preview_epg_cache
	$(BUILD_DIR)/test_search_timer_preview_epg_cache


test-vdr-snapshot-read-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SnapshotCache.cpp \
		core/vdr/src/SnapshotCacheService.cpp \
		core/vdr/src/SnapshotAccessService.cpp \
		core/vdr/src/SearchTimerPreviewEpgCache.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		core/vdr/tests/test_vdr_snapshot_read_service.cpp \
		-o $(BUILD_DIR)/test_vdr_snapshot_read_service
	$(BUILD_DIR)/test_vdr_snapshot_read_service


test-vdr-snapshot-read-service-searchtimer-preview-epg-cache:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/SnapshotCache.cpp \
		core/vdr/src/SnapshotCacheService.cpp \
		core/vdr/src/SnapshotAccessService.cpp \
		core/vdr/src/SearchTimerPreviewEpgCache.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		core/vdr/tests/test_vdr_snapshot_read_service_searchtimer_preview_epg_cache.cpp \
		-o $(BUILD_DIR)/test_vdr_snapshot_read_service_searchtimer_preview_epg_cache
	$(BUILD_DIR)/test_vdr_snapshot_read_service_searchtimer_preview_epg_cache


test-api-router-searchtimer-preview-epg-cache:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(VDR_SRC) \
		core/vdr/src/VdrRecordingCacheRepository.cpp \
		core/vdr/src/SearchTimerResultJsonSerializer.cpp \
		core/vdr/src/SearchTimerService.cpp \
		api/rest/src/SearchTimerCreateRequestParser.cpp \
		api/rest/src/SearchTimerUpdateRequestParser.cpp \
		api/rest/src/SearchTimerDeleteRequestParser.cpp \
		api/rest/src/SearchTimerWorkflowValidationRequestParser.cpp \
		api/rest/src/SearchTimerController.cpp \
		api/rest/tests/test_api_router_searchtimer_preview_epg_cache.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_api_router_searchtimer_preview_epg_cache
	$(BUILD_DIR)/test_api_router_searchtimer_preview_epg_cache

.PHONY: test-frontend-contracts
test-frontend-contracts: test-frontend-i18n
	python3 tools/check_frontend_ownership_contracts.py
	python3 tools/check_frontend_platform_runtime_context.py
	node web/frontend/tests/test_phase66_home_shell_composition.js
	node web/frontend/tests/test_phase66_live_tv_hero.js
	node web/frontend/tests/test_phase66_home_live_hero_module_visibility.js
	node web/frontend/tests/test_phase66_home_programme_actions.js
	node web/frontend/tests/test_phase66_deferred_live_preview.js
	node web/frontend/tests/test_phase66_live_preview_audio_controls.js


# Transitive runtime test source loaded by test_deferred_frontend_runtime_loader.js:
# web/frontend/tests/test_query_cache_refresh_security_runtime.js
test-frontend-i18n:
	python3 tools/check_frontend_i18n_contracts.py
	node web/frontend/tests/test_i18n_runtime.js
	node web/frontend/tests/test_timer_workflows_runtime.js
	node web/frontend/tests/test_searchtimer_workflows_runtime.js
	node web/frontend/tests/test_searchtimer_maintenance_security_runtime.js
	node web/frontend/tests/test_searchtimer_execution_security_runtime.js
	node web/frontend/tests/test_channel_day_program_runtime.js
	node web/frontend/tests/test_channel_day_navigation_runtime.js
	node web/frontend/tests/test_channel_day_program_compat_runtime.js
	node web/frontend/tests/test_deferred_frontend_runtime_loader.js
	node web/frontend/tests/test_epg_timeline_enhancements.js
	node web/frontend/tests/test_epg_timeline_deferred_install.js
	node web/frontend/tests/test_epg_metadata_detail.js
	node web/frontend/tests/test_epg_metadata_mobile_navigation.js
	node web/frontend/tests/test_epg_metadata_detail_hook.js
	python3 web/frontend/tests/test_epg_runtime_bundle_builder.py
	python3 web/frontend/tests/test_epg_metadata_runtime_bundle.py
