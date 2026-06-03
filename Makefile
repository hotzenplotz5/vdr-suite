CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra \
        -Icore/sqlite/include \
        -Icore/recordings/include \
        -Icore/daemon/include \
        -Icore/vdr/include \
	-Icore/http/include \
        -Iapi/rest/include
        
LDFLAGS := $(shell pkg-config --libs sqlite3)

SQLITE_SRC := core/sqlite/src/Database.cpp

include mk/vdr-sources.mk


include mk/http-sources.mk


include mk/recording-sources.mk

