#include "MediaCheck.h"

#include <iostream>
#include <stdexcept>
#include <string>

// Deliberately separate from the write-capable import binary. The caller
// supplies its trusted backend video root; RECORDING is relative to it.
// No worker registration, grants, web API or filesystem write is performed.
int main(int argc, char* argv[])
{
    if (argc != 4 || std::string(argv[1]) != "check") {
        std::cerr << "Usage: vdr-suite-media-check check VIDEO_ROOT RECORDING_RELATIVE\n";
        return 2;
    }
    try {
        const auto report =
            vdrsuite::mediatools::checkRecordingTransportStream(argv[2], argv[3]);
        std::cout << report.json() << '\n';
        return report.status() == "errors" ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "MEDIA_CHECK_UNCERTAIN=" << e.what() << '\n';
        return 2;
    }
}
