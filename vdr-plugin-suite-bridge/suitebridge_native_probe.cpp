#include "suitebridge_native_probe.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <strings.h>
#include <utility>
#include <vector>

namespace {
constexpr const char *NativeOperation = "vdr.native.probe";
constexpr std::uint64_t NativeOperationSchema = 1;
constexpr int SuccessReplyCode = 900;
constexpr int MalformedReplyCode = 501;
constexpr int UnsupportedReplyCode = 504;
constexpr int ConflictReplyCode = 554;
constexpr int StaleReplyCode = 555;
constexpr int MissingReplyCode = 550;
constexpr int CapacityReplyCode = 451;

bool safeIdentifier(const std::string &value)
{
  if (value.empty() || value.size() > 128) return false;
  return std::all_of(value.begin(), value.end(), [](unsigned char character) {
    return std::isalnum(character) != 0 || character == '-' ||
        character == '_' || character == '.' || character == ':';
  });
}

bool unsignedValue(const std::string &value, std::uint64_t &parsed)
{
  if (value.empty() || value.size() > 19) return false;
  parsed = 0;
  for (unsigned char character : value) {
    if (character < '0' || character > '9') return false;
    const unsigned digit = static_cast<unsigned>(character - '0');
    if (parsed >
        (static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - digit) / 10U) {
      return false;
    }
    parsed = parsed * 10U + digit;
  }
  return parsed > 0;
}

std::vector<std::string> split(const char *option)
{
  std::vector<std::string> values;
  if (option == nullptr) return values;
  const std::string input(option);
  std::size_t position = 0;
  while (position < input.size()) {
    while (position < input.size() && input[position] == ' ') ++position;
    if (position == input.size()) break;
    const std::size_t end = input.find(' ', position);
    values.push_back(input.substr(
        position, end == std::string::npos ? std::string::npos : end - position));
    if (values.size() > 20) return {};
    if (end == std::string::npos) break;
    position = end + 1;
  }
  return values;
}

std::string escape(const std::string &value)
{
  std::ostringstream output;
  for (unsigned char character : value) {
    switch (character) {
      case '"': output << "\\\""; break;
      case '\\': output << "\\\\"; break;
      default:
        if (character >= 0x20U && character != 0x7fU) {
          output << static_cast<char>(character);
        }
    }
  }
  return output.str();
}

std::int64_t nowSeconds()
{
  return static_cast<std::int64_t>(std::time(nullptr));
}

void appendIdentity(std::ostringstream &output, const std::string &value)
{
  output << value.size() << ':' << value << '|';
}

void appendIdentity(std::ostringstream &output, std::uint64_t value)
{
  output << value << '|';
}

template <typename Request>
std::string requestIdentity(const Request &request)
{
  std::ostringstream output;
  appendIdentity(output, request.commandId);
  appendIdentity(output, request.requestFingerprint);
  appendIdentity(output, request.operationId);
  appendIdentity(output, request.jobId);
  appendIdentity(output, request.attemptId);
  appendIdentity(output, request.claimEpoch);
  appendIdentity(output, request.backendId);
  appendIdentity(output, request.agentId);
  appendIdentity(output, request.agentInstanceId);
  appendIdentity(output, request.backendGeneration);
  appendIdentity(output, request.pluginInstanceEpoch);
  appendIdentity(output, request.probeNonce);
  return output.str();
}

SuiteBridgeCommandResult rejection(
    int replyCode,
    const std::string &reasonCode,
    const std::string &receiptCategory,
    const std::string &pluginInstanceEpoch)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = replyCode;
  std::ostringstream payload;
  payload << "{\"reasonCode\":\"" << reasonCode
          << "\",\"receiptCategory\":\"" << receiptCategory
          << "\",\"nativeOperation\":\"" << NativeOperation
          << "\",\"nativeOperationSchema\":" << NativeOperationSchema
          << ",\"pluginInstanceEpoch\":\"" << escape(pluginInstanceEpoch)
          << "\",\"mutationsState\":\"disabled\"}";
  result.payload = payload.str();
  return result;
}

