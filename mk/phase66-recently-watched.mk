CXXFLAGS += -Icore/media/include

.PHONY: test-recently-watched

test-recently-watched:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(SQLITE_SRC) \
		core/media/src/RecentlyWatched.cpp \
		core/media/src/RecentlyWatchedRepository.cpp \
		core/media/tests/test_recently_watched.cpp \
		$(LDFLAGS) \
		-o $(BUILD_DIR)/test_recently_watched
	$(BUILD_DIR)/test_recently_watched

# Slice 66.6 core history truth participates in ordinary and fast hosted regression.
test: test-recently-watched
test-ci-fast: test-recently-watched
test-vdr: test-recently-watched
