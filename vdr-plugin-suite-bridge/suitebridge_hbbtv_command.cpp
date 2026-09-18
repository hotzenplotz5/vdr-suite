#include "suitebridge_hbbtv_command.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <strings.h>

namespace {

constexpr std::size_t kMaximumPayloadBytes = 48U * 1024U;

void AppendJsonString(std::ostringstream &out, const std::string &value)
{
  out << '"';
  for (unsigned char ch : value) {
    switch (ch) {
      case '"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\b': out << "\\b"; break;
      case '\f': out << "\\f"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (ch < 0x20) {
          out << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<unsigned int>(ch) << std::dec << std::setfill(' ');
        } else {
          out << static_cast<char>(ch);
        }
        break;
    }
  }
  out << '"';
}

std::string BoundedString(const char *value, std::size_t capacity)
{
  if (value == nullptr || capacity == 0) return {};
  std::size_t length = 0;
  while (length < capacity && value[length] != '\0') ++length;
  if (length == capacity) return {};
  return std::string(value, length);
}

const char *ResultName(std::uint8_t result)
{
  switch (result) {
    case VDRWEB_HBBTV_RESULT_OK: return "ok";
    case VDRWEB_HBBTV_RESULT_INVALID_REQUEST: return "invalid_request";
    case VDRWEB_HBBTV_RESULT_NO_LIVE_SERVICE: return "no_live_service";
    case VDRWEB_HBBTV_RESULT_CHANNEL_MISMATCH: return "channel_mismatch";
    case VDRWEB_HBBTV_RESULT_NO_APPLICATIONS: return "no_applications";
    case VDRWEB_HBBTV_RESULT_RECEIVER_INACTIVE: return "receiver_inactive";
  }
  return "unknown";
}

SuiteBridgeCommandResult Rejected(int replyCode, const char *reason)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = replyCode;
  std::ostringstream payload;
  payload << "{\"schemaVersion\":1,\"result\":\"rejected\",\"reason\":";
  AppendJsonString(payload, reason ? reason : "unknown");
  payload << '}';
  result.payload = payload.str();
  return result;
}

} // namespace

SuiteBridgeCommandResult SuiteBridgeHbbtvCommandService::Handle(
    const char *command,
    const char *option) const
{
  if (command == nullptr || strcasecmp(command, "HBBAPPS") != 0) return {};
  if (provider_ == nullptr) return Rejected(550, "provider_unavailable");
  if (option == nullptr) return Rejected(504, "hbbtv_discovery_request_invalid");

  std::istringstream input(option);
  std::string schema;
  std::string channelId;
  std::string extra;
  if (!(input >> schema >> channelId) || (input >> extra) || schema != "1") {
    return Rejected(504, "hbbtv_discovery_request_invalid");
  }

  VdrWebHbbtvDiscoveryV1 discovery{};
  std::string error;
  if (!provider_->Discover(channelId, discovery, error)) {
    return Rejected(550, error.c_str());
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = 250;

  std::ostringstream payload;
  payload << "{\"schemaVersion\":1"
          << ",\"provider\":\"vdr-plugin-web\""
          << ",\"providerSchemaVersion\":" << discovery.schemaVersion
          << ",\"capability\":\"broadcast.hbbtv.discovery\""
          << ",\"result\":";
  AppendJsonString(payload, ResultName(discovery.result));
  payload << ",\"resultCode\":" << static_cast<unsigned int>(discovery.result)
          << ",\"channel\":";
  AppendJsonString(payload, channelId);
  payload << ",\"receiverActive\":"
          << (discovery.receiverActive ? "true" : "false")
          << ",\"revision\":" << discovery.discoveryRevision
          << ",\"observedAt\":" << discovery.observedAt
          << ",\"applications\":[";

  const std::size_t count =
      discovery.applicationCount <= VDRWEB_HBBTV_MAX_APPLICATIONS
          ? discovery.applicationCount : 0U;
  for (std::size_t index = 0; index < count; ++index) {
    if (index != 0) payload << ',';
    const VdrWebHbbtvApplicationV1 &application =
        discovery.applications[index];
    payload << "{\"applicationId\":" << application.applicationId
            << ",\"controlCode\":"
            << static_cast<unsigned int>(application.controlCode)
            << ",\"priority\":"
            << static_cast<unsigned int>(application.priority)
            << ",\"name\":";
    AppendJsonString(
        payload,
        BoundedString(
            application.name,
            VDRWEB_HBBTV_APPLICATION_NAME_MAX));
    payload << ",\"urlBase\":";
    AppendJsonString(
        payload,
        BoundedString(application.urlBase, VDRWEB_HBBTV_URL_BASE_MAX));
    payload << ",\"urlLocation\":";
    AppendJsonString(
        payload,
        BoundedString(
            application.urlLocation,
            VDRWEB_HBBTV_URL_LOCATION_MAX));
    payload << ",\"urlExtension\":";
    AppendJsonString(
        payload,
        BoundedString(
            application.urlExtension,
            VDRWEB_HBBTV_URL_EXTENSION_MAX));
    payload << '}';
  }
  payload << "]}";

  result.payload = payload.str();
  if (result.payload.size() > kMaximumPayloadBytes) {
    return Rejected(552, "hbbtv_discovery_payload_too_large");
  }
  return result;
}
