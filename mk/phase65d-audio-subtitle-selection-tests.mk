DAEMON_SRC += \
	api/rest/src/RecordingMediaSessionTrackSelectionRequestParser.cpp \
	api/rest/src/RecordingMediaSessionTrackSelection.cpp \
	api/rest/src/RecordingMediaSessionAudioTrackPreference.cpp \
	core/media/src/RecordingMediaTrackContract.cpp \
	core/media/src/RecordingMediaSessionAudioTrackSelection.cpp \
	core/media/src/RecordingMediaSessionTrackState.cpp

.PHONY: install-phase65d-recording-track-controls \
	test-phase65d-recording-media-track-contract \
	test-phase65d-recording-audio-track-selection-request-parser \
	test-phase65d-recording-audio-track-preference-parser \
	test-phase65d-recording-audio-track-selection-runtime \
	test-phase65d-recording-track-controls \
	test-phase65d-recording-hls-audio-replacement

install-runtime: install-phase65d-recording-track-controls

install-phase65d-recording-track-controls: install-live-remote-frontend
	cat \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js \
		web/frontend/api/recording-track-controls.js \
		> $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-tracks.js.tmp
	chmod 0644 $(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-tracks.js.tmp
	mv -f \
		$(DESTDIR)$(DATADIR)/web/frontend/api/.session-frontend-sync-tracks.js.tmp \
		$(DESTDIR)$(DATADIR)/web/frontend/api/session-frontend-sync.js

test-phase65d-recording-media-track-contract:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/RecordingMediaTrackContract.cpp \
		core/media/tests/test_recording_media_track_contract.cpp \
		-o $(BUILD_DIR)/test_phase65d_recording_media_track_contract
	$(BUILD_DIR)/test_phase65d_recording_media_track_contract

test-phase65d-recording-audio-track-selection-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include -Iapi/rest/include \
		api/rest/src/RecordingMediaSessionRequestParser.cpp \
		api/rest/src/RecordingMediaSessionTrackSelectionRequestParser.cpp \
		api/rest/tests/test_recording_media_session_audio_track_selection_request_parser.cpp \
		-o $(BUILD_DIR)/test_phase65d_recording_audio_track_selection_request_parser
	$(BUILD_DIR)/test_phase65d_recording_audio_track_selection_request_parser

test-phase65d-recording-audio-track-preference-parser:
	$(BUILD_CXX) $(CXXFLAGS) -Iapi/rest/include \
		api/rest/src/RecordingMediaSessionAudioTrackPreference.cpp \
		api/rest/tests/test_recording_media_session_audio_track_preference.cpp \
		-o $(BUILD_DIR)/test_phase65d_recording_audio_track_preference_parser
	$(BUILD_DIR)/test_phase65d_recording_audio_track_preference_parser

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
		core/media/src/RecordingMediaSessionTrackState.cpp \
		core/media/tests/test_recording_media_session_audio_track_selection.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65d_recording_audio_track_selection_runtime
	$(BUILD_DIR)/test_phase65d_recording_audio_track_selection_runtime

test-phase65d-recording-track-controls:
	node --check web/frontend/api/recording-track-controls.js
	node web/frontend/tests/test_phase65d_recording_track_controls.js

test-phase65d-recording-hls-audio-replacement:
	node --check web/frontend/api/recording-fallback-controls.js
	node web/frontend/tests/test_phase65d_recording_hls_audio_replacement.js

test-fast: test-phase65d-recording-media-track-contract test-phase65d-recording-audio-track-selection-request-parser test-phase65d-recording-audio-track-preference-parser test-phase65d-recording-audio-track-selection-runtime
test-frontend-i18n: test-phase65d-recording-track-controls test-phase65d-recording-hls-audio-replacement
