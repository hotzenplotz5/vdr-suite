.PHONY: test-null-runtime-logger

# The smallest runtime logger contract belongs to the fast diagnostics path.
test-runtime-diagnostics: test-null-runtime-logger


test-null-runtime-logger:
	$(CXX) $(CXXFLAGS) \
		core/runtime/src/NullRuntimeLogger.cpp \
		core/runtime/tests/test_null_runtime_logger.cpp \
		-o /tmp/test_null_runtime_logger
	/tmp/test_null_runtime_logger
