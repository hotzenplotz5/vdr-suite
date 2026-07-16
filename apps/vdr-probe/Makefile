CXX := g++
ROOT := ../..
BUILD_DIR ?= $(abspath $(ROOT)/.build)
BUILD_CXX = mkdir -p "$(BUILD_DIR)" && $(CXX)

CXXFLAGS := -std=c++17 -Wall -Wextra \
        -I$(ROOT)/core/http/include \
        -I$(ROOT)/core/vdr/include

VDR_SRC := \
        $(ROOT)/core/vdr/src/VdrConfig.cpp \
        $(ROOT)/core/vdr/src/ExternalVdrAdapter.cpp \
        $(ROOT)/core/vdr/src/MockVdrAdapter.cpp \
        $(ROOT)/core/vdr/src/VdrService.cpp \
        $(ROOT)/core/vdr/src/RestfulApiChannelMapper.cpp \
        $(ROOT)/core/vdr/src/RestfulApiRecordingMapper.cpp \
        $(ROOT)/core/vdr/src/RestfulApiTimerMapper.cpp \
        $(ROOT)/core/vdr/src/RestfulApiEventMapper.cpp \
        $(ROOT)/core/vdr/src/RestfulApiStatusMapper.cpp \
        $(ROOT)/core/vdr/src/RestfulApiVdrAdapter.cpp \
        $(ROOT)/core/vdr/src/VdrOverviewService.cpp \
        $(ROOT)/core/vdr/src/VdrOverviewJsonSerializer.cpp \
        $(ROOT)/core/vdr/src/VdrAdapterFactory.cpp

HTTP_SRC := \
        $(ROOT)/core/http/src/BasicHttpClient.cpp

.PHONY: vdr-probe clean

vdr-probe:
	$(BUILD_CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(HTTP_SRC) \
		main.cpp \
		-o $(BUILD_DIR)/vdr-probe

clean:
	rm -f "$(BUILD_DIR)/vdr-probe"
