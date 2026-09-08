.PHONY: test-basic-http-client-deadline

test-basic-http-client-deadline:
	$(BUILD_CXX) $(CXXFLAGS) -pthread \
		core/http/src/BasicHttpClient.cpp \
		core/http/tests/test_basic_http_client_deadline.cpp \
		-o $(BUILD_DIR)/test_basic_http_client_deadline
	$(BUILD_DIR)/test_basic_http_client_deadline

# The HTTP recovery changes must be exercised by the required fast CI graph,
# not only by the broader local test target.
test-fast: test-basic-http-client-deadline

test: test-basic-http-client-deadline
