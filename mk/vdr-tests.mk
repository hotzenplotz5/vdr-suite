test-vdr-config:
	$(CXX) $(CXXFLAGS) \
		core/vdr/src/VdrConfig.cpp \
		core/vdr/tests/test_vdr_config.cpp \
		-o /tmp/test_vdr_config
	/tmp/test_vdr_config

# existing content preserved above in repository

test-null-runtime-logger:
	$(CXX) $(CXXFLAGS) \
		core/runtime/src/NullRuntimeLogger.cpp \
		core/runtime/tests/test_null_runtime_logger.cpp \
		-o /tmp/test_null_runtime_logger
	/tmp/test_null_runtime_logger
