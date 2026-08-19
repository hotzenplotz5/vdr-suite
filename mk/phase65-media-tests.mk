.PHONY: test-phase65-media-capability-negotiation test-phase65-media-transcode-policy test-phase65-local-recording-source test-phase65-segmented-recording-byte-source test-phase65-ffmpeg-hls-command-builder test-phase65-ffprobe-recording-source test-phase65-media-session-workspace test-phase65-media-process-runner test-phase65-media-session-persistence test-phase65-media-hls-artifact-reader test-phase65-media-gateway-http test-phase65-api-response-headers test-phase65-recording-playback-authorization test-phase65-media-access-credential-http test-phase65-recording-media-session-request-parser test-phase65-recording-media-session-runtime test-phase65-live-provider-authority test-phase65-live-media-adaptation test-phase65-live-media-session-issuance

CXXFLAGS += -Icore/media/include

test-phase65-media-capability-negotiation:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/MediaPresentationSelector.cpp \
		core/media/tests/test_media_presentation_selector.cpp \
		-o $(BUILD_DIR)/test_phase65_media_capability_negotiation
	$(BUILD_DIR)/test_phase65_media_capability_negotiation

test-phase65-media-transcode-policy:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/MediaTranscodePolicy.cpp \
		core/media/tests/test_media_transcode_policy.cpp \
		-o $(BUILD_DIR)/test_phase65_media_transcode_policy
	$(BUILD_DIR)/test_phase65_media_transcode_policy

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
		core/media/src/MediaRouteLeaseRepository.cpp \
		core/media/src/MediaSessionIssuanceService.cpp \
		core/media/src/MediaAccessGrantAuthenticator.cpp \
		core/media/tests/test_media_session_persistence.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65_media_session_persistence
	$(BUILD_DIR)/test_phase65_media_session_persistence

test-phase65-media-hls-artifact-reader:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/MediaHlsArtifactReader.cpp \
		core/media/tests/test_media_hls_artifact_reader.cpp \
		-o $(BUILD_DIR)/test_phase65_media_hls_artifact_reader
	$(BUILD_DIR)/test_phase65_media_hls_artifact_reader

test-phase65-media-gateway-http:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		-Icore/http/include -Icore/media/include -Icore/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/media/src/MediaSessionRepository.cpp \
		core/media/src/MediaRouteLeaseRepository.cpp \
		core/media/src/MediaSessionIssuanceService.cpp \
		core/media/src/MediaAccessGrantAuthenticator.cpp \
		core/media/src/MediaHlsArtifactReader.cpp \
		core/http/src/MediaGatewayHttpServer.cpp \
		core/http/tests/test_media_gateway_http_server.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65_media_gateway_http
	$(BUILD_DIR)/test_phase65_media_gateway_http

test-phase65-api-response-headers:
	python3 tools/check_phase65_api_response_headers.py

test-phase65-recording-playback-authorization:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/security/include \
		core/security/tests/test_recording_playback_authorization.cpp \
		-o $(BUILD_DIR)/test_phase65_recording_playback_authorization
	$(BUILD_DIR)/test_phase65_recording_playback_authorization

test-phase65-media-access-credential-http:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/http/include \
		core/http/tests/test_media_access_credential_http.cpp \
		-o $(BUILD_DIR)/test_phase65_media_access_credential_http
	$(BUILD_DIR)/test_phase65_media_access_credential_http

test-phase65-recording-media-session-request-parser:
	$(BUILD_CXX) $(CXXFLAGS) -Iapi/rest/include -Icore/media/include \
		api/rest/src/RecordingMediaSessionRequestParser.cpp \
		api/rest/tests/test_recording_media_session_request_parser.cpp \
		-o $(BUILD_DIR)/test_phase65_recording_media_session_request_parser
	$(BUILD_DIR)/test_phase65_recording_media_session_request_parser

test-phase65-recording-media-session-runtime:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/media/include -Icore/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/media/src/MediaTranscodePolicy.cpp \
		core/media/src/FfmpegHlsCommandBuilder.cpp \
		core/media/src/MediaProcessRunner.cpp \
		core/media/src/MediaSessionWorkspace.cpp \
		core/media/src/MediaSessionRepository.cpp \
		core/media/src/MediaSessionIssuanceService.cpp \
		core/media/src/RecordingMediaSessionRuntime.cpp \
		core/media/tests/test_recording_media_session_runtime.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65_recording_media_session_runtime
	$(BUILD_DIR)/test_phase65_recording_media_session_runtime

test-phase65-live-provider-authority:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/agent/include \
		core/agent/src/BackendAgentLocalProvider.cpp \
		core/agent/src/BackendAgentLiveProviderAuthority.cpp \
		core/agent/tests/test_backend_agent_live_provider_authority.cpp \
		-o $(BUILD_DIR)/test_phase65_live_provider_authority
	$(BUILD_DIR)/test_phase65_live_provider_authority

test-phase65-live-media-adaptation:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/FfmpegHlsCommandBuilder.cpp \
		core/media/src/FfprobeRecordingSource.cpp \
		core/media/src/FfprobeLiveSource.cpp \
		core/media/tests/test_live_media_adaptation.cpp \
		-o $(BUILD_DIR)/test_phase65_live_media_adaptation
	$(BUILD_DIR)/test_phase65_live_media_adaptation

test-phase65-live-media-session-issuance:
	$(BUILD_CXX) $(CXXFLAGS) -pthread -Icore/media/include -Icore/sqlite/include \
		core/sqlite/src/Database.cpp \
		core/media/src/MediaSessionRepository.cpp \
		core/media/src/MediaSessionIssuanceService.cpp \
		core/media/tests/test_live_media_session_issuance.cpp \
		-lsqlite3 -lcrypt \
		-o $(BUILD_DIR)/test_phase65_live_media_session_issuance
	$(BUILD_DIR)/test_phase65_live_media_session_issuance

# test-ci-fast already owns test-fast in the canonical group file. Extend that
# existing public group instead of defining a second canonical group target.
test-fast: test-phase65-media-capability-negotiation test-phase65-media-transcode-policy test-phase65-media-transcode-calibrator-install test-phase65-local-recording-source test-phase65-segmented-recording-byte-source test-phase65-ffmpeg-hls-command-builder test-phase65-ffprobe-recording-source test-phase65-media-session-workspace test-phase65-media-process-runner test-phase65-media-session-persistence test-phase65-media-hls-artifact-reader test-phase65-media-gateway-http test-phase65-api-response-headers test-phase65-recording-playback-authorization test-phase65-media-access-credential-http test-phase65-recording-media-session-request-parser test-phase65-recording-media-session-runtime test-phase65-live-provider-authority test-phase65-live-media-adaptation test-phase65-live-media-session-issuance
