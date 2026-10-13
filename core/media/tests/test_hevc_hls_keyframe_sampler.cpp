#include "HevcHlsKeyframeSampler.h"

#include <cassert>
#include <string>

int main()
{
    const MediaProcessRunner runner;
    const auto bad = sampleHevcHlsKeyframes(runner, "relative/path");
    assert(!bad.complete);
    assert(bad.reasonCode == "invalid_probe_workspace");

    // No input.ffconcat in this isolated directory: the bounded ffprobe
    // process must fail closed, without creating trusted policy evidence.
    const auto missing = sampleHevcHlsKeyframes(runner, "/tmp");
    assert(!missing.complete);
    assert(!missing.reasonCode.empty());
    assert(!missing.packets.parsed);
    return 0;
}
