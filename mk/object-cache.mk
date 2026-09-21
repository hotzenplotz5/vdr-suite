OBJECT_CACHE_DIR ?= $(BUILD_DIR)/obj
BUILD_CXX = python3 tools/build_cpp_cached.py --compiler "$(CXX)" --cache-dir "$(OBJECT_CACHE_DIR)" --

.PHONY: check-cpp-object-cache

# Keep the cache contract in the existing fast build-path gate without
# introducing a second public test-group owner.
test-build-artifact-paths: check-cpp-object-cache

check-cpp-object-cache:
	python3 tools/test_build_cpp_cached.py
