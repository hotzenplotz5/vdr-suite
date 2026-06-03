CXX := g++
ROOT := ../..

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
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(HTTP_SRC) \
		main.cpp \
		-o /tmp/vdr-probe

clean:
	rm -f /tmp/vdr-probe
