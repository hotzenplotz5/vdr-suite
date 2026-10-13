#include "HevcHlsKeyframeSampler.h"

#include <iostream>
#include <string>

// Isolated operator diagnostic; NEVER attests source identity or enables
// the experimental HLS mode. The operator supplies a prepared, private
// workspace containing input.ffconcat. No recording writes.
int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "usage: hevc_hls_probe_diagnostic <workspace>\n";
        return 2;
    }
    const MediaProcessRunner runner;
    const auto result = sampleHevcHlsKeyframes(runner, argv[1]);
    std::cout << "PROBE_COMPLETE=" << (result.complete ? "YES" : "NO") << "\n";
    if (!result.complete) {
        std::cout << "REASON=" << result.reasonCode << "\n";
        std::cout << "POLICY=UNTRUSTED_NO_SELECTION\n";
        return 1;
    }
    const auto& keys = result.packets.keyframePtsSeconds;
    std::cout << "KEYFRAMES=" << keys.size() << "\n";
    double minimum = 0.0;
    double maximum = 0.0;
    for (std::size_t i = 1; i < keys.size(); ++i) {
        const double interval = keys[i] - keys[i - 1];
        if (i == 1 || interval < minimum) minimum = interval;
        if (i == 1 || interval > maximum) maximum = interval;
    }
    if (keys.size() > 1) {
        std::cout << "GOP_MIN_SECONDS=" << minimum << "\n";
        std::cout << "GOP_MAX_SECONDS=" << maximum << "\n";
    }
    std::cout << "POLICY=UNTRUSTED_NO_SELECTION\n";
    return 0;
}
