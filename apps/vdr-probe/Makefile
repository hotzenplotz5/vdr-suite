CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra \
        -Icore/http/include \
        -Icore/vdr/include

VDR_SRC := \
        core/vdr/src/VdrConfig.cpp \
        core/vdr/src/ExternalVdrAdapter.cpp \
        core/vdr/src/MockVdrAdapter.cpp \
        core/vdr/src/VdrService.cpp \
        core/vdr/src/RestfulApiChannelMapper.cpp \
        core/vdr/src/RestfulApiRecordingMapper.cpp \
        core/vdr/src/RestfulApiTimerMapper.cpp \
        core/vdr/src/RestfulApiEventMapper.cpp \
        core/vdr/src/RestfulApiStatusMapper.cpp \
        core/vdr/src/RestfulApiVdrAdapter.cpp \
        core/vdr/src/VdrOverviewService.cpp \
        core/vdr/src/VdrOverviewJsonSerializer.cpp \
        core/vdr/src/VdrAdapterFactory.cpp

HTTP_SRC := \
        core/http/src/BasicHttpClient.cpp

.PHONY: vdr-probe clean

vdr-probe:
	$(CXX) $(CXXFLAGS) \
		$(VDR_SRC) \
		$(HTTP_SRC) \
		apps/vdr-probe/main.cpp \
		-o /tmp/vdr-probe

clean:
	rm -f /tmp/vdr-probe
