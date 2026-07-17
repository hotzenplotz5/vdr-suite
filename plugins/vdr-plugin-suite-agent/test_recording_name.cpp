#include "recording_name.h"

#include <cassert>

using vdrsuite::agent::buildMovedRecordingName;

int main()
{
    {
        const auto result = buildMovedRecordingName(
            "Archive~My Record",
            "Movies/Drama");
        assert(result.success);
        assert(result.newName == "Movies~Drama~My Record");
    }

    {
        const auto result = buildMovedRecordingName(
            "Archive~My Record",
            "Movies~Drama");
        assert(result.success);
        assert(result.newName == "Movies~Drama~My Record");
    }

    {
        const auto result = buildMovedRecordingName(
            "Archive~My Record",
            "/");
        assert(result.success);
        assert(result.newName == "My Record");
    }

    {
        const auto result = buildMovedRecordingName(
            "My Record",
            "");
        assert(result.success);
        assert(result.newName == "My Record");
    }

    {
        const auto result = buildMovedRecordingName(
            "Archive~My Record",
            "Movies/../Drama");
        assert(!result.success);
    }

    {
        const auto result = buildMovedRecordingName(
            "Archive~My Record",
            "Movies\nDrama");
        assert(!result.success);
    }

    return 0;
}
