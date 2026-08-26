DAEMON_SRC += \
	core/media/src/MediaPlaybackContract.cpp \
	api/rest/src/MediaPlaybackContractResponse.cpp

.PHONY: test-phase65d-media-playback-contract \
	test-phase65d-media-playback-contract-response

test-phase65d-media-playback-contract:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/MediaPlaybackContract.cpp \
		core/media/tests/test_media_playback_contract.cpp \
		-o $(BUILD_DIR)/test_phase65d_media_playback_contract
	$(BUILD_DIR)/test_phase65d_media_playback_contract

test-phase65d-media-playback-contract-response:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include -Iapi/rest/include \
		core/media/src/MediaPlaybackContract.cpp \
		api/rest/src/MediaPlaybackContractResponse.cpp \
		api/rest/tests/test_media_playback_contract_response.cpp \
		-o $(BUILD_DIR)/test_phase65d_media_playback_contract_response
	$(BUILD_DIR)/test_phase65d_media_playback_contract_response

test-fast: test-phase65d-media-playback-contract test-phase65d-media-playback-contract-response
