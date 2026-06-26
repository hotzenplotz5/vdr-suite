CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra \
        -Icore/sqlite/include \
        -Icore/recordings/include \
        -Icore/daemon/include \
        -Icore/vdr/include \
        -Icore/http/include \
        -Icore/runtime/include \
        -Iapi/rest/include

LDFLAGS := $(shell pkg-config --libs sqlite3)

SQLITE_SRC := core/sqlite/src/Database.cpp

.PHONY: all test clean prepare-test-db dashboard-cli daemon test-vdr-snapshot-builder-startup-snapshot

all: test

test-ci-fast: test-vdr-snapshot-builder-startup-snapshot

test-vdr: test-vdr-snapshot-builder-startup-snapshot

test-vdr-snapshot-builder-startup-snapshot:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		core/vdr/tests/test_vdr_snapshot_builder_startup_snapshot.cpp \
		-o /tmp/test_vdr_snapshot_builder_startup_snapshot
	/tmp/test_vdr_snapshot_builder_startup_snapshot

prepare-test-db:
	rm -f /tmp/vdr-suite-test.db
	sqlite3 /tmp/vdr-suite-test.db < database/schema/vdr-suite.sql
	sqlite3 /tmp/vdr-suite-test.db < database/testdata/sample-data.sql
