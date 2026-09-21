#include "RuntimeConfig.h"

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{

const std::vector<const char*> RuntimeVariables = {
    "VDR_SUITE_SUITE_BRIDGE_ENABLED",
    "VDR_SUITE_SUITE_BRIDGE_BACKEND_ID",
    "VDR_SUITE_SUITE_BRIDGE_HOST",
    "VDR_SUITE_SUITE_BRIDGE_PORT",
    "VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS",
    "VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS",
    "VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS",
    "VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS",
    "VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS",
    "VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS",
    "VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS",
    "VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_SOURCE_ROOTS",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_CACHE_ROOT",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_BYTES",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_DIMENSION",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_ENABLED",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MIN_AGE_SECONDS",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MAX_FILES",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_ENABLED",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MIN_AGE_SECONDS",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MAX_FILES"
};

void clearRuntimeEnvironment()
{
    for (const char* name : RuntimeVariables)
    {
        unsetenv(name);
    }
}

}

int main()
{
    unsetenv("VDR_SUITE_DATABASE_PATH");
    unsetenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS");
    clearRuntimeEnvironment();

    RuntimeConfig defaultConfig;
    assert(defaultConfig.databasePath() == "/tmp/vdr-suite-test.db");
    assert(defaultConfig.recordingArtworkRoots().empty());
    assert(!defaultConfig.suiteBridge().enabled);
    assert(defaultConfig.suiteBridge().backendId == "default");
    assert(defaultConfig.suiteBridge().host == "127.0.0.1");
    assert(defaultConfig.suiteBridge().port == 6419);
    assert(defaultConfig.suiteBridge().connectTimeoutMs == 1000);
    assert(defaultConfig.suiteBridge().ioTimeoutMs == 1000);
    assert(defaultConfig.suiteBridge().operationTimeoutMs == 3000);
    assert(defaultConfig.suiteBridge().pollIntervalMs == 5000);
    assert(defaultConfig.suiteBridge().staleAfterMs == 15000);
    assert(defaultConfig.suiteBridge().offlineAfterMs == 60000);
    assert(defaultConfig.suiteBridge().reconnectInitialMs == 1000);
    assert(defaultConfig.suiteBridge().reconnectMaximumMs == 30000);
    assert(!defaultConfig.seriesArtworkFallback().enabled);
    assert(defaultConfig.seriesArtworkFallback().sourceRoots ==
           std::vector<std::string>{
               "/var/cache/vdr-suite/epg-artwork/incoming"});
    assert(defaultConfig.seriesArtworkFallback().cacheRoot ==
           "/var/cache/vdr-suite/epg-artwork/external");
    assert(defaultConfig.seriesArtworkFallback().maximumSourceBytes ==
           16 * 1024 * 1024);
    assert(defaultConfig.seriesArtworkFallback().maximumDimension == 16384);
    assert(!defaultConfig.seriesArtworkFallback().orphanCleanupEnabled);
    assert(defaultConfig.seriesArtworkFallback()
               .orphanCleanupMinimumAgeSeconds ==
           7 * 24 * 60 * 60);
    assert(defaultConfig.seriesArtworkFallback().orphanCleanupMaximumFiles ==
           64);
    assert(!defaultConfig.seriesArtworkFallback().incomingCleanupEnabled);
    assert(defaultConfig.seriesArtworkFallback()
               .incomingCleanupMinimumAgeSeconds ==
           24 * 60 * 60);
    assert(defaultConfig.seriesArtworkFallback().incomingCleanupMaximumFiles ==
           64);

    setenv(
        "VDR_SUITE_DATABASE_PATH",
        "/var/lib/vdr-suite/vdr-suite.db",
        1);
    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=/srv/tvscraper;wohnhaus2=/mnt/secondary artwork",
        1);
    setenv("VDR_SUITE_SUITE_BRIDGE_ENABLED", "YES", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_BACKEND_ID", "wohnhaus2", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_HOST", "127.0.0.2", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_PORT", "6420", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS", "1100", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS", "1200", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS", "3300", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS", "6000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS", "18000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS", "72000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS", "1500", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS", "45000", 1);
    setenv("VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED", "ON", 1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_SOURCE_ROOTS",
        "/srv/provider-one;/srv/provider two",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_CACHE_ROOT",
        "/var/cache/vdr-suite/epg-artwork/custom",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_BYTES",
        "8388608",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_DIMENSION",
        "8192",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_ENABLED",
        "yes",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MIN_AGE_SECONDS",
        "172800",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MAX_FILES",
        "12",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_ENABLED",
        "on",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MIN_AGE_SECONDS",
        "43200",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MAX_FILES",
        "21",
        1);

    RuntimeConfig overriddenConfig;
    assert(overriddenConfig.databasePath() ==
           "/var/lib/vdr-suite/vdr-suite.db");
    assert(overriddenConfig.recordingArtworkRoots().size() == 2);
    assert(overriddenConfig.recordingArtworkRoots().at("default") ==
           "/srv/tvscraper");
    assert(overriddenConfig.recordingArtworkRoots().at("wohnhaus2") ==
           "/mnt/secondary artwork");
    assert(overriddenConfig.suiteBridge().enabled);
    assert(overriddenConfig.suiteBridge().backendId == "wohnhaus2");
    assert(overriddenConfig.suiteBridge().host == "127.0.0.2");
    assert(overriddenConfig.suiteBridge().port == 6420);
    assert(overriddenConfig.suiteBridge().connectTimeoutMs == 1100);
    assert(overriddenConfig.suiteBridge().ioTimeoutMs == 1200);
    assert(overriddenConfig.suiteBridge().operationTimeoutMs == 3300);
    assert(overriddenConfig.suiteBridge().pollIntervalMs == 6000);
    assert(overriddenConfig.suiteBridge().staleAfterMs == 18000);
    assert(overriddenConfig.suiteBridge().offlineAfterMs == 72000);
    assert(overriddenConfig.suiteBridge().reconnectInitialMs == 1500);
    assert(overriddenConfig.suiteBridge().reconnectMaximumMs == 45000);
    assert(overriddenConfig.seriesArtworkFallback().enabled);
    assert(overriddenConfig.seriesArtworkFallback().sourceRoots ==
           (std::vector<std::string>{
               "/srv/provider-one",
               "/srv/provider two"}));
    assert(overriddenConfig.seriesArtworkFallback().cacheRoot ==
           "/var/cache/vdr-suite/epg-artwork/custom");
    assert(overriddenConfig.seriesArtworkFallback().maximumSourceBytes ==
           8388608);
    assert(overriddenConfig.seriesArtworkFallback().maximumDimension == 8192);
    assert(overriddenConfig.seriesArtworkFallback().orphanCleanupEnabled);
    assert(overriddenConfig.seriesArtworkFallback()
               .orphanCleanupMinimumAgeSeconds == 172800);
    assert(overriddenConfig.seriesArtworkFallback()
               .orphanCleanupMaximumFiles == 12);
    assert(overriddenConfig.seriesArtworkFallback().incomingCleanupEnabled);
    assert(overriddenConfig.seriesArtworkFallback()
               .incomingCleanupMinimumAgeSeconds == 43200);
    assert(overriddenConfig.seriesArtworkFallback()
               .incomingCleanupMaximumFiles == 21);

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=/srv/one;default=/srv/two",
        1);
    RuntimeConfig duplicateConfig;
    assert(duplicateConfig.recordingArtworkRoots().empty());

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "default=relative/path",
        1);
    RuntimeConfig relativeConfig;
    assert(relativeConfig.recordingArtworkRoots().empty());

    setenv(
        "VDR_SUITE_RECORDING_ARTWORK_ROOTS",
        "invalid backend=/srv/tvscraper",
        1);
    RuntimeConfig invalidBackendArtworkConfig;
    assert(invalidBackendArtworkConfig.recordingArtworkRoots().empty());

    setenv("VDR_SUITE_SUITE_BRIDGE_ENABLED", "invalid", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_BACKEND_ID", "invalid backend", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_HOST", std::string(300, 'x').c_str(), 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_PORT", "70000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS", "-1", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS", "not-a-number", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS", "0", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS", "20000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS", "1000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS", "1000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS", "40000", 1);
    setenv("VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS", "1000", 1);
    setenv("VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED", "invalid", 1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_SOURCE_ROOTS",
        "relative;/srv/provider",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_CACHE_ROOT",
        "/tmp/escape",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_BYTES",
        "33",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_MAX_DIMENSION",
        "99999",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_ENABLED",
        "invalid",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MIN_AGE_SECONDS",
        "12",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MAX_FILES",
        "0",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_ENABLED",
        "invalid",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MIN_AGE_SECONDS",
        "3599",
        1);
    setenv(
        "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MAX_FILES",
        "1025",
        1);

    RuntimeConfig invalidSuiteBridgeConfig;
    assert(!invalidSuiteBridgeConfig.suiteBridge().enabled);
    assert(invalidSuiteBridgeConfig.suiteBridge().backendId == "default");
    assert(invalidSuiteBridgeConfig.suiteBridge().host == "127.0.0.1");
    assert(invalidSuiteBridgeConfig.suiteBridge().port == 6419);
    assert(invalidSuiteBridgeConfig.suiteBridge().connectTimeoutMs == 1000);
    assert(invalidSuiteBridgeConfig.suiteBridge().ioTimeoutMs == 1000);
    assert(invalidSuiteBridgeConfig.suiteBridge().operationTimeoutMs == 3000);
    assert(invalidSuiteBridgeConfig.suiteBridge().pollIntervalMs == 20000);
    assert(invalidSuiteBridgeConfig.suiteBridge().staleAfterMs == 20000);
    assert(invalidSuiteBridgeConfig.suiteBridge().offlineAfterMs == 20000);
    assert(invalidSuiteBridgeConfig.suiteBridge().reconnectInitialMs == 40000);
    assert(invalidSuiteBridgeConfig.suiteBridge().reconnectMaximumMs == 40000);
    assert(!invalidSuiteBridgeConfig.seriesArtworkFallback().enabled);
    assert(invalidSuiteBridgeConfig.seriesArtworkFallback().sourceRoots ==
           std::vector<std::string>{
               "/var/cache/vdr-suite/epg-artwork/incoming"});
    assert(invalidSuiteBridgeConfig.seriesArtworkFallback().cacheRoot ==
           "/var/cache/vdr-suite/epg-artwork/external");
    assert(invalidSuiteBridgeConfig.seriesArtworkFallback().maximumSourceBytes ==
           16 * 1024 * 1024);
    assert(invalidSuiteBridgeConfig.seriesArtworkFallback().maximumDimension ==
           16384);
    assert(!invalidSuiteBridgeConfig.seriesArtworkFallback()
                .orphanCleanupEnabled);
    assert(invalidSuiteBridgeConfig.seriesArtworkFallback()
               .orphanCleanupMinimumAgeSeconds ==
           7 * 24 * 60 * 60);
    assert(invalidSuiteBridgeConfig.seriesArtworkFallback()
               .orphanCleanupMaximumFiles == 64);
    assert(!invalidSuiteBridgeConfig.seriesArtworkFallback()
                .incomingCleanupEnabled);
    assert(invalidSuiteBridgeConfig.seriesArtworkFallback()
               .incomingCleanupMinimumAgeSeconds ==
           24 * 60 * 60);
    assert(invalidSuiteBridgeConfig.seriesArtworkFallback()
               .incomingCleanupMaximumFiles == 64);

    setenv("VDR_SUITE_DATABASE_PATH", "", 1);
    setenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS", "", 1);
    clearRuntimeEnvironment();

    RuntimeConfig emptyOverrideConfig;
    assert(emptyOverrideConfig.databasePath() ==
           "/tmp/vdr-suite-test.db");
    assert(emptyOverrideConfig.recordingArtworkRoots().empty());
    assert(!emptyOverrideConfig.suiteBridge().enabled);
    assert(!emptyOverrideConfig.seriesArtworkFallback().enabled);
    assert(emptyOverrideConfig.seriesArtworkFallback().sourceRoots ==
           std::vector<std::string>{
               "/var/cache/vdr-suite/epg-artwork/incoming"});
    assert(!emptyOverrideConfig.seriesArtworkFallback().orphanCleanupEnabled);
    assert(!emptyOverrideConfig.seriesArtworkFallback().incomingCleanupEnabled);

    unsetenv("VDR_SUITE_DATABASE_PATH");
    unsetenv("VDR_SUITE_RECORDING_ARTWORK_ROOTS");
    clearRuntimeEnvironment();

    return 0;
}
