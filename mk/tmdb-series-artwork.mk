CURL_CFLAGS := $(shell pkg-config --cflags libcurl 2>/dev/null)
CURL_LDFLAGS := $(shell pkg-config --libs libcurl 2>/dev/null)

CXXFLAGS += $(CURL_CFLAGS)
LDFLAGS += $(CURL_LDFLAGS)

VDR_SRC += \
        core/vdr/src/EpgSeriesArtworkProviderCacheRepository.cpp \
        core/vdr/src/TmdbSeriesArtworkJson.cpp \
        core/vdr/src/TmdbSeriesArtworkProvider.cpp

DAEMON_SRC += \
        core/http/src/CurlExternalArtworkHttpTransport.cpp \
        core/daemon/src/TmdbSeriesArtworkRuntimeConfig.cpp
