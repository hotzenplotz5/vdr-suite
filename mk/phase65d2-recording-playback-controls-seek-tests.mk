.PHONY: test-phase65d2-recording-media-session-seek test-phase65d2-recording-playback-controls

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
		core/media/tests/test_recording_media_session_seek.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65d2_recording_media_session_seek
	$(BUILD_DIR)/test_phase65d2_recording_media_session_seek

test-phase65d2-recording-playback-controls:
	node web/frontend/tests/test_phase65d2_recording_playback_controls.js

test-fast: test-phase65d2-recording-media-session-seek test-vdr-recording-query-service
test-frontend-i18n: test-phase65d2-recording-playback-controls
