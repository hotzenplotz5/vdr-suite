#include "suitebridge_native_timer_preview_core.h"

#include <cassert>
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
}
