#include "suitebridge_hbbtv_adapter.h"

#include <vdr/plugin.h>

#include <cstring>
#include <utility>

SuiteBridgeHbbtvAdapter::SuiteBridgeHbbtvAdapter()
    : SuiteBridgeHbbtvAdapter(
          [](const char *id, void *data) {
            return cPluginManager::CallFirstService(id, data) != nullptr;
          })
{
}

SuiteBridgeHbbtvAdapter::SuiteBridgeHbbtvAdapter(ServiceCaller caller)
    : caller_(std::move(caller))
{
}

bool SuiteBridgeHbbtvAdapter::Discover(
    const std::string &channelId,
    VdrWebHbbtvDiscoveryV1 &discovery,
    std::string &error) const
{
  error.clear();
  if (channelId.empty() || channelId.size() >= VDRWEB_HBBTV_CHANNEL_ID_MAX) {
    error = "invalid_request";
    return false;
  }

  std::memset(&discovery, 0, sizeof(discovery));
  discovery.structSize = sizeof(discovery);
  std::memcpy(discovery.channelId, channelId.data(), channelId.size());
  discovery.channelId[channelId.size()] = '\0';

  if (!caller_ ||
      !caller_(VDRWEB_SERVICE_HBBTV_DISCOVERY_V1, &discovery)) {
    error = "provider_unavailable";
    return false;
  }

  if (discovery.structSize != sizeof(discovery) ||
      discovery.schemaVersion != VDRWEB_HBBTV_SERVICE_SCHEMA_V1) {
    error = "provider_schema_incompatible";
    return false;
  }

  if (discovery.applicationCount > VDRWEB_HBBTV_MAX_APPLICATIONS) {
    error = "provider_application_count_invalid";
    return false;
  }

  return true;
}
