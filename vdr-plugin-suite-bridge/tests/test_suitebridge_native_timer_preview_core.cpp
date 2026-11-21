#include "suitebridge_native_timer_preview_core.h"
#include "suitebridge_native_timer_wallclock.h"

#include <cassert>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace vdrsuite_native_timer_preview;

NativeSnapshot isolated()
{
  NativeSnapshot s;
  s.proposedStart = 1792222800;
  s.proposedStop = 1792226400;
  s.proposedOneShot = true;
  s.proposedChannelVerified = true;
  s.proposedFreeToAir = true;
  s.allTimersEnumerated = true;
  s.allDevicesEnumerated = true;
  s.existingTimers.push_back({false, false, 0, 0});
  s.existingTimers.push_back({true, true, 1792210000, 1792215000});
  s.devices.push_back({true, false, false});
  s.devices.push_back({true, true, true});
  return s;
}

int main()
{
  // Isolated process-local timezone fixture, restored before returning.
  const char* priorTz = std::getenv("TZ");
  const bool hadTz = priorTz != nullptr;
  const std::string originalTz = hadTz ? priorTz : "";
  assert(setenv("TZ", "Europe/Berlin", 1) == 0);
  tzset();
  const std::time_t foldFirst = 1792888200;  // 02:30 CEST
  const std::time_t foldSecond = 1792891800; // 02:30 CET
  const std::time_t normal = 1792309200;     // ordinary 09:40 CEST
  std::tm a{}, b{}, regular{};
  assert(localtime_r(&foldFirst, &a));
  assert(localtime_r(&foldSecond, &b));
  assert(localtime_r(&normal, &regular));
  assert(!suitebridge_native_timer_wallclock::uniquelyRepresentsMinute(
      foldFirst, a));
  assert(!suitebridge_native_timer_wallclock::uniquelyRepresentsMinute(
      foldSecond, b));
  assert(suitebridge_native_timer_wallclock::uniquelyRepresentsMinute(
      normal, regular));
  assert((hadTz ? setenv("TZ", originalTz.c_str(), 1) :
                  unsetenv("TZ")) == 0);
  tzset();

  std::int64_t minuteStart = 0, minuteStop = 0;
  assert(nativeMinuteExpandedWindow(
      1792222819, 1792226391, minuteStart, minuteStop));
  assert(minuteStart == 1792222800);
  assert(minuteStop == 1792226400);
  // The final seconds of a preceding Timer still overlap VDR's expanded
  // starting minute even when they precede the EPG event's raw start.
  auto adjacent = isolated();
  adjacent.proposedStart = minuteStart;
  adjacent.proposedStop = minuteStop;
  adjacent.existingTimers.push_back(
      {true, true, minuteStart - 100, minuteStart + 1});
  assert(std::string(evaluate(adjacent).reason) ==
      "active_timer_overlap_not_simulated");
  assert(!nativeMinuteExpandedWindow(
      1792222800, 1792222800, minuteStart, minuteStop));

  const auto eligible = isolated();
  const auto decision = evaluate(eligible);
  assert(decision.disposition == Disposition::candidateClear);
  assert(decision.consideredTimers == 2);
  assert(decision.consideredDevices == 2);
  assert(std::string(decision.reason) ==
      "isolated_one_shot_native_device_candidate_clear");

  auto s = isolated();
  s.allTimersEnumerated = false;
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.allDevicesEnumerated = false;
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.proposedFreeToAir = false;
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.proposedOneShot = false;
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.proposedStop = s.proposedStart + 24 * 3600;
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.existingTimers.push_back({true, true, 1792220000, 1792224000});
  assert(std::string(evaluate(s).reason) ==
      "active_timer_overlap_not_simulated");
  s = isolated();
  s.existingTimers.push_back({true, false, 0, 0});
  assert(std::string(evaluate(s).reason) ==
      "active_timer_schedule_indeterminate");
  s = isolated();
  s.devices[1].availableWithoutPreemptingRecording = false;
  assert(std::string(evaluate(s).reason) ==
      "no_proven_nonpreempting_native_device");
  s = isolated();
  s.devices[1].enumerated = false;
  assert(std::string(evaluate(s).reason) ==
      "device_inventory_incomplete");
  s = isolated();
  s.existingTimers[1].start = 0;
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.devices.clear();
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.existingTimers.resize(4097);
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.proposedChannelVerified = false;
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.proposedStart = 0;
  assert(evaluate(s).disposition == Disposition::unknown);
  s = isolated();
  s.proposedStop = s.proposedStart;
  assert(evaluate(s).disposition == Disposition::unknown);
}
