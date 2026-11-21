.PHONY: test-phase67-runtime-generation test-phase67-embedded-backend-lifecycle test-phase67-teletext-lifecycle

test-phase67-runtime-generation:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/agent/src/BackendRuntimeGeneration.cpp \
		core/agent/tests/test_backend_runtime_generation.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_backend_runtime_generation
	$(BUILD_DIR)/test_backend_runtime_generation

test-phase67-embedded-backend-lifecycle:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/agent/src/BackendRuntimeGeneration.cpp \
		core/daemon/src/EmbeddedBackendLifecycle.cpp \
		core/daemon/tests/test_embedded_backend_lifecycle.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_embedded_backend_lifecycle
	$(BUILD_DIR)/test_embedded_backend_lifecycle

test-phase67-teletext-lifecycle: \
	test-phase67-runtime-generation \
	test-phase67-embedded-backend-lifecycle
	python3 tools/check_phase67_teletext_embedded_lifecycle.py
