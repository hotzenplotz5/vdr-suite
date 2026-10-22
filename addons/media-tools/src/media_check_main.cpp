#include "MediaCheck.h"
#include "MediaCheckFfmpeg.h"

#include <iostream>
#include <stdexcept>
#include <string>

// Operator-only backend-local CLI, not an actor-facing HTTP entrypoint.
// The decoding path feeds verified TS input to a fixed FFmpeg executable
// using stdin and FFmpeg's null muxer. It never writes a recording file.
int main(int argc, char* argv[])
{
    if (argc != 4 ||
        (std::string(argv[1]) != "check" &&
         std::string(argv[1]) != "check-decode")) {
        std::cerr
            << "Usage: vdr-suite-media-check check VIDEO_ROOT RECORDING_RELATIVE\n"
            << "       vdr-suite-media-check check-decode VIDEO_ROOT RECORDING_RELATIVE\n";
        return 2;
    }
    try {
        vdrsuite::mediatools::MediaCheckReport report;
        if (std::string(argv[1]) == "check-decode") {
            vdrsuite::mediatools::FfmpegMediaCheckDecoder decoder;
            report = vdrsuite::mediatools::checkRecordingWithDecoder(
                argv[2], argv[3], decoder);
        } else {
            report = vdrsuite::mediatools::checkRecordingTransportStream(
                argv[2], argv[3]);
        }
        std::cout << report.json() << '\n';
        if (report.status() == "incomplete") return 2;
        return report.status() == "errors" ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "MEDIA_CHECK_UNCERTAIN=" << e.what() << '\n';
        return 2;
    }
}
