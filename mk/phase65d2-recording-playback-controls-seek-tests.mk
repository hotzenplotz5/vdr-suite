DAEMON_SRC += api/rest/src/RecordingMediaSessionPlaybackStatusRequestParser.cpp

.PHONY: test-phase65d2-recording-media-session-seek test-phase65d2-recording-media-session-seek-activation test-phase65d2-vdr-recording-index-updater test-phase65d2-recording-playback-status-request-parser test-phase65d2-recording-playback-controls

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

test-phase65d2-recording-playback-controls:
	node web/frontend/tests/test_phase65d2_recording_playback_controls.js

test-fast: test-phase65d2-recording-media-session-seek test-phase65d2-recording-media-session-seek-activation test-phase65d2-vdr-recording-index-updater test-phase65d2-recording-playback-status-request-parser test-vdr-recording-query-service
test-frontend-i18n: test-phase65d2-recording-playback-controls
