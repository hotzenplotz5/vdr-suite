#include "suitebridge_osd_input.h"

#include <vdr/remote.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <limits>
#include <sstream>
#include <strings.h>
#include <vector>

namespace {

constexpr int SuccessReplyCode = 900;
constexpr int MalformedReplyCode = 501;
constexpr int ConflictReplyCode = 554;
constexpr int StaleReplyCode = 555;
constexpr int NativeRejectedReplyCode = 451;

bool safeIdentifier(const std::string &value)
{
  return !value.empty() && value.size() <= 128 &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
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
        (static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) -
         digit) / 10U) {
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
    if (values.size() > 12) return {};
    if (end == std::string::npos) break;
    position = end + 1;
  }
  return values;
}

bool actionKey(const std::string &action, eKeys &key)
{
  if (action == "up") key = kUp;
  else if (action == "down") key = kDown;
  else if (action == "left") key = kLeft;
  else if (action == "right") key = kRight;
  else if (action == "ok") key = kOk;
  else if (action == "back") key = kBack;
  else if (action == "red") key = kRed;
  else if (action == "green") key = kGreen;
  else if (action == "yellow") key = kYellow;
  else if (action == "blue") key = kBlue;
  else return false;
  return true;
}

SuiteBridgeCommandResult response(
    int replyCode,
    const std::string &category,
    const std::string &reason,
    const std::string &commandId)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = replyCode;
  std::ostringstream payload;
  payload << "{\"commandId\":\"" << commandId
          << "\",\"resultCategory\":\"" << category
          << "\",\"reasonCode\":\"" << reason
          << "\",\"nativeOperation\":\"legacy-osd.input\""
          << ",\"nativeOperationSchema\":1}";
  result.payload = payload.str();
  return result;
}

} // namespace

SuiteBridgeCommandResult SuiteBridgeOsdInputService::Handle(
    const char *command,
    const char *option,
    const SuiteBridgeOsdSnapshot &snapshot)
{
  if (command == nullptr || strcasecmp(command, "OSDINPUT") != 0) return {};

  const std::vector<std::string> values = split(option);
  if (values.size() != 11 || values[0] != "1")
    return response(MalformedReplyCode, "rejected", "legacy_osd_input_malformed", "");

  const std::string &commandId = values[1];
  const std::string &requestFingerprint = values[2];
  const std::string &surfaceId = values[4];
  const std::string &osdEpoch = values[5];
  const std::string &controllerLeaseId = values[6];
  const std::string &action = values[9];

  std::uint64_t backendGeneration = 0;
  std::uint64_t controllerLeaseEpoch = 0;
  std::uint64_t leaseRevision = 0;
  std::uint64_t deadline = 0;
  eKeys key = kNone;
  if (!safeIdentifier(commandId) ||
      !safeIdentifier(requestFingerprint) ||
      !unsignedValue(values[3], backendGeneration) ||
      !safeIdentifier(surfaceId) ||
      !safeIdentifier(osdEpoch) ||
      !safeIdentifier(controllerLeaseId) ||
      !unsignedValue(values[7], controllerLeaseEpoch) ||
      !unsignedValue(values[8], leaseRevision) ||
      !actionKey(action, key) ||
      !unsignedValue(values[10], deadline)) {
    return response(
        MalformedReplyCode, "rejected",
        "legacy_osd_input_malformed", commandId);
  }

  const RecentCommand *existing = Find(commandId);
  if (existing != nullptr) {
    if (existing->requestFingerprint != requestFingerprint)
      return response(
          ConflictReplyCode, "conflict",
          "legacy_osd_input_duplicate_conflict", commandId);
    return response(
        SuccessReplyCode, "duplicate",
        "legacy_osd_input_already_dispatched", commandId);
  }

  const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
  if (deadline <= static_cast<std::uint64_t>(now))
    return response(
        StaleReplyCode, "stale",
        "legacy_osd_input_deadline_expired", commandId);

  if (!snapshot.active || !snapshot.complete || !snapshot.consistent ||
      osdEpoch != snapshot.osdEpoch.data())
    return response(
        StaleReplyCode, "stale",
        "legacy_osd_input_osd_epoch_stale", commandId);

  if (!cRemote::Put(key))
    return response(
        NativeRejectedReplyCode, "native_rejected",
        "legacy_osd_input_native_rejected", commandId);

  Remember(commandId, requestFingerprint);
  return response(
      SuccessReplyCode, "dispatched_unverified",
      "legacy_osd_input_dispatched", commandId);
}

const SuiteBridgeOsdInputService::RecentCommand *
SuiteBridgeOsdInputService::Find(const std::string &commandId) const
{
  for (const RecentCommand &entry : recentCommands_) {
    if (entry.commandId == commandId) return &entry;
  }
  return nullptr;
}

void SuiteBridgeOsdInputService::Remember(
    const std::string &commandId,
    const std::string &requestFingerprint)
{
  recentCommands_[recentCommandIndex_] = {commandId, requestFingerprint};
  recentCommandIndex_ =
      (recentCommandIndex_ + 1U) % recentCommands_.size();
}
