.PHONY: test-restfulapi-recording-trash-contract


test-restfulapi-recording-trash-contract:
	$(CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_recording_trash_contract.cpp \
		-o /tmp/test_restfulapi_recording_trash_contract
	/tmp/test_restfulapi_recording_trash_contract
