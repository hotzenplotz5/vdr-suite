#include "MediaShrink.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace vdrsuite::mediatools;

namespace {
ShrinkOptions optionsFor(int argc, char* argv[], int from) {
    ShrinkOptions options;
    for (int i = from; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--night-mode") options.nightMode = true;
        else if (arg.rfind("--max-height=", 0) == 0) {
            const std::string value = arg.substr(13);
            std::size_t used = 0;
            try { options.maxHeight = std::stoul(value, &used); }
            catch (...) { throw std::invalid_argument("invalid height"); }
            if (used != value.size()) throw std::invalid_argument("invalid height");
        } else if (arg == "--encoder=software") options.encoder = ShrinkEncoder::software;
        else if (arg == "--encoder=vaapi") options.encoder = ShrinkEncoder::vaapi;
        else if (arg == "--encoder=qsv") options.encoder = ShrinkEncoder::qsv;
        else if (arg == "--encoder=nvenc") options.encoder = ShrinkEncoder::nvenc;
        else throw std::invalid_argument("unrecognized shrink option");
    }
    return options;
}
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: vdr-suite-media-shrink plan VIDEO_ROOT RELATIVE.rec [options]\n"
                  << "       vdr-suite-media-shrink stage VIDEO_ROOT RELATIVE.rec --confirm-staging [options]\n"
                  << "       vdr-suite-media-shrink publish VIDEO_ROOT RELATIVE.rec CANDIDATE --confirm-new-recording\n";
        return 2;
    }
    try {
        const std::string action(argv[1]);
        const std::string root(argv[2]), relative(argv[3]);
        if (action == "publish") {
            if (argc != 6 || std::string(argv[5]) != "--confirm-new-recording")
                throw std::invalid_argument("publication requires explicit confirmation");
            const auto plan = inspectShrink(root, relative);
            NativeProcessRunner runner(std::chrono::seconds(120));
            const auto result = publishShrinkCandidate(plan, argv[4], runner);
            std::cout << "HEVC_PUBLISHED_NEW_RECORDING=" << result << "\n"
                      << "ORIGINAL_RECORDING_PRESERVED=YES\n"
                      << "SUITE_RECONCILIATION=REQUIRED\n";
            return 0;
        }
        if (action != "plan" && action != "stage")
            throw std::invalid_argument("unknown shrink action");
        int firstOption = 4;
        if (action == "stage") {
            if (argc < 5 || std::string(argv[4]) != "--confirm-staging")
                throw std::invalid_argument("staging requires explicit confirmation");
            firstOption = 5;
        }
        const auto settings = optionsFor(argc, argv, firstOption);
        const auto plan = inspectShrink(root, relative, settings);
        std::cout << "SHRINK_SOURCE_BYTES=" << plan.totalBytes << "\n"
                  << "SHRINK_SEGMENTS=" << plan.segments.size() << "\n";
        if (action == "plan") {
            std::cout << "SHRINK_PLAN_ONLY=YES\n";
            return 0;
        }
        // This administrator-invoked local conversion is not a remote Agent
        // command. The protected Agent importer retains its shorter default.
        NativeProcessRunner runner(std::chrono::seconds(4 * 3600));
        const auto candidate = stageShrink(plan, runner, requireShrinkCapacity);
        std::cout << "SHRINK_REVIEW_CANDIDATE=" << candidate << "\n"
                  << "ORIGINAL_RECORDING_PRESERVED=YES\n"
                  << "SHRINK_PUBLICATION=REQUIRES_SEPARATE_CONFIRMATION\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "MEDIA_SHRINK_ERROR=" << e.what() << "\n";
        return 1;
    }
}
