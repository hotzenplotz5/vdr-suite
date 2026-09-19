#include "suitebridge_hbbtv_command.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
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

bool SafeIdentity(const std::string &value, std::size_t maximumLength)
{
  return !value.empty() && value.size() < maximumLength &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' ||
            character == '_' || character == '.' || character == ':';
      });
}

template <std::size_t N>
void CopyText(char (&target)[N], const std::string &value)
{
  std::memset(target, 0, N);
  const std::size_t count = std::min(value.size(), N - 1U);
  std::memcpy(target, value.data(), count);
  target[count] = '\0';
}

const char *DiscoveryResultName(std::uint8_t result)
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

const char *RuntimeResultName(std::uint8_t result)
{
  switch (result) {
    case VDRWEB_HBBTV_RUNTIME_RESULT_OK: return "ok";
    case VDRWEB_HBBTV_RUNTIME_RESULT_ACCEPTED: return "accepted";
    case VDRWEB_HBBTV_RUNTIME_RESULT_INVALID_REQUEST: return "invalid_request";
    case VDRWEB_HBBTV_RUNTIME_RESULT_DISCOVERY_STALE: return "discovery_stale";
    case VDRWEB_HBBTV_RUNTIME_RESULT_APPLICATION_NOT_LAUNCHABLE:
      return "application_not_launchable";
    case VDRWEB_HBBTV_RUNTIME_RESULT_BUSY: return "busy";
    case VDRWEB_HBBTV_RUNTIME_RESULT_SESSION_NOT_ACTIVE:
      return "session_not_active";
    case VDRWEB_HBBTV_RUNTIME_RESULT_ACTION_UNSUPPORTED:
      return "action_unsupported";
    case VDRWEB_HBBTV_RUNTIME_RESULT_RUNTIME_UNAVAILABLE:
      return "runtime_unavailable";
  }
  return "unknown";
}

const char *RuntimeStateName(std::uint8_t state)
{
  switch (state) {
    case VDRWEB_HBBTV_RUNTIME_STATE_NONE: return "none";
    case VDRWEB_HBBTV_RUNTIME_STATE_STARTING: return "starting";
    case VDRWEB_HBBTV_RUNTIME_STATE_ACTIVE: return "active";
    case VDRWEB_HBBTV_RUNTIME_STATE_CLOSING: return "closing";
    case VDRWEB_HBBTV_RUNTIME_STATE_FAILED: return "failed";
  }
  return "unknown";
}

bool RuntimeOperation(
    const std::string &value,
    std::uint8_t &operation,
    const char *&name)
{
  if (strcasecmp(value.c_str(), "LAUNCH") == 0) {
    operation = VDRWEB_HBBTV_RUNTIME_LAUNCH;
    name = "launch";
    return true;
  }
  if (strcasecmp(value.c_str(), "STATUS") == 0) {
    operation = VDRWEB_HBBTV_RUNTIME_STATUS;
    name = "status";
    return true;
  }
  if (strcasecmp(value.c_str(), "INPUT") == 0) {
    operation = VDRWEB_HBBTV_RUNTIME_INPUT;
    name = "input";
    return true;
  }
  if (strcasecmp(value.c_str(), "CLOSE") == 0) {
    operation = VDRWEB_HBBTV_RUNTIME_CLOSE;
    name = "close";
    return true;
  }
  return false;
}

