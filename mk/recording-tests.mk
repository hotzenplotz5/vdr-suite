.PHONY: test-restfulapi-recording-trash-contract

test-restfulapi-recording-trash-contract:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_recording_trash_contract.cpp \
		-o $(BUILD_DIR)/test_restfulapi_recording_trash_contract
	$(BUILD_DIR)/test_restfulapi_recording_trash_contract
