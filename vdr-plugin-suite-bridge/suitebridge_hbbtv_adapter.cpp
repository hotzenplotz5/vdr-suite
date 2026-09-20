#include "suitebridge_hbbtv_adapter.h"

#include <vdr/plugin.h>

#include <cstring>
#include <string>
#include <utility>

namespace {

std::string BoundedText(const char *value, std::size_t capacity)
{
  if (value == nullptr || capacity == 0) return {};
  std::size_t length = 0;
  while (length < capacity && value[length] != '\0') ++length;
  if (length == 0 || length == capacity) return {};
  return std::string(value, length);
}

} // namespace

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

bool SuiteBridgeHbbtvAdapter::Runtime(
    VdrWebHbbtvRuntimeV1 &runtime,
    std::string &error) const
{
  error.clear();

  if (runtime.structSize != sizeof(runtime) ||
      runtime.operation < VDRWEB_HBBTV_RUNTIME_LAUNCH ||
      runtime.operation > VDRWEB_HBBTV_RUNTIME_CLOSE ||
      runtime.applicationId == 0 ||
      runtime.descriptorRevision == 0) {
    error = "invalid_request";
    return false;
  }

  const std::string sessionId =
      BoundedText(runtime.sessionId, VDRWEB_HBBTV_SESSION_ID_MAX);
  const std::string channelId =
      BoundedText(runtime.channelId, VDRWEB_HBBTV_CHANNEL_ID_MAX);
  if (sessionId.empty() || channelId.empty()) {
    error = "invalid_request";
    return false;
  }

  const uint8_t operation = runtime.operation;
  const uint8_t inputAction = runtime.inputAction;
  const uint32_t applicationId = runtime.applicationId;
  const uint64_t descriptorRevision = runtime.descriptorRevision;

  if (!caller_ ||
      !caller_(VDRWEB_SERVICE_HBBTV_RUNTIME_V1, &runtime)) {
    error = "provider_unavailable";
    return false;
  }

  if (runtime.structSize != sizeof(runtime) ||
      runtime.schemaVersion != VDRWEB_HBBTV_RUNTIME_SCHEMA_V1) {
    error = "provider_schema_incompatible";
    return false;
  }

  if (runtime.result > VDRWEB_HBBTV_RUNTIME_RESULT_RUNTIME_UNAVAILABLE ||
      runtime.state > VDRWEB_HBBTV_RUNTIME_STATE_FAILED) {
    error = "provider_runtime_payload_invalid";
    return false;
  }

  if (runtime.operation != operation ||
      runtime.inputAction != inputAction ||
      runtime.applicationId != applicationId ||
      runtime.descriptorRevision != descriptorRevision ||
      BoundedText(runtime.sessionId, VDRWEB_HBBTV_SESSION_ID_MAX) != sessionId ||
      BoundedText(runtime.channelId, VDRWEB_HBBTV_CHANNEL_ID_MAX) != channelId) {
    error = "provider_runtime_identity_mismatch";
    return false;
  }

  return true;
}

bool SuiteBridgeHbbtvAdapter::Presentation(
    VdrWebHbbtvPresentationV1 &presentation,
    std::string &error) const
{
  error.clear();

  if (presentation.structSize != sizeof(presentation) ||
      (presentation.operation != VDRWEB_HBBTV_PRESENTATION_META &&
       presentation.operation != VDRWEB_HBBTV_PRESENTATION_CHUNK)) {
    error = "invalid_request";
    return false;
  }

  const std::string sessionId =
      BoundedText(presentation.sessionId, VDRWEB_HBBTV_SESSION_ID_MAX);
  if (sessionId.empty()) {
    error = "invalid_request";
    return false;
  }

  const uint8_t operation = presentation.operation;
  if (!caller_ ||
      !caller_(VDRWEB_SERVICE_HBBTV_PRESENTATION_V1, &presentation)) {
    error = "provider_unavailable";
    return false;
  }

  if (presentation.structSize != sizeof(presentation) ||
      presentation.schemaVersion != VDRWEB_HBBTV_PRESENTATION_SCHEMA_V1) {
    error = "provider_schema_incompatible";
    return false;
  }

  if (presentation.operation != operation ||
      BoundedText(
          presentation.sessionId,
          VDRWEB_HBBTV_SESSION_ID_MAX) != sessionId ||
      presentation.result >
          VDRWEB_HBBTV_PRESENTATION_RESULT_FRAME_TOO_LARGE ||
      presentation.returnedBytes > VDRWEB_HBBTV_PRESENTATION_CHUNK_MAX ||
      presentation.encodedBytes > 16U * 1024U * 1024U ||
      presentation.renderWidth > 3840U ||
      presentation.renderHeight > 2160U) {
    error = "provider_presentation_payload_invalid";
    return false;
  }

  return true;
}

bool SuiteBridgeHbbtvAdapter::Media(
    VdrWebHbbtvMediaV1 &media,
    std::string &error) const
{
  error.clear();
  if (media.structSize != sizeof(media)) {
    error = "invalid_request";
    return false;
  }
  const std::string sessionId =
      BoundedText(media.sessionId, VDRWEB_HBBTV_SESSION_ID_MAX);
  if (sessionId.empty()) {
    error = "invalid_request";
    return false;
  }
  if (!caller_ || !caller_(VDRWEB_SERVICE_HBBTV_MEDIA_V1, &media)) {
    error = "provider_unavailable";
    return false;
  }
  if (media.structSize != sizeof(media) ||
      media.schemaVersion != VDRWEB_HBBTV_MEDIA_SCHEMA_V1) {
    error = "provider_schema_incompatible";
    return false;
  }
  if (BoundedText(media.sessionId, VDRWEB_HBBTV_SESSION_ID_MAX) != sessionId ||
      media.result > VDRWEB_HBBTV_MEDIA_RESULT_SESSION_MISMATCH ||
      media.state > VDRWEB_HBBTV_MEDIA_STATE_FAILED ||
      media.fullscreen > 1U || media.consumerConnected > 1U ||
      media.x < 0 || media.y < 0 || media.width < 0 || media.height < 0 ||
      media.x > 16384 || media.y > 16384 ||
      media.width > 16384 || media.height > 16384) {
    error = "provider_media_payload_invalid";
    return false;
  }
  return true;
}
