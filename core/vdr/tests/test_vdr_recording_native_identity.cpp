#include "VdrRecordingNativeIdentity.h"

#include <cassert>
#include <string>

int main()
{
    const std::string key =
        VdrRecordingNativeIdentity::keyForNativeId(
            "/srv/vdr/video/Forrest_Gump/2026-07-20.20.15.1-0.rec");
    assert(key == "c94d0eb9958a85079f81f059a436003c");
    assert(VdrRecordingNativeIdentity::isValidKey(key));

    const std::string unicode =
        VdrRecordingNativeIdentity::keyForNativeId(
            "/srv/vdr/video/Filme/Die fabelhafte Welt der Amélie/% Aufnahme/2026.rec");
    assert(unicode == "901448858ce5ea8d9e990ca5227c3a6a");

    assert(VdrRecordingNativeIdentity::keyForNativeId("").empty());
    assert(VdrRecordingNativeIdentity::keyForNativeId(
        std::string(VdrRecordingNativeIdentity::MaximumNativeIdBytes + 1, 'x')).empty());
    assert(VdrRecordingNativeIdentity::keyForNativeId("bad\npath").empty());
    assert(!VdrRecordingNativeIdentity::isValidKey(
        "0123456789ABCDEF0123456789ABCDEF"));
    assert(!VdrRecordingNativeIdentity::isValidKey(
        "0123456789abcdef0123456789abcdeG"));
    return 0;
}
