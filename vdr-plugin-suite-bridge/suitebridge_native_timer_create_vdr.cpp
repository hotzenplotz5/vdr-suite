#include "suitebridge_native_timer_create_vdr.h"

#include <vdr/timers.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>
#include <string>

namespace {
constexpr int TimerWriteLockTimeoutMs = 1000;

SuiteBridgeNativeTimerCreateMutationResult result(
    SuiteBridgeNativeTimerCreateMutationDisposition disposition,
    const std::string &evidence)
{
  return {disposition, evidence};
}

std::string evidence(const char *reason, const std::string &commandId)
{
  return std::string("ntcreate:vdr:") + reason + ":" + commandId;
}

bool digits(const std::string &value)
{
  return !value.empty() &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isdigit(character) != 0;
      });
}

bool validTime(const std::string &value)
{
  if (value.size() != 4 || !digits(value)) return false;
  const int hour = (value[0] - '0') * 10 + value[1] - '0';
  const int minute = (value[2] - '0') * 10 + value[3] - '0';
  return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

bool validDay(const std::string &value)
{
  if (value.size() != 10 || value[4] != '-' || value[7] != '-') return false;
  for (std::size_t index = 0; index < value.size(); ++index)
    if (index != 4 && index != 7 &&
        std::isdigit(static_cast<unsigned char>(value[index])) == 0)
      return false;
  return true;
}

bool validWeekdays(const std::string &value)
{
  static constexpr char Names[7] = {'M', 'T', 'W', 'T', 'F', 'S', 'S'};
  if (value.size() != 7) return false;
  for (std::size_t index = 0; index < value.size(); ++index)
    if (value[index] != '-' && value[index] != Names[index]) return false;
  return true;
}

bool safeChannelId(const std::string &value)
{
  return !value.empty() && value.size() <= 192 &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '.' ||
            character == '-' || character == '_';
      });
}

std::string timerDay(
    const SuiteBridgeNativeTimerCreateRequest &request,
    bool &valid)
{
  valid = validWeekdays(request.weekdays);
  if (!valid) return {};
  const bool repeating =
      request.weekdays.find_first_not_of('-') != std::string::npos;
  if (repeating) {
    if (!request.day.empty() && !validDay(request.day)) {
      valid = false;
      return {};
    }
    return request.weekdays +
        (request.day.empty() ? std::string() : "@" + request.day);
  }
  valid = validDay(request.day);
  return valid ? request.day : std::string();
}

std::string timerFile(
    const SuiteBridgeNativeTimerCreateRequest &request,
    bool &valid)
{
  valid = !request.title.empty() &&
      request.title.find('~') == std::string::npos &&
      request.title.find('|') == std::string::npos &&
      request.directory.find('|') == std::string::npos;
  if (!valid) return {};
  std::string value = request.directory.empty()
      ? request.title
      : request.directory + "~" + request.title;
  std::replace(value.begin(), value.end(), ':', '|');
  return value;
}
} // namespace

SuiteBridgeNativeTimerCreateMutationResult
SuiteBridgeNativeTimerCreateVdrMutationCallback::CreateTimer(
    const SuiteBridgeNativeTimerCreateRequest &request)
{
  bool valid = false;
  const std::string day = timerDay(request, valid);
  if (!valid || !safeChannelId(request.channelId) ||
      !validTime(request.startTime) || !validTime(request.endTime) ||
      request.priority > 99 || request.lifetime > 99)
    return result(
        SuiteBridgeNativeTimerCreateMutationDisposition::RejectedWithoutEffect,
        evidence("invalid-specification", request.commandId));

  const std::string file = timerFile(request, valid);
  if (!valid)
    return result(
        SuiteBridgeNativeTimerCreateMutationDisposition::RejectedWithoutEffect,
        evidence("invalid-file", request.commandId));

  const unsigned flags =
      (request.enabled ? tfActive : tfNone) |
      (request.vps ? tfVps : tfNone);
  std::ostringstream definition;
  definition << flags << ':' << request.channelId << ':' << day << ':'
             << request.startTime << ':' << request.endTime << ':'
             << request.priority << ':' << request.lifetime << ':'
             << file << ':';

  std::unique_ptr<cTimer> timer(new cTimer());
  if (!timer->Parse(definition.str().c_str()))
    return result(
        SuiteBridgeNativeTimerCreateMutationDisposition::RejectedWithoutEffect,
        evidence("parse-rejected", request.commandId));

  try {
    cStateKey stateKey;
    cTimers *timers =
        cTimers::GetTimersWrite(stateKey, TimerWriteLockTimeoutMs);
    if (timers == nullptr)
      return result(
          SuiteBridgeNativeTimerCreateMutationDisposition::RejectedWithoutEffect,
          evidence("lock-unavailable", request.commandId));

    timers->SetExplicitModify();
    timers->Add(timer.get());
    timer.release();
    timers->SetModified();
    stateKey.Remove();
    return result(
        SuiteBridgeNativeTimerCreateMutationDisposition::AppliedUnverified,
        evidence("created-unverified", request.commandId));
  } catch (...) {
    return result(
        SuiteBridgeNativeTimerCreateMutationDisposition::OutcomeUnknown,
        evidence("exception", request.commandId));
  }
}