template <typename ReceiptEntry>
std::string evidenceJson(
    const ReceiptEntry &entry,
    const std::string &receiptCategory,
    bool readback)
{
  std::ostringstream payload;
  payload << "{\"commandId\":\"" << escape(entry.request.commandId)
          << "\",\"requestFingerprint\":\""
          << escape(entry.request.requestFingerprint)
          << "\",\"nativeOperation\":\"" << NativeOperation
          << "\",\"nativeOperationSchema\":" << NativeOperationSchema
          << ",\"pluginInstanceEpoch\":\""
          << escape(entry.request.pluginInstanceEpoch)
          << "\",\"nativeExecutionSequence\":"
          << entry.nativeExecutionSequence
          << ",\"receiptCategory\":\"" << receiptCategory
          << "\",\"acceptedAt\":" << entry.acceptedAt
          << ",\"sideEffectClass\":\"none\""
          << ",\"resultCategory\":\"succeeded\""
          << ",\"vdrActive\":" << (entry.vdrActive ? "true" : "false")
          << ",\"mutationsState\":\"disabled\""
          << ",\"sideEffectObserved\":false"
          << ",\"boundedDiagnostics\":\"native probe completed\""
          << ",\"completedAt\":" << entry.completedAt;
  if (readback) {
    payload << ",\"readbackCategory\":\"verified\""
            << ",\"duplicateDisposition\":\"exact_replay\"";
  }
  payload << '}';
  return payload.str();
}

template <typename Request>
bool parseExecute(
    const char *option,
    Request &request)
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 17 || values[0] != "EXEC" ||
      values[1] != "vdr-suite-native/1" ||
      values[2] != "vdr.native.probe" || values[3] != "1") {
    return false;
  }
  request.commandId = values[4];
  request.requestFingerprint = values[5];
  request.operationId = values[6];
  request.jobId = values[7];
  request.attemptId = values[8];
  request.backendId = values[10];
  request.agentId = values[11];
  request.agentInstanceId = values[12];
  request.pluginInstanceEpoch = values[14];
  request.probeNonce = values[16];
  return unsignedValue(values[9], request.claimEpoch) &&
      unsignedValue(values[13], request.backendGeneration) &&
      values[15] == "1" &&
      safeIdentifier(request.commandId) &&
      safeIdentifier(request.requestFingerprint) &&
      safeIdentifier(request.operationId) &&
      safeIdentifier(request.jobId) &&
      safeIdentifier(request.attemptId) &&
      safeIdentifier(request.backendId) &&
      safeIdentifier(request.agentId) &&
      safeIdentifier(request.agentInstanceId) &&
      safeIdentifier(request.pluginInstanceEpoch) &&
      safeIdentifier(request.probeNonce);
}

bool parseReadback(
    const char *option,
    std::string &commandId,
    std::string &requestFingerprint,
    std::string &pluginInstanceEpoch,
    std::uint64_t &nativeExecutionSequence)
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 6 || values[0] != "READ" || values[1] != "1") {
    return false;
  }
  commandId = values[2];
  requestFingerprint = values[3];
  pluginInstanceEpoch = values[4];
  return safeIdentifier(commandId) && safeIdentifier(requestFingerprint) &&
      safeIdentifier(pluginInstanceEpoch) &&
      unsignedValue(values[5], nativeExecutionSequence);
}
} // namespace

std::string GenerateSuiteBridgePluginInstanceEpoch()
{
  std::random_device random;
  std::ostringstream output;
  output << "pie_" << std::hex << std::setfill('0');
  for (int index = 0; index < 4; ++index) {
    output << std::setw(8) << static_cast<std::uint32_t>(random());
  }
  return output.str();
}

SuiteBridgeNativeProbeService::SuiteBridgeNativeProbeService(
    std::string pluginInstanceEpoch)
    : pluginInstanceEpoch_(std::move(pluginInstanceEpoch))
{
  if (!safeIdentifier(pluginInstanceEpoch_)) {
    pluginInstanceEpoch_ = GenerateSuiteBridgePluginInstanceEpoch();
  }
}

const std::string &SuiteBridgeNativeProbeService::PluginInstanceEpoch() const noexcept
{
  return pluginInstanceEpoch_;
}

SuiteBridgeCommandResult SuiteBridgeNativeProbeService::Handle(
    const char *command,
    const char *option,
    const VdrActiveProbe &vdrActiveProbe)
{
  if (command == nullptr) return {};
  if (strcasecmp(command, "NCAP") == 0) return capability(option);
  if (strcasecmp(command, "NPROBE") != 0) return {};
  const std::vector<std::string> values = split(option);
  if (values.empty()) {
    return rejection(MalformedReplyCode, "native_probe_malformed", "rejected", pluginInstanceEpoch_);
  }
  if (values.front() == "EXEC") return execute(option, vdrActiveProbe);
  if (values.front() == "READ") return readback(option);
  return rejection(UnsupportedReplyCode, "native_probe_unsupported", "unsupported", pluginInstanceEpoch_);
}

