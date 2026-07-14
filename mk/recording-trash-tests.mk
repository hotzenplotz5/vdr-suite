.PHONY: test-restfulapi-recording-trash-contract \
	test-restfulapi-recording-trash-workflow

test-restfulapi-recording-trash-contract:
	$(CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_recording_trash_contract.cpp \
		-o /tmp/test_restfulapi_recording_trash_contract
	/tmp/test_restfulapi_recording_trash_contract

test-restfulapi-recording-trash-workflow:
	$(CXX) $(CXXFLAGS) \
		core/recordings/tests/test_restfulapi_recording_action_executor_response_contract.cpp \
		-o /tmp/test_restfulapi_recording_trash_workflow
	/tmp/test_restfulapi_recording_trash_workflow
