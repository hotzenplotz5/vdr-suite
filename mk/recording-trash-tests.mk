.PHONY: test-restfulapi-recording-trash-contract \
	test-restfulapi-recording-trash-workflow \
	test-recording-cache-reconcile-budget

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

test-recording-cache-reconcile-budget:
	$(CXX) $(CXXFLAGS) \
		core/daemon/tests/test_recording_cache_reconcile_budget.cpp \
		-o /tmp/test_recording_cache_reconcile_budget
	/tmp/test_recording_cache_reconcile_budget
