PLUGIN := suiteagent
CXX ?= g++
PKG_CONFIG ?= pkg-config

VDR_CFLAGS := $(shell $(PKG_CONFIG) --cflags vdr 2>/dev/null)
VDR_APIVERSION := $(shell $(PKG_CONFIG) --variable=apiversion vdr 2>/dev/null)

CPPFLAGS += $(VDR_CFLAGS)
CXXFLAGS += -std=c++17 -fPIC -Wall -Wextra -Wpedantic -Werror
LDFLAGS += -shared

OBJS := suiteagent.o recording_name.o
TEST := test_recording_name
TARGET := libvdr-$(PLUGIN).so.$(VDR_APIVERSION)

.PHONY: all test clean check-vdr

all: check-vdr $(TARGET)

check-vdr:
	@test -n "$(VDR_APIVERSION)" || { \
		echo "VDR development pkg-config metadata not found" >&2; \
		exit 1; \
	}

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

$(TEST): test_recording_name.cpp recording_name.cpp recording_name.h
	$(CXX) -std=c++17 -Wall -Wextra -Wpedantic -Werror \
		-o $@ test_recording_name.cpp recording_name.cpp

test: $(TEST)
	./$(TEST)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST)
