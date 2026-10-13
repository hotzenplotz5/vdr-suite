#include "HevcHlsKeyframeProbe.h"

#include <algorithm>
#include <cassert>
#include <string>

int main()
{
    const auto plan = hevcHlsKeyframeProbePlan();
    assert(plan.argv.front() == "/usr/bin/ffprobe");
    assert(std::find(plan.argv.begin(), plan.argv.end(), "input.ffconcat") != plan.argv.end());
    assert(std::find(plan.argv.begin(), plan.argv.end(), "%+30") != plan.argv.end());
    assert(std::find(plan.argv.begin(), plan.argv.end(), "-show_packets") != plan.argv.end());
    assert(std::find(plan.argv.begin(), plan.argv.end(), "/bin/sh") == plan.argv.end());

    const auto evidence = parseHevcHlsKeyframeProbe(
        "1.400,K__\n1.441,___\n2.399,K__\n3.398,K__\n4.397,K__\n5.396,K__\n6.395,K__\n");
    assert(evidence.parsed && !evidence.truncated);
    assert(evidence.keyframePtsSeconds.size() == 6);

    const auto invalid = parseHevcHlsKeyframeProbe("1.400,K__\nUNKNOWN,___\n");
    assert(!invalid.parsed);
    const auto backwards = parseHevcHlsKeyframeProbe("1.400,K__\n1.390,___\n");
    assert(!backwards.parsed);
    const auto missing = parseHevcHlsKeyframeProbe("1.400\n");
    assert(!missing.parsed);
    const auto huge = parseHevcHlsKeyframeProbe(std::string(300000, 'x'));
    assert(!huge.parsed && huge.truncated);
    const auto empty = parseHevcHlsKeyframeProbe("");
    assert(!empty.parsed);
    return 0;
}
