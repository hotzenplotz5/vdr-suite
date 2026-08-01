.PHONY: install-live-remote-frontend test-live-remote \
	test-remote-action-domain \
	test-restfulapi-remote-action-executor \
	test-remote-action-service \
	test-remote-action-controller \
	test-live-overlay \
	test-live-overlay-change-feed \
	test-live-remote-api-runtime \
	test-live-remote-frontend

install-runtime: install-live-remote-frontend

install-live-remote-frontend:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/modules
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend/api
	$(INSTALL) -d $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand
	$(INSTALL) -m 0644 web/frontend/api/live-remote-client-api.js $(DESTDIR)$(DATADIR)/web/frontend/api/live-remote-client-api.js
	$(INSTALL) -m 0644 web/frontend/modules/remote.js $(DESTDIR)$(DATADIR)/web/frontend/modules/remote.js
	$(RM) $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.svg
	$(INSTALL) -m 0644 web/frontend/assets/vdr-remote-photorealistic.png $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.png

test-live-remote: \
	test-vdr-capability-set \
	test-capability-resolver \
	test-capability-report-builder \
	test-capability-report-service \
	test-capability-controller \
	test-backend-registry-json-serializer \
	test-snapshot-change-feed \
	test-remote-action-domain \
	test-restfulapi-remote-action-executor \
	test-remote-action-service \
	test-remote-action-controller \
	test-live-overlay \
	test-live-overlay-change-feed \
	test-live-remote-api-runtime \
	test-live-remote-frontend

test-remote-action-domain:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/RemoteActionDomain.cpp \
		core/vdr/tests/test_remote_action_domain.cpp \
		-o $(BUILD_DIR)/test_remote_action_domain
	$(BUILD_DIR)/test_remote_action_domain

test-restfulapi-remote-action-executor:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/MockHttpClient.cpp \
		core/vdr/src/RemoteActionDomain.cpp \
		core/vdr/src/RestfulApiRemoteActionExecutor.cpp \
		core/vdr/tests/test_restfulapi_remote_action_executor.cpp \
		-o $(BUILD_DIR)/test_restfulapi_remote_action_executor
	$(BUILD_DIR)/test_restfulapi_remote_action_executor

test-remote-action-service:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/RemoteActionDomain.cpp \
		core/vdr/src/RemoteActionService.cpp \
		core/vdr/tests/test_remote_action_service.cpp \
		-o $(BUILD_DIR)/test_remote_action_service
	$(BUILD_DIR)/test_remote_action_service

test-remote-action-controller:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/RemoteActionDomain.cpp \
		core/vdr/src/RemoteActionService.cpp \
		api/rest/src/RemoteActionRequestParser.cpp \
		api/rest/src/RemoteActionController.cpp \
		api/rest/tests/test_remote_action_controller.cpp \
		-o $(BUILD_DIR)/test_remote_action_controller
	$(BUILD_DIR)/test_remote_action_controller

test-live-overlay:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/SearchTimerPreviewEpgCache.cpp \
		core/vdr/src/SnapshotCache.cpp \
		core/vdr/src/SnapshotCacheService.cpp \
		core/vdr/src/SnapshotAccessService.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		core/vdr/src/LiveOverlay.cpp \
		core/vdr/tests/test_live_overlay.cpp \
		-o $(BUILD_DIR)/test_live_overlay
	$(BUILD_DIR)/test_live_overlay

test-live-overlay-change-feed:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/vdr/src/VdrChangeEvent.cpp \
		core/vdr/src/SearchTimerPreviewEpgCache.cpp \
		core/vdr/src/SnapshotChangeFeedEntry.cpp \
		core/vdr/src/SnapshotChangeFeed.cpp \
		core/vdr/src/SnapshotChangeFeedService.cpp \
		core/vdr/tests/test_live_overlay_change_feed.cpp \
		-o $(BUILD_DIR)/test_live_overlay_change_feed
	$(BUILD_DIR)/test_live_overlay_change_feed

test-live-remote-api-runtime:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/http/src/MockHttpClient.cpp \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/src/BackendRegistry.cpp \
		core/vdr/src/BackendRegistryService.cpp \
		core/vdr/src/RemoteActionDomain.cpp \
		core/vdr/src/RemoteActionService.cpp \
		core/vdr/src/RestfulApiRemoteActionExecutor.cpp \
		core/vdr/src/SearchTimerPreviewEpgCache.cpp \
		core/vdr/src/SnapshotCache.cpp \
		core/vdr/src/SnapshotCacheService.cpp \
		core/vdr/src/SnapshotAccessService.cpp \
		core/vdr/src/VdrSnapshotReadService.cpp \
		core/vdr/src/LiveOverlay.cpp \
		core/vdr/src/RestfulApiLiveChannelStateProvider.cpp \
		api/rest/src/RemoteActionRequestParser.cpp \
		api/rest/src/RemoteActionController.cpp \
		api/rest/src/LiveOverlayController.cpp \
		api/rest/src/LiveRemoteApiRuntime.cpp \
		api/rest/tests/test_live_remote_api_runtime.cpp \
		-o $(BUILD_DIR)/test_live_remote_api_runtime
	$(BUILD_DIR)/test_live_remote_api_runtime

test-live-remote-frontend:
	node --check web/frontend/api/live-remote-client-api.js
	node --check web/frontend/platform/deferred-runtime-loader.js
	node --check web/frontend/modules/remote.js
	node web/frontend/tests/test_remote_runtime.js
	node web/frontend/tests/test_browser_session_runtime.js
	node web/frontend/tests/test_timer_security_runtime.js
	node web/frontend/tests/test_channel_move_security_runtime.js
	node web/frontend/tests/test_recording_execution_security_runtime.js

test-frontend-contracts: test-live-remote-frontend
