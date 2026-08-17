.PHONY: test-phase65-media-capability-negotiation test-phase65-local-recording-source test-phase65-ffmpeg-hls-command-builder

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

test-phase65-ffmpeg-hls-command-builder:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/FfmpegHlsCommandBuilder.cpp \
		core/media/tests/test_ffmpeg_hls_command_builder.cpp \
		-o $(BUILD_DIR)/test_phase65_ffmpeg_hls_command_builder
	$(BUILD_DIR)/test_phase65_ffmpeg_hls_command_builder

# test-ci-fast already owns test-fast in the canonical group file. Extend that
# existing public group instead of defining a second canonical group target.
test-fast: test-phase65-media-capability-negotiation test-phase65-local-recording-source test-phase65-ffmpeg-hls-command-builder
