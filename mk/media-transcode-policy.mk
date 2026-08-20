.PHONY: install-media-transcode-calibrator install-media-transcode-settings-frontend test-phase65-media-transcode-calibrator-install test-phase65-media-transcode-calibrator-policy test-phase65-media-transcode-backend-settings test-phase65-media-transcode-settings-api test-phase65-media-transcode-settings-security test-phase65-media-transcode-settings-frontend test-phase65-media-transcode-settings-install

# Keep the calibrator available with install-runtime as an operator tool. The
# daemon itself never benchmarks during playback or startup. The managed
# backend setting is a frontend/API control plane over the same typed policy;
# it never accepts arbitrary FFmpeg arguments or hardware device paths.
install-runtime: install-media-transcode-calibrator install-media-transcode-settings-frontend

install-media-transcode-calibrator:
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 tools/vdr_suite_media_calibrate.py \
		$(DESTDIR)$(BINDIR)/vdr-suite-media-calibrate

install-media-transcode-settings-frontend:
	$(INSTALL) -d $(DESTDIR)$(DATADIR)/web/frontend
	$(INSTALL) -m 0644 web/frontend/settings-media-transcode.js \
		$(DESTDIR)$(DATADIR)/web/frontend/settings-media-transcode.js

test-phase65-media-transcode-calibrator-policy:
	python3 tools/tests/test_vdr_suite_media_calibrate.py

test-phase65-media-transcode-calibrator-install: test-phase65-media-transcode-calibrator-policy
	python3 -m py_compile tools/vdr_suite_media_calibrate.py
	python3 -c 'import shutil; shutil.rmtree("/tmp/vdr-suite-media-calibrator-install", ignore_errors=True)'
	$(MAKE) install-media-transcode-calibrator \
		DESTDIR=/tmp/vdr-suite-media-calibrator-install PREFIX=/usr
	test -x /tmp/vdr-suite-media-calibrator-install/usr/bin/vdr-suite-media-calibrate
	/tmp/vdr-suite-media-calibrator-install/usr/bin/vdr-suite-media-calibrate \
		--help >/dev/null
	python3 -c 'import shutil; shutil.rmtree("/tmp/vdr-suite-media-calibrator-install", ignore_errors=True)'

test-phase65-media-transcode-backend-settings:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		-Icore/daemon/include -Icore/media/include -Icore/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/media/src/MediaTranscodePolicy.cpp \
		core/media/src/MediaProcessRunner.cpp \
		core/daemon/src/MediaTranscodeBackendSettingsService.cpp \
		core/daemon/tests/test_media_transcode_backend_settings_service.cpp \
		$(LDFLAGS) -lsqlite3 \
		-o $(BUILD_DIR)/test_phase65_media_transcode_backend_settings
	$(BUILD_DIR)/test_phase65_media_transcode_backend_settings

test-phase65-media-transcode-settings-api:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		-Iapi/rest/include -Icore/daemon/include -Icore/media/include -Icore/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/media/src/MediaTranscodePolicy.cpp \
		core/media/src/MediaProcessRunner.cpp \
		core/daemon/src/MediaTranscodeBackendSettingsService.cpp \
		api/rest/src/MediaTranscodeSettingsApiRuntime.cpp \
		api/rest/tests/test_media_transcode_settings_api_runtime.cpp \
		$(LDFLAGS) -lsqlite3 \
		-o $(BUILD_DIR)/test_phase65_media_transcode_settings_api
	$(BUILD_DIR)/test_phase65_media_transcode_settings_api

test-phase65-media-transcode-settings-security:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		$(SECURITY_SRC) \
		core/security/tests/test_media_transcode_settings_route_scope_security.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_media_transcode_settings_route_scope_security
	$(BUILD_DIR)/test_media_transcode_settings_route_scope_security

test-phase65-media-transcode-settings-frontend:
	node web/frontend/tests/test_media_transcode_settings_runtime.js

test-phase65-media-transcode-settings-install: test-phase65-media-transcode-settings-frontend
	python3 -c 'import shutil; shutil.rmtree("/tmp/vdr-suite-media-settings-install", ignore_errors=True)'
	$(MAKE) install-media-transcode-settings-frontend \
		DESTDIR=/tmp/vdr-suite-media-settings-install PREFIX=/usr
	test -f /tmp/vdr-suite-media-settings-install/usr/share/vdr-suite/web/frontend/settings-media-transcode.js
	grep -F 'VdrSuiteMediaTranscodeSettings' \
		/tmp/vdr-suite-media-settings-install/usr/share/vdr-suite/web/frontend/settings-media-transcode.js >/dev/null
	python3 -c 'import shutil; shutil.rmtree("/tmp/vdr-suite-media-settings-install", ignore_errors=True)'

# These are the coherent regression owners for the managed encoder policy.
test-fast: test-phase65-media-transcode-backend-settings test-phase65-media-transcode-settings-api test-phase65-media-transcode-settings-security

test-frontend-i18n: test-phase65-media-transcode-settings-frontend

test-install-staging: test-phase65-media-transcode-settings-install

# LiveMediaSessionRuntime uses the same settings resolver as the daemon. Its
# focused test target therefore links the settings implementation as a test
# dependency without widening the production source ownership graph.
test-phase65-live-media-session-runtime: CXXFLAGS += -Iapi/rest/include -Icore/daemon/include
test-phase65-live-media-session-runtime: AGENT_CONTROL_PLANE_DOMAIN_SRC += \
	core/daemon/src/MediaTranscodeBackendSettingsService.cpp \
	api/rest/src/MediaTranscodeSettingsApiRuntime.cpp
