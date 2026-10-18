#include "suitebridge_native_timer_preview_vdr.h"

#include <vdr/channels.h>
#include <vdr/device.h>
#include <vdr/timers.h>

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <limits>
#include <string>

namespace {
constexpr int NativeReadLockTimeoutMs = 250;
constexpr std::size_t MaximumNativeTimers = 4096;
constexpr int MaximumNativeDevices = 64;

bool validLocalProjection(const SuiteBridgeNativeTimerPreviewVdrRequest& r)
{
  // Project the exact minute-rounded one-shot boundaries in the owning VDR
  // process. Do not use the Suite Daemon's TZ or trust a Device projection.
  if (!r.enabled || r.vps || r.weekdays != "-------" ||
      r.unixStart <= 0 || r.unixStop <= r.unixStart ||
      r.unixStop - r.unixStart >= 24 * 3600 ||
      r.unixStop > std::numeric_limits<std::int64_t>::max() - 60 ||
      r.priority < 0 || r.priority > 99)
    return false;
  const std::int64_t roundedStart =
      r.unixStart - r.unixStart % 60;
  const std::int64_t roundedStop =
      r.unixStop + (60 - r.unixStop % 60) % 60;
  if (roundedStop <= roundedStart ||
      roundedStop - roundedStart >= 24 * 3600)
    return false;
  const time_t nativeStart = static_cast<time_t>(roundedStart);
  const time_t nativeStop = static_cast<time_t>(roundedStop);
  if (static_cast<std::int64_t>(nativeStart) != roundedStart ||
      static_cast<std::int64_t>(nativeStop) != roundedStop)
    return false;
  std::tm start{}, stop{};
  if (!localtime_r(&nativeStart, &start) ||
      !localtime_r(&nativeStop, &stop) ||
      start.tm_gmtoff != stop.tm_gmtoff ||
      start.tm_year + 1900 < 1970 ||
      start.tm_year + 1900 > 9999 ||
      stop.tm_year + 1900 > 9999)
    return false;
  char day[16]{}, startHhmm[8]{}, stopHhmm[8]{};
  if (std::snprintf(day, sizeof(day), "%04d-%02d-%02d",
          start.tm_year + 1900, start.tm_mon + 1, start.tm_mday) != 10 ||
      std::snprintf(startHhmm, sizeof(startHhmm), "%02d%02d",
          start.tm_hour, start.tm_min) != 4 ||
      std::snprintf(stopHhmm, sizeof(stopHhmm), "%02d%02d",
          stop.tm_hour, stop.tm_min) != 4)
    return false;
  return r.day == day && r.startTime == startHhmm &&
      r.endTime == stopHhmm;
}
} // namespace

vdrsuite_native_timer_preview::NativeSnapshot
SuiteBridgeCaptureNativeTimerPreviewVdr(
    const SuiteBridgeNativeTimerPreviewVdrRequest& request)
{
  using namespace vdrsuite_native_timer_preview;
  NativeSnapshot result;
  result.proposedStart = request.unixStart;
  result.proposedStop = request.unixStop;
  result.proposedOneShot = validLocalProjection(request);
  if (!result.proposedOneShot) return result;

  // Never take a VDR write lock or add a candidate to the live Timer list.
  // Collect the entire Timer inventory and then verify its generation still
  // matches after the separate channel/device query.
  cStateKey timerKey;
  const cTimers* timers = cTimers::GetTimersRead(
      timerKey, NativeReadLockTimeoutMs);
  if (!timers) return result;
  bool inventoryComplete = true;
  for (const cTimer* t = timers->First(); t; t = timers->Next(t)) {
    if (result.existingTimers.size() >= MaximumNativeTimers) {
      inventoryComplete = false;
      break;
    }
    ExistingTimer observed;
    observed.active = t->HasFlags(tfActive);
    if (observed.active && t->Local() && t->WeekDays() == 0 &&
        !t->HasFlags(tfVps)) {
      time_t start = 0, stop = 0;
      t->CalcStartStopTime(start, stop);
      if (start > 0 && stop > start) {
        observed.start = static_cast<std::int64_t>(start);
        observed.stop = static_cast<std::int64_t>(stop);
        observed.exactOneShot = true;
      }
    }
    result.existingTimers.push_back(observed);
  }
  timerKey.Remove();
  if (!inventoryComplete) return result;

  cStateKey channelKey;
  const cChannels* channels = cChannels::GetChannelsRead(
      channelKey, NativeReadLockTimeoutMs);
  if (!channels) return result;
  const tChannelID nativeId =
      tChannelID::FromString(request.channelId.c_str());
  const cChannel* channel = nativeId.Valid()
      ? channels->GetByChannelID(nativeId)
      : nullptr;
  if (channel) {
    const cString exactId = channel->GetChannelID().ToString();
    const char* const canonicalId = *exactId;
    result.proposedChannelVerified =
        canonicalId != nullptr && request.channelId == canonicalId;
    if (result.proposedChannelVerified) {
      result.proposedFreeToAir = channel->Ca(0) == CA_FTA;
      const int count = cDevice::NumDevices();
      if (count > 0 && count <= MaximumNativeDevices) {
        // Do not use VDR's channel-based GetDevice selector: even its
        // Query=true path may release a CAM slot. cDvbDevice::ProvidesChannel
        // also writes the internal bonded-receiver flag despite being const.
        // ProvidesTransponder is a narrower capability query; require an
        // actually idle, non-bonded, free-to-air device before suggesting
        // an isolated candidate. No tuning, CAM changes or detachments.
        bool complete = true;
        for (int index = 0; index < count; ++index) {
          const cDevice* device = cDevice::GetDevice(index);
          if (!device) { complete = false; break; }
          Device observed;
          observed.enumerated = true;
          observed.supportsProposedChannel =
              device->ProvidesTransponder(channel);
          observed.availableWithoutPreemptingRecording =
              observed.supportsProposedChannel &&
              !device->Receiving() && !device->Occupied() &&
              !device->IsBonded() &&
              device->Priority(true) <= IDLEPRIORITY;
          result.devices.push_back(observed);
        }
        result.allDevicesEnumerated = complete;
      }
    }
  }
  channelKey.Remove();

  // Re-use the SAME state key: a non-null read means the timer inventory
  // changed while inspecting devices. A timeout is also indeterminate.
  const cTimers* changed = cTimers::GetTimersRead(
      timerKey, NativeReadLockTimeoutMs);
  if (changed) timerKey.Remove();
  result.allTimersEnumerated = !changed && !timerKey.TimedOut();
  return result;
}
