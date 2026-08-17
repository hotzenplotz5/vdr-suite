.PHONY: test-phase65-media-capability-negotiation test-phase65-local-recording-source test-phase65-segmented-recording-byte-source test-phase65-ffmpeg-hls-command-builder test-phase65-ffprobe-recording-source test-phase65-media-session-workspace test-phase65-media-process-runner test-phase65-media-session-persistence

test-phase65-media-capability-negotiation:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/MediaPresentationSelector.cpp \
		core/media/tests/test_media_presentation_selector.cpp \
		-o $(BUILD_DIR)/test_phase65_media_capability_negotiation
	$(BUILD_DIR)/test_phase65_media_capability_negotiation

test-phase65-local-recording-source:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/LocalVdrRecordingSourceResolver.cpp \
		core/media/tests/test_local_vdr_recording_source_resolver.cpp \
		-o $(BUILD_DIR)/test_phase65_local_recording_source
	$(BUILD_DIR)/test_phase65_local_recording_source

test-phase65-segmented-recording-byte-source:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/SegmentedRecordingByteSource.cpp \
		core/media/tests/test_segmented_recording_byte_source.cpp \
		-o $(BUILD_DIR)/test_phase65_segmented_recording_byte_source
	$(BUILD_DIR)/test_phase65_segmented_recording_byte_source

test-phase65-ffmpeg-hls-command-builder:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/FfmpegHlsCommandBuilder.cpp \
		core/media/tests/test_ffmpeg_hls_command_builder.cpp \
		-o $(BUILD_DIR)/test_phase65_ffmpeg_hls_command_builder
	$(BUILD_DIR)/test_phase65_ffmpeg_hls_command_builder

test-phase65-ffprobe-recording-source:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/FfprobeRecordingSource.cpp \
		core/media/tests/test_ffprobe_recording_source.cpp \
		-o $(BUILD_DIR)/test_phase65_ffprobe_recording_source
	$(BUILD_DIR)/test_phase65_ffprobe_recording_source

test-phase65-media-session-workspace:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/MediaSessionWorkspace.cpp \
		core/media/tests/test_media_session_workspace.cpp \
		-o $(BUILD_DIR)/test_phase65_media_session_workspace
	$(BUILD_DIR)/test_phase65_media_session_workspace

test-phase65-media-process-runner:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/media/include \
		core/media/src/MediaProcessRunner.cpp \
		core/media/tests/test_media_process_runner.cpp \
		-o $(BUILD_DIR)/test_phase65_media_process_runner
	$(BUILD_DIR)/test_phase65_media_process_runner

test-phase65-media-session-persistence:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/media/include -Icore/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/media/src/MediaSessionRepository.cpp \
		core/media/src/MediaSessionIssuanceService.cpp \
		core/media/src/MediaAccessGrantAuthenticator.cpp \
		core/media/tests/test_media_session_persistence.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65_media_session_persistence
	$(BUILD_DIR)/test_phase65_media_session_persistence

# test-ci-fast already owns test-fast in the canonical group file. Extend that
# existing public group instead of defining a second canonical group target.
test-fast: test-phase65-media-capability-negotiation test-phase65-local-recording-source test-phase65-segmented-recording-byte-source test-phase65-ffmpeg-hls-command-builder test-phase65-ffprobe-recording-source test-phase65-media-session-workspace test-phase65-media-process-runner test-phase65-media-session-persistence
