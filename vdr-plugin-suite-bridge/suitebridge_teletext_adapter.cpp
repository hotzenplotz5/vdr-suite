#include "suitebridge_teletext_adapter.h"

#include <vdr/plugin.h>

#include <cstring>
#include <utility>

SuiteBridgeTeletextAdapter::SuiteBridgeTeletextAdapter()
    : SuiteBridgeTeletextAdapter(
          [](const char *id, void *data) {
            return cPluginManager::CallFirstService(id, data) != nullptr;
          })
{
}

SuiteBridgeTeletextAdapter::SuiteBridgeTeletextAdapter(ServiceCaller caller)
    : caller_(std::move(caller))
{
}

bool SuiteBridgeTeletextAdapter::ValidateCapabilities(
    const OsdTeletextCapabilitiesV1 &capabilities,
    std::string &error) const
{
  if (capabilities.structSize != sizeof(capabilities) ||
      capabilities.schemaVersion != OSDTELETEXT_SERVICE_SCHEMA_V1) {
    error = "provider_schema_incompatible";
    return false;
  }
  if (capabilities.rows != OSDTELETEXT_PAGE_ROWS ||
      capabilities.columns != OSDTELETEXT_PAGE_COLUMNS ||
      capabilities.pageRead == 0) {
    error = "provider_capability_incompatible";
    return false;
  }
  return true;
}

bool SuiteBridgeTeletextAdapter::ReadCapabilities(
    OsdTeletextCapabilitiesV1 &capabilities,
    std::string &error) const
{
  error.clear();
  std::memset(&capabilities, 0, sizeof(capabilities));
  capabilities.structSize = sizeof(capabilities);

  if (!caller_ ||
      !caller_(OSDTELETEXT_SERVICE_CAPABILITIES_V1, &capabilities)) {
    error = "provider_unavailable";
    return false;
  }

  return ValidateCapabilities(capabilities, error);
}

bool SuiteBridgeTeletextAdapter::ReadPage(
    const std::string &channelId,
    std::uint16_t pageNumber,
    std::uint16_t subpageCode,
    OsdTeletextGetPageV1 &page,
    std::string &error) const
{
  error.clear();
  OsdTeletextCapabilitiesV1 capabilities{};
  if (!ReadCapabilities(capabilities, error)) return false;

  if (channelId.empty() || channelId.size() >= OSDTELETEXT_CHANNEL_ID_MAX ||
      pageNumber < 100 || pageNumber > 899) {
    error = "invalid_request";
    return false;
  }

  std::memset(&page, 0, sizeof(page));
  page.structSize = sizeof(page);
  std::memcpy(page.channelId, channelId.data(), channelId.size());
  page.channelId[channelId.size()] = '\0';
  page.pageNumber = pageNumber;
  page.subpageCode = subpageCode;

  if (!caller_ || !caller_(OSDTELETEXT_SERVICE_GET_PAGE_V1, &page)) {
    error = "page_service_unavailable";
    return false;
  }

  if (page.structSize != sizeof(page) ||
      page.schemaVersion != OSDTELETEXT_SERVICE_SCHEMA_V1) {
    error = "page_schema_incompatible";
    return false;
  }

  if (page.result == OSDTELETEXT_RESULT_OK &&
      (page.rows != OSDTELETEXT_PAGE_ROWS ||
       page.columns != OSDTELETEXT_PAGE_COLUMNS)) {
    error = "page_shape_incompatible";
    return false;
  }

  return true;
}
