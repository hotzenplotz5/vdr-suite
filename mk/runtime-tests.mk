.PHONY: test-null-runtime-logger

# The smallest runtime logger contract belongs to the fast diagnostics path.
test-runtime-diagnostics: test-null-runtime-logger


test-null-runtime-logger:
	$(BUILD_CXX) $(CXXFLAGS) \
		core/runtime/src/NullRuntimeLogger.cpp \
		core/runtime/tests/test_null_runtime_logger.cpp \
		-o $(BUILD_DIR)/test_null_runtime_logger
	$(BUILD_DIR)/test_null_runtime_logger