SuiteBridgeCommandResult SuiteBridgeNativeProbeService::capability(
    const char *option) const
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 1 || values.front() != "1") {
    return rejection(UnsupportedReplyCode, "native_capability_schema_unsupported", "unsupported", pluginInstanceEpoch_);
  }
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = SuccessReplyCode;
  std::ostringstream payload;
  payload << "{\"nativeOperation\":\"" << NativeOperation
          << "\",\"nativeOperationSchema\":" << NativeOperationSchema
          << ",\"sideEffectClass\":\"none\""
          << ",\"mutations\":\"disabled\""
          << ",\"localProviderKind\":\"suitebridge\""
          << ",\"pluginInstanceEpoch\":\""
          << escape(pluginInstanceEpoch_) << "\"}";
  result.payload = payload.str();
  return result;
}

SuiteBridgeCommandResult SuiteBridgeNativeProbeService::execute(
    const char *option,
    const VdrActiveProbe &vdrActiveProbe)
{
  Request request;
  if (!parseExecute(option, request)) {
    return rejection(MalformedReplyCode, "native_probe_malformed", "rejected", pluginInstanceEpoch_);
  }
  if (request.pluginInstanceEpoch != pluginInstanceEpoch_) {
    return rejection(StaleReplyCode, "plugin_instance_epoch_stale", "stale", pluginInstanceEpoch_);
  }

  const std::string identity = requestIdentity(request);
  std::lock_guard<std::mutex> lock(mutex_);
  if (ReceiptEntry *existing = find(request.commandId)) {
    if (existing->requestIdentity != identity) {
      return rejection(ConflictReplyCode, "native_probe_conflict", "conflict", pluginInstanceEpoch_);
    }
    SuiteBridgeCommandResult result;
    result.handled = true;
    result.replyCode = SuccessReplyCode;
    result.payload = evidenceJson(*existing, "duplicate", false);
    return result;
  }
  if (nativeExecutionSequence_ ==
      static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
    return rejection(ConflictReplyCode, "native_sequence_exhausted", "rejected", pluginInstanceEpoch_);
  }

  ReceiptEntry *entry = reserve();
  if (entry == nullptr) {
    return rejection(
        CapacityReplyCode,
        "native_receipt_capacity_exhausted",
        "rejected",
        pluginInstanceEpoch_);
  }
  entry->occupied = true;
  entry->request = std::move(request);
  entry->requestIdentity = identity;
  entry->nativeExecutionSequence = ++nativeExecutionSequence_;
  entry->acceptedAt = nowSeconds();
  entry->vdrActive = vdrActiveProbe ? vdrActiveProbe() : false;
  entry->completedAt = nowSeconds();

  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = SuccessReplyCode;
  result.payload = evidenceJson(*entry, "accepted", false);
  return result;
}

SuiteBridgeCommandResult SuiteBridgeNativeProbeService::readback(
    const char *option) const
{
  std::string commandId;
  std::string requestFingerprint;
  std::string pluginInstanceEpoch;
  std::uint64_t sequence = 0;
  if (!parseReadback(
          option, commandId, requestFingerprint,
          pluginInstanceEpoch, sequence)) {
    return rejection(MalformedReplyCode, "native_readback_malformed", "rejected", pluginInstanceEpoch_);
  }
  if (pluginInstanceEpoch != pluginInstanceEpoch_) {
    return rejection(StaleReplyCode, "plugin_instance_epoch_stale", "stale", pluginInstanceEpoch_);
  }
  std::lock_guard<std::mutex> lock(mutex_);
  const ReceiptEntry *entry = find(commandId);
  if (entry == nullptr) {
    return rejection(MissingReplyCode, "native_receipt_not_found", "rejected", pluginInstanceEpoch_);
  }
  if (entry->request.requestFingerprint != requestFingerprint ||
      entry->nativeExecutionSequence != sequence) {
    return rejection(ConflictReplyCode, "native_readback_conflict", "conflict", pluginInstanceEpoch_);
  }
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = SuccessReplyCode;
  result.payload = evidenceJson(*entry, "duplicate", true);
  return result;
}

SuiteBridgeNativeProbeService::ReceiptEntry *
SuiteBridgeNativeProbeService::find(const std::string &commandId)
{
  for (ReceiptEntry &entry : receipts_) {
    if (entry.occupied && entry.request.commandId == commandId) return &entry;
  }
  return nullptr;
}

const SuiteBridgeNativeProbeService::ReceiptEntry *
SuiteBridgeNativeProbeService::find(const std::string &commandId) const
{
  for (const ReceiptEntry &entry : receipts_) {
    if (entry.occupied && entry.request.commandId == commandId) return &entry;
  }
  return nullptr;
}

SuiteBridgeNativeProbeService::ReceiptEntry *SuiteBridgeNativeProbeService::reserve()
{
  for (ReceiptEntry &entry : receipts_) {
    if (!entry.occupied) return &entry;
  }
  return nullptr;
}
