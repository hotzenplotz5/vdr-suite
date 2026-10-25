#include "MediaImport.h"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace vdrsuite::mediatools;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: vdr-suite-media-import plan SOURCE_ROOT VIDEO_ROOT FILE\n"
                  << "       vdr-suite-media-import import SOURCE_ROOT VIDEO_ROOT FILE --confirm-writes [--approve-video-reencode]\n"
                  << "       Use plan first to inspect the codec. The optional encoding flag is operator-only.\n";
        return 2;
    }
    try {
        const std::string action(argv[1]);
        if (action != "plan" && action != "import")
            throw std::invalid_argument("unknown action");
        const bool localReencode = action == "import" && argc == 7 &&
            std::string(argv[5]) == "--confirm-writes" &&
            std::string(argv[6]) == "--approve-video-reencode";
        if ((action == "plan" && argc != 5) ||
            (action == "import" && !(
                (argc == 6 && std::string(argv[5]) == "--confirm-writes") ||
                localReencode))) {
            throw std::invalid_argument("invalid import arguments or missing write confirmation");
        }
        const auto plan = action == "plan"
            ? inspectImportPreview(ImportRequest{argv[2], argv[3], argv[4]})
            : inspectImport(ImportRequest{argv[2], argv[3], argv[4]});
        std::cout << "SOURCE=" << plan.sourceFile << "\n"
                  << "TITLE=" << plan.title << "\n"
                  << "SOURCE_BYTES=" << plan.sourceBytes << "\n"
                  << "TARGET_PARENT=" << plan.titleDirectory << "\n";
        if (action == "plan") {
            NativeProcessRunner probe(std::chrono::seconds(30));
            const auto codec = probeImportVideoCodec(plan, probe);
            const auto capacity = inspectImportCapacity(plan);
            std::cout << "VIDEO_CODEC=" << codec << "\n"
                      << "VIDEO_REENCODE_REQUIRED="
                      << (importVideoNeedsReencode(codec) ? "YES" : "NO") << "\n"
                      << "AVAILABLE_BYTES=" << capacity.availableBytes << "\n"
                      << "REQUIRED_BYTES=" << capacity.requiredBytes << "\n"
                      << "RESERVE_BYTES=" << capacity.reserveBytes << "\n"
                      << "TARGET_WRITABLE=" << (capacity.writable ? "YES" : "NO") << "\n"
                      << "SPACE_SUFFICIENT=" << (capacity.sufficient ? "YES" : "NO") << "\n"
                      << "DUPLICATE="
                      << (plan.duplicateTarget ? "YES" : "NO") << "\n"
                      << "IMPORT_STATUS=PLAN_ONLY\n";
            return 0;
        }
        if (!localReencode) {
            // Fail before copying gigabytes into private staging. The
            // executor repeats this check on the immutable staged snapshot
            // in case the source or codec changes after the plan.
            NativeProcessRunner preview(std::chrono::seconds(30));
            const auto codec = probeImportVideoCodec(plan, preview);
            if (importVideoNeedsReencode(codec)) {
                std::cout << "VIDEO_CODEC=" << codec << "\n"
                          << "VIDEO_REENCODE_REQUIRED=YES\n"
                          << "IMPORT_STATUS=DECISION_REQUIRED\n"
                          << "SOURCE_PRESERVED=YES\n";
                return 3;
            }
        }
        NativeProcessRunner runner;
        const auto result = executeRemuxImport(
            plan, runner, requireProductionCapacity, localReencode);
        std::cout << "IMPORT_STATUS=COMMITTED\nRECORDING=" << result << "\n"
                  << "SOURCE_PRESERVED=YES\n"
                  << "SUITE_RECONCILIATION=NOT_CONNECTED\n";
        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "MEDIA_IMPORT_ERROR=" << exc.what() << "\n";
        return 1;
    }
}
