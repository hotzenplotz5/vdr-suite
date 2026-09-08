.PHONY: test-basic-http-client-deadline

test-basic-http-client-deadline:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		core/http/src/BasicHttpClient.cpp \
		core/http/tests/test_basic_http_client_deadline.cpp \
		-o $(BUILD_DIR)/test_basic_http_client_deadline
	$(BUILD_DIR)/test_basic_http_client_deadline

test: test-basic-http-client-deadline
