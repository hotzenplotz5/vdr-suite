#include "suitebridge_recording_identity.h"

#include <cassert>
#include <string>

int main()
{
  const std::string simple =
      SuiteBridgeRecordingIdentity::KeyForNativeId(
          "/srv/vdr/video/Forrest_Gump/2026-07-20.20.15.1-0.rec");
  assert(simple == "c94d0eb9958a85079f81f059a436003c");
  assert(SuiteBridgeRecordingIdentity::IsValidKey(simple));

  const std::string unicode =
      SuiteBridgeRecordingIdentity::KeyForNativeId(
          "/srv/vdr/video/Filme/Die fabelhafte Welt der Amélie/% Aufnahme/2026.rec");
  assert(unicode == "901448858ce5ea8d9e990ca5227c3a6a");
  assert(unicode != simple);

  assert(SuiteBridgeRecordingIdentity::KeyForNativeId("").empty());
  assert(SuiteBridgeRecordingIdentity::KeyForNativeId(
      std::string(SuiteBridgeRecordingIdentity::kMaximumNativeIdBytes + 1, 'x')).empty());
  assert(SuiteBridgeRecordingIdentity::KeyForNativeId(
      std::string("bad\npath")).empty());
  assert(!SuiteBridgeRecordingIdentity::IsValidKey("ABC"));
  assert(!SuiteBridgeRecordingIdentity::IsValidKey(
      "0123456789abcdef0123456789abcdeG"));
  assert(!SuiteBridgeRecordingIdentity::IsValidKey(
      "0123456789ABCDEF0123456789ABCDEF"));
  return 0;
}
