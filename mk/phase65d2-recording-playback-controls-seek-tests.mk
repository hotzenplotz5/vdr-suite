DAEMON_SRC += \
	api/rest/src/RecordingMediaSessionPlaybackStatusRequestParser.cpp \
	api/rest/src/RecordingMediaSessionStartPosition.cpp \
	api/rest/src/RecordingMediaSessionCreate.cpp \
	api/rest/src/RecordingMediaSessionPlaybackStatus.cpp \
	core/media/src/RecordingMediaSessionHlsResume.cpp

.PHONY: test-phase65d2-recording-media-session-seek test-phase65d2-recording-media-session-seek-activation test-phase65d2-vdr-recording-index-updater test-phase65d2-recording-playback-status-request-parser test-phase65d2-recording-start-position-parser test-phase65d2-hls-resume-command test-phase65d2-recording-playback-controls test-phase65d2-recording-stop-restart test-phase65d2-recording-stop-resume-choice test-phase65d2-recording-restart-choice-real-loader test-phase65d2-recording-fallback-controls test-phase65d2-recording-fallback-resume-choice test-phase65d2-recording-time-input-mask

test-phase65d2-recording-media-session-seek:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/media/include -Icore/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/media/src/MediaTranscodePolicy.cpp \
		core/media/src/FfmpegHlsCommandBuilder.cpp \
		core/media/src/MediaProcessRunner.cpp \
		core/media/src/MediaSessionWorkspace.cpp \
		core/media/src/MediaSessionRepository.cpp \
		core/media/src/MediaSessionIssuanceService.cpp \
		core/media/src/RecordingSourceFingerprint.cpp \
		core/media/src/SegmentedRecordingByteSource.cpp \
		core/media/src/RecordingDirectSourceRegistry.cpp \
		core/media/src/RecordingMediaSessionRuntime.cpp \
		core/media/src/RecordingMediaSessionSeekTimeline.cpp \
		core/media/tests/test_recording_media_session_seek.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65d2_recording_media_session_seek
	$(BUILD_DIR)/test_phase65d2_recording_media_session_seek

test-phase65d2-recording-media-session-seek-activation:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/media/include -Icore/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/media/src/MediaTranscodePolicy.cpp \
		core/media/src/FfmpegHlsCommandBuilder.cpp \
		core/media/src/MediaProcessRunner.cpp \
		core/media/src/MediaSessionWorkspace.cpp \
		core/media/src/MediaSessionRepository.cpp \
		core/media/src/MediaSessionIssuanceService.cpp \
		core/media/src/RecordingSourceFingerprint.cpp \
		core/media/src/SegmentedRecordingByteSource.cpp \
		core/media/src/RecordingDirectSourceRegistry.cpp \
		core/media/src/RecordingMediaSessionRuntime.cpp \
		core/media/src/RecordingMediaSessionSeekTimeline.cpp \
		core/media/tests/test_recording_media_session_seek_activation.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65d2_recording_media_session_seek_activation
	$(BUILD_DIR)/test_phase65d2_recording_media_session_seek_activation

test-phase65d2-vdr-recording-index-updater:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/vdr/include \
		core/vdr/src/VdrRecordingIndexUpdater.cpp \
		core/vdr/tests/test_vdr_recording_index_updater.cpp \
		-o $(BUILD_DIR)/test_phase65d2_vdr_recording_index_updater
	$(BUILD_DIR)/test_phase65d2_vdr_recording_index_updater

test-phase65d2-recording-playback-status-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include -Iapi/rest/include \
		api/rest/src/RecordingMediaSessionPlaybackStatusRequestParser.cpp \
		api/rest/tests/test_recording_media_session_playback_status_request_parser.cpp \
		-o $(BUILD_DIR)/test_phase65d2_recording_playback_status_request_parser
	$(BUILD_DIR)/test_phase65d2_recording_playback_status_request_parser

test-phase65d2-recording-start-position-parser:
	$(BUILD_CXX) $(CXXFLAGS) -Iapi/rest/include \
		api/rest/src/RecordingMediaSessionStartPosition.cpp \
		api/rest/tests/test_recording_media_session_start_position.cpp \
		-o $(BUILD_DIR)/test_phase65d2_recording_start_position_parser
	$(BUILD_DIR)/test_phase65d2_recording_start_position_parser

test-phase65d2-hls-resume-command:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/FfmpegHlsCommandBuilder.cpp \
		core/media/tests/test_phase65d2_hls_resume_command.cpp \
		-o $(BUILD_DIR)/test_phase65d2_hls_resume_command
	$(BUILD_DIR)/test_phase65d2_hls_resume_command

test-phase65d2-recording-playback-controls:
	node web/frontend/tests/test_phase65d2_recording_playback_controls.js

test-phase65d2-recording-stop-restart:
	node web/frontend/tests/test_phase65d2_recording_stop_restart.js

test-phase65d2-recording-stop-resume-choice:
	node web/frontend/tests/test_phase65d2_recording_stop_resume_choice.js

test-phase65d2-recording-restart-choice-real-loader:
	node web/frontend/tests/test_phase65d2_recording_restart_choice_real_loader.js

test-phase65d2-recording-fallback-controls:
	node --check web/frontend/api/recording-fallback-controls.js
	node web/frontend/tests/test_phase65d2_recording_fallback_controls.js
	node web/frontend/tests/test_phase65d2_recording_fallback_restart_binding.js

test-phase65d2-recording-fallback-resume-choice:
	node web/frontend/tests/test_phase65d2_recording_fallback_resume_choice.js

test-phase65d2-recording-time-input-mask:
	node --check web/frontend/api/recording-time-input-mask.js
	node web/frontend/tests/test_phase65d2_recording_time_input_mask.js

test-fast: test-phase65d2-recording-media-session-seek test-phase65d2-recording-media-session-seek-activation test-phase65d2-vdr-recording-index-updater test-phase65d2-recording-playback-status-request-parser test-phase65d2-recording-start-position-parser test-phase65d2-hls-resume-command test-vdr-recording-query-service test-vdr-recording-cache-repository
test-frontend-i18n: test-phase65d2-recording-playback-controls test-phase65d2-recording-stop-restart test-phase65d2-recording-stop-resume-choice test-phase65d2-recording-restart-choice-real-loader test-phase65d2-recording-fallback-controls test-phase65d2-recording-fallback-resume-choice test-phase65d2-recording-time-input-mask