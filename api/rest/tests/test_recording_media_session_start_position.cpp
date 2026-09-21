#include "RecordingMediaSessionStartPosition.h"

#include <cassert>
#include <limits>
#include <string>

int main()
{
    const RecordingMediaSessionStartPositionParser parser;

    {
        const auto result = parser.parse(
            R"json({"backendId":"default","recordingId":"rec-1"})json");
        assert(result.valid);
        assert(!result.present);
        assert(result.seconds == 0);
        assert(result.reasonCode.empty());
    }

    {
        const auto result = parser.parse(
            R"json({"backendId":"default","recordingId":"rec-1","startPositionSeconds":2494})json");
        assert(result.valid);
        assert(result.present);
        assert(result.seconds == 2494);
        assert(result.reasonCode.empty());
    }

    {
        const auto result = parser.parse(
            R"json({"startPositionSeconds":0})json");
        assert(result.valid);
        assert(result.present);
        assert(result.seconds == 0);
    }

    for (const std::string body : {
             R"json({"startPositionSeconds":-1})json",
             R"json({"startPositionSeconds":12.5})json",
             R"json({"startPositionSeconds":"12"})json",
             R"json({"startPositionSeconds":12junk})json"}) {
        const auto result = parser.parse(body);
        assert(!result.valid);
        assert(result.present);
        assert(result.reasonCode == "invalid_recording_start_position");
    }

    const std::string overflow =
        std::string("{\"startPositionSeconds\":") +
        std::to_string(static_cast<long long>(std::numeric_limits<int>::max()) + 1LL) +
        "}";
    const auto tooLarge = parser.parse(overflow);
    assert(!tooLarge.valid);
    assert(tooLarge.reasonCode == "invalid_recording_start_position");

    return 0;
}
