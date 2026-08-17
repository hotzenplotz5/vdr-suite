.PHONY: test-phase65-media-capability-negotiation

test-phase65-media-capability-negotiation:
	$(BUILD_CXX) $(CXXFLAGS) -Icore/media/include \
		core/media/src/MediaPresentationSelector.cpp \
		core/media/tests/test_media_presentation_selector.cpp \
		-o $(BUILD_DIR)/test_phase65_media_capability_negotiation
	$(BUILD_DIR)/test_phase65_media_capability_negotiation

# Phase 65 is an active runtime phase. Keep its contract test in the fast CI gate.
test-ci-fast: test-phase65-media-capability-negotiation
