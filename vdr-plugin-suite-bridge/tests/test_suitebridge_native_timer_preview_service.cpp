#include "suitebridge_native_timer_preview_service.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>

using namespace vdrsuite_native_timer_preview;

namespace {
std::string hex(const std::string& value)
{
  if (value.empty()) return "-";
  std::string output;
  const char* alphabet = "0123456789abcdef";
  for (unsigned char c : value) {
    output += alphabet[(c >> 4) & 15];
    output += alphabet[c & 15];
  }
  return output;
}
void field(std::string& output, const std::string& s)
{
  output += std::to_string(s.size()) + ":" + s + "|";
}
std::string wire(const std::string& epoch = "pie_test",
                 const std::string& channel = "S19.2E-1-1019-10301",
                 const std::string& day = "2026-10-17")
{
  const std::string title = "News", directory = "TV";
  const std::string days = "-------", startTime = "0740",
                    endTime = "0840";
  std::string f = "native-timer-specification/1|";
  for (const auto& item : {channel, title, directory, day, days,
                           startTime, endTime,
                           std::string("50"), std::string("99"),
                           std::string("1"), std::string("0")})
    field(f, item);
  const std::string sha =
      vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(f);
  return "CHECK 1 " + epoch + " 1792222800 1792226400 " +
      hex(channel) + " " + hex(title) + " " + hex(directory) + " " +
      hex(day) + " " + hex(days) + " " + hex(startTime) + " " +
      hex(endTime) + " 50 99 1 0 " + sha;
}

NativeSnapshot snapshot(const SuiteBridgeNativeTimerPreviewVdrRequest& request)
{
  NativeSnapshot s;
  s.proposedStart = request.unixStart;
  s.proposedStop = request.unixStop;
  s.proposedOneShot = true;
  s.proposedChannelVerified = true;
  s.proposedFreeToAir = true;
  s.allTimersEnumerated = true;
  s.allDevicesEnumerated = true;
  s.existingTimers.push_back({true, true, 1792210000, 1792215000});
  s.devices.push_back({true, true, true});
  return s;
}
}

int main()
{
  SuiteBridgeNativeTimerPreviewService disabled("pie_test");
  const std::string good = wire();
  auto r = disabled.Handle("NTPREVIEW", good.c_str());
  assert(r.handled && r.replyCode == 550);
  assert(r.payload.find("native_unavailable") != std::string::npos);
  assert(!disabled.Handle("NTCREATE", good.c_str()).handled);

  SuiteBridgeNativeTimerPreviewService service("pie_test", snapshot);
  r = service.Handle("NTPREVIEW", good.c_str());
  assert(r.handled && r.replyCode == 900);
  assert(r.payload.find("vdr-suite-ntpreview/1 ") == 0);
  assert(r.payload.find(" isolated_clear") != std::string::npos);
  assert(r.payload.find(" pie_test ") != std::string::npos);

  r = service.Handle("NTPREVIEW", wire("pie_other").c_str());
  assert(r.replyCode == 501 && r.payload.find("stale") != std::string::npos);
  r = service.Handle("NTPREVIEW", wire("pie_test", "bad.channel").c_str());
  assert(r.replyCode == 900); // digest matches; native reader decides channel
  r = service.Handle("NTPREVIEW", (good + " extra").c_str());
  assert(r.replyCode == 501);

  std::string tampered = good;
  tampered.back() = tampered.back() == 'a' ? 'b' : 'a';
  r = service.Handle("NTPREVIEW", tampered.c_str());
  assert(r.replyCode == 501);
  r = service.Handle("NTPREVIEW", "CHECK 1 pie_test 1 2 00");
  assert(r.replyCode == 501);

  SuiteBridgeNativeTimerPreviewService overlap(
      "pie_test", [](const SuiteBridgeNativeTimerPreviewVdrRequest& request) {
        NativeSnapshot s = snapshot(request);
        s.existingTimers.push_back({true, true, request.unixStart,
                                    request.unixStop});
        return s;
      });
  r = overlap.Handle("NTPREVIEW", good.c_str());
  assert(r.replyCode == 550);
  assert(r.payload.find("active_timer_overlap_not_simulated") !=
      std::string::npos);

  SuiteBridgeNativeTimerPreviewService fault(
      "pie_test", [](const SuiteBridgeNativeTimerPreviewVdrRequest&)
          -> NativeSnapshot { throw std::runtime_error("bad"); });
  r = fault.Handle("NTPREVIEW", good.c_str());
  assert(r.replyCode == 550);
  assert(r.payload.find("native_capture_failed") != std::string::npos);
}
