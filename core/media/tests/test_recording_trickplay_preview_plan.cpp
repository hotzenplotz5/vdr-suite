#include "RecordingTrickplayPreviewPlan.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace {
bool pair(const std::vector<std::string>& argv,
          const std::string& flag, const std::string& value)
{
    for (std::size_t i=0; i+1<argv.size(); ++i)
        if (argv[i] == flag && argv[i+1] == value) return true;
    return false;
}
bool absent(const std::vector<std::string>& argv, const std::string& value)
{
    return std::find(argv.begin(), argv.end(), value) == argv.end();
}
}

int main()
{
    RecordingTrickplayPreviewPlanner planner;
    {
        const auto p = planner.build(125, 600, 2, {300.0, 300.0});
        assert(p.valid);
        assert(p.requestedSeconds == 125);
        assert(p.sampleSeconds == 120);
        assert(p.intervalSeconds == 10);
        assert(pair(p.argv, "-ss", "120"));
        assert(pair(p.argv, "-i", "input.ffconcat"));
        assert(pair(p.argv, "-frames:v", "1"));
        assert(pair(p.argv, "-f", "image2pipe"));
        assert(pair(p.argv, "-map", "0:v:0"));
        assert(pair(p.argv, "-protocol_whitelist", "file,pipe"));
        assert(pair(p.argv, "-vf", "scale=320:-2:flags=fast_bilinear,setsar=1"));
        assert(absent(p.argv, "-re"));
        assert(absent(p.argv, "-hls_time"));
        assert(absent(p.argv, "/etc/passwd"));
    }
    {
        const auto zero = planner.build(0, 30, 1, {30.0});
        const auto last = planner.build(29, 30, 1, {30.0});
        assert(zero.valid && zero.sampleSeconds == 0);
        assert(last.valid && last.sampleSeconds == 20);
    }
    for (int bad : {-1, 600, 601, std::numeric_limits<int>::max()}) {
        const auto p = planner.build(bad, 600, 2, {300., 300.});
        assert(!p.valid && p.argv.empty());
        assert(p.reasonCode == "trickplay_position_out_of_bounds");
    }
    {
        const auto missing = planner.build(10, 600, 2, {});
        const auto partial = planner.build(10, 600, 2, {600.0});
        const auto zero = planner.build(10, 600, 0, {});
        const auto wrongDuration = planner.build(10, 600, 2, {100.0, 100.0});
        const auto bogus = planner.build(10, 0, 1, {1.0});
        const auto huge = planner.build(10, 99999999, 1, {99999999.0});
        assert(!missing.valid && !partial.valid && !zero.valid);
        assert(!wrongDuration.valid && wrongDuration.reasonCode ==
               "trickplay_index_inconsistent");
        assert(!bogus.valid && !huge.valid);
    }
    for (double d : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                     std::numeric_limits<double>::quiet_NaN()}) {
        const auto result = planner.build(30, 600, 2, {300.0, d});
        assert(!result.valid && result.argv.empty());
    }
    {
        const auto p = planner.build(43000, 43200, 1, {43200.0});
        assert(p.valid && p.sampleSeconds == 43000);
        const auto oversized = planner.build(0, 3600, 1025,
                                               std::vector<double>(1025, 3.512));
        assert(!oversized.valid);
    }
    return 0;
}