bool RuntimeInputAction(
    const std::string &value,
    std::uint8_t &action,
    std::string &canonical)
{
  struct Mapping {
    const char *name;
    std::uint8_t code;
  };
  static const Mapping mappings[] = {
      {"UP", VDRWEB_HBBTV_INPUT_UP},
      {"DOWN", VDRWEB_HBBTV_INPUT_DOWN},
      {"LEFT", VDRWEB_HBBTV_INPUT_LEFT},
      {"RIGHT", VDRWEB_HBBTV_INPUT_RIGHT},
      {"OK", VDRWEB_HBBTV_INPUT_OK},
      {"BACK", VDRWEB_HBBTV_INPUT_BACK},
      {"RED", VDRWEB_HBBTV_INPUT_RED},
      {"GREEN", VDRWEB_HBBTV_INPUT_GREEN},
      {"YELLOW", VDRWEB_HBBTV_INPUT_YELLOW},
      {"BLUE", VDRWEB_HBBTV_INPUT_BLUE},
      {"0", VDRWEB_HBBTV_INPUT_0},
      {"1", VDRWEB_HBBTV_INPUT_1},
      {"2", VDRWEB_HBBTV_INPUT_2},
      {"3", VDRWEB_HBBTV_INPUT_3},
      {"4", VDRWEB_HBBTV_INPUT_4},
      {"5", VDRWEB_HBBTV_INPUT_5},
      {"6", VDRWEB_HBBTV_INPUT_6},
      {"7", VDRWEB_HBBTV_INPUT_7},
      {"8", VDRWEB_HBBTV_INPUT_8},
      {"9", VDRWEB_HBBTV_INPUT_9},
      {"PLAY", VDRWEB_HBBTV_INPUT_PLAY},
      {"PAUSE", VDRWEB_HBBTV_INPUT_PAUSE},
      {"STOP", VDRWEB_HBBTV_INPUT_STOP},
      {"FAST_FORWARD", VDRWEB_HBBTV_INPUT_FAST_FORWARD},
      {"REWIND", VDRWEB_HBBTV_INPUT_REWIND},
  };

  for (const Mapping &mapping : mappings) {
    if (strcasecmp(value.c_str(), mapping.name) == 0) {
      action = mapping.code;
      canonical = mapping.name;
      return true;
    }
  }
  return false;
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

SuiteBridgeCommandResult HandleDiscovery(
    const ISuiteBridgeHbbtvProvider *provider,
    const char *option)
{
  if (provider == nullptr) return Rejected(550, "provider_unavailable");
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
  if (!provider->Discover(channelId, discovery, error)) {
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
  AppendJsonString(payload, DiscoveryResultName(discovery.result));
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
  if (result.payload.size() > kMaximumPayloadBytes)
    return Rejected(552, "hbbtv_discovery_payload_too_large");
  return result;
}

SuiteBridgeCommandResult HandleRuntime(
    const ISuiteBridgeHbbtvProvider *provider,
    const char *option)
{
  if (provider == nullptr) return Rejected(550, "provider_unavailable");
  if (option == nullptr) return Rejected(504, "hbbtv_runtime_request_invalid");

  std::istringstream input(option);
  std::string operationText;
  std::string schema;
  std::string sessionId;
  std::string channelId;
  std::uint64_t applicationId = 0;
  std::uint64_t descriptorRevision = 0;

  if (!(input >> operationText >> schema >> sessionId >> channelId >>
        applicationId >> descriptorRevision) ||
      schema != "1" ||
      applicationId == 0 ||
      applicationId > std::numeric_limits<std::uint32_t>::max() ||
      descriptorRevision == 0 ||
      !SafeIdentity(sessionId, VDRWEB_HBBTV_SESSION_ID_MAX) ||
      !SafeIdentity(channelId, VDRWEB_HBBTV_CHANNEL_ID_MAX)) {
    return Rejected(504, "hbbtv_runtime_request_invalid");
  }

  std::uint8_t operation = 0;
  const char *operationName = nullptr;
  if (!RuntimeOperation(operationText, operation, operationName))
    return Rejected(504, "hbbtv_runtime_operation_invalid");

  std::uint8_t inputAction = VDRWEB_HBBTV_INPUT_NONE;
  std::string canonicalAction;
  std::string extra;

  if (operation == VDRWEB_HBBTV_RUNTIME_INPUT) {
    std::string actionText;
    if (!(input >> actionText) ||
        !RuntimeInputAction(actionText, inputAction, canonicalAction) ||
        (input >> extra)) {
      return Rejected(504, "hbbtv_runtime_input_invalid");
    }
  }
  else if (input >> extra) {
    return Rejected(504, "hbbtv_runtime_request_invalid");
  }

  VdrWebHbbtvRuntimeV1 runtime{};
  runtime.structSize = sizeof(runtime);
  runtime.operation = operation;
  runtime.inputAction = inputAction;
  runtime.applicationId = static_cast<std::uint32_t>(applicationId);
  runtime.descriptorRevision = descriptorRevision;
  CopyText(runtime.sessionId, sessionId);
  CopyText(runtime.channelId, channelId);

  std::string error;
  if (!provider->Runtime(runtime, error))
    return Rejected(550, error.c_str());

  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = 250;

  std::ostringstream payload;
  payload << "{\"schemaVersion\":1"
          << ",\"provider\":\"vdr-plugin-web\""
          << ",\"providerSchemaVersion\":" << runtime.schemaVersion
          << ",\"capability\":\"broadcast.hbbtv.runtime\""
          << ",\"operation\":";
  AppendJsonString(payload, operationName);
  payload << ",\"result\":";
  AppendJsonString(payload, RuntimeResultName(runtime.result));
  payload << ",\"resultCode\":" << static_cast<unsigned int>(runtime.result)
          << ",\"state\":";
  AppendJsonString(payload, RuntimeStateName(runtime.state));
  payload << ",\"stateCode\":" << static_cast<unsigned int>(runtime.state)
          << ",\"sessionId\":";
  AppendJsonString(payload, sessionId);
  payload << ",\"channel\":";
  AppendJsonString(payload, channelId);
  payload << ",\"applicationId\":" << runtime.applicationId
          << ",\"descriptorRevision\":" << runtime.descriptorRevision;
  if (operation == VDRWEB_HBBTV_RUNTIME_INPUT) {
    payload << ",\"action\":";
    AppendJsonString(payload, canonicalAction);
  }
  payload << '}';

  result.payload = payload.str();
  if (result.payload.size() > kMaximumPayloadBytes)
    return Rejected(552, "hbbtv_runtime_payload_too_large");
  return result;
}

} // namespace

SuiteBridgeCommandResult SuiteBridgeHbbtvCommandService::Handle(
    const char *command,
    const char *option) const
{
  if (command == nullptr) return {};
  if (strcasecmp(command, "HBBAPPS") == 0)
    return HandleDiscovery(provider_, option);
  if (strcasecmp(command, "HBBRUN") == 0)
    return HandleRuntime(provider_, option);
  return {};
}
