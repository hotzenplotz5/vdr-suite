#pragma once

#include <map>
#include <string>
#include <vector>

struct RuntimeSuiteBridgeConfig
{
    bool enabled = false;
    std::string backendId = "default";
    std::string host = "127.0.0.1";
    int port = 6419;
    int connectTimeoutMs = 1000;
    int ioTimeoutMs = 1000;
    int operationTimeoutMs = 3000;
    int pollIntervalMs = 5000;
    int staleAfterMs = 15000;
    int offlineAfterMs = 60000;
    int reconnectInitialMs = 1000;
    int reconnectMaximumMs = 30000;
};

struct RuntimeSeriesArtworkFallbackConfig
{
    bool enabled = false;
    std::vector<std::string> sourceRoots = {
        "/var/cache/vdr-suite/epg-artwork/incoming"
    };
    std::string cacheRoot =
        "/var/cache/vdr-suite/epg-artwork/external";
    int maximumSourceBytes = 16 * 1024 * 1024;
    int maximumDimension = 16384;
    bool orphanCleanupEnabled = false;
    int orphanCleanupMinimumAgeSeconds = 7 * 24 * 60 * 60;
    int orphanCleanupMaximumFiles = 64;
    bool incomingCleanupEnabled = false;
    int incomingCleanupMinimumAgeSeconds = 24 * 60 * 60;
    int incomingCleanupMaximumFiles = 64;
};

class RuntimeConfig
{
public:
    RuntimeConfig();

    const std::string& databasePath() const;
    const std::string& vdrMode() const;
    const std::string& vdrHost() const;
    int vdrPort() const;
    const std::string& httpListenHost() const;
    int httpListenPort() const;
    const std::map<std::string, std::string>& recordingArtworkRoots() const;
    const RuntimeSuiteBridgeConfig& suiteBridge() const;
    const RuntimeSeriesArtworkFallbackConfig& seriesArtworkFallback() const;

private:
    std::string databasePath_;
    std::string vdrMode_;
    std::string vdrHost_;
    int vdrPort_;
    std::string httpListenHost_;
    int httpListenPort_;
    std::map<std::string, std::string> recordingArtworkRoots_;
    RuntimeSuiteBridgeConfig suiteBridge_;
    RuntimeSeriesArtworkFallbackConfig seriesArtworkFallback_;
};
