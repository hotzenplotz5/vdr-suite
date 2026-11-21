#include "RecordingTrickplayDeviceRequest.h"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    const RecordingTrickplayDeviceRequestParser parser;
    const std::string root =
        "/api/v1/recording-playback-sessions/";
    const std::string id = "ms_0123456789abcdef0123456789abcdef";
    const auto uri = [&](const std::string& seconds) {
        return root + id + "/trickplay?seconds=" + seconds;
    };
    {
        const auto zero = parser.parse(uri("0"));
        const auto latest = parser.parse(uri("43199"));
        assert(zero.valid && zero.sessionId == id);
        assert(zero.positionSeconds == 0);
        assert(latest.valid && latest.positionSeconds == 43199);
        assert(latest.reasonCode.empty());
    }
    for (const std::string& suffix : {
        "", "00", "01", "-1", "+1", "1.0", "43200", "999999999",
        "1&backendId=other", "1#bad", "1/../etc/passwd", "%31",
        "1%00", " 1", "1 ", "1?seconds=2", "1;abc"}) {
        const auto invalid = parser.parse(uri(suffix));
        assert(!invalid.valid);
        assert(invalid.sessionId.empty());
    }
    {
        auto badId = id;
        badId[4] = 'Z';
        assert(!parser.parse(root + badId + "/trickplay?seconds=0").valid);
        assert(!parser.parse(
            root + "ms_other/trickplay?seconds=0").valid);
        assert(!parser.parse(
            "/api/media/sessions/" + id + "/trickplay?seconds=1").valid);
        assert(!parser.parse(
            root + id + "/trickplay?seconds=1&secret=1").valid);
        assert(!parser.parse(
            root + id + "/trickplay?seconds=1").valid == false);
    }
    return 0;
}
