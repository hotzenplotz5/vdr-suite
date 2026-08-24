DAEMON_SRC += \
	api/rest/src/RecordingMediaSessionTrackSelectionRequestParser.cpp \
	core/media/src/RecordingMediaTrackContract.cpp \
	core/media/src/RecordingMediaSessionAudioTrackSelection.cpp

.PHONY: test-phase65d-recording-media-track-contract test-phase65d-recording-audio-track-selection-request-parser test-phase65d-recording-audio-track-selection-runtime

test-phase65d-recording-media-track-contract:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/RecordingMediaTrackContract.cpp \
		core/media/tests/test_recording_media_track_contract.cpp \
		-o $(BUILD_DIR)/test_phase65d_recording_media_track_contract
	$(BUILD_DIR)/test_phase65d_recording_media_track_contract

test-phase65d-recording-audio-track-selection-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include -Iapi/rest/include \
		api/rest/src/RecordingMediaSessionTrackSelectionRequestParser.cpp \
		api/rest/tests/test_recording_media_session_audio_track_selection_request_parser.cpp \
		-o $(BUILD_DIR)/test_phase65d_recording_audio_track_selection_request_parser
	$(BUILD_DIR)/test_phase65d_recording_audio_track_selection_request_parser

test-phase65d-recording-audio-track-selection-runtime:
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
		core/media/src/RecordingMediaSessionAudioTrackSelection.cpp \
		core/media/tests/test_recording_media_session_audio_track_selection.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65d_recording_audio_track_selection_runtime
	$(BUILD_DIR)/test_phase65d_recording_audio_track_selection_runtime

test-fast: test-phase65d-recording-media-track-contract test-phase65d-recording-audio-track-selection-request-parser test-phase65d-recording-audio-track-selection-runtime
