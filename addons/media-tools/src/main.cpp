#include "MediaImport.h"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace vdrsuite::mediatools;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: vdr-suite-media-import plan SOURCE_ROOT VIDEO_ROOT FILE\n"
                  << "       vdr-suite-media-import import SOURCE_ROOT VIDEO_ROOT FILE --confirm-writes\n";
        return 2;
    }
    try {
        const std::string action(argv[1]);
        if (argc != 5 && argc != 6) throw std::invalid_argument("invalid argument count");
        if (action != "plan" && action != "import") throw std::invalid_argument("unknown action");
        if ((action == "plan" && argc != 5) ||
            (action == "import" && (argc != 6 || std::string(argv[5]) != "--confirm-writes"))) {
            throw std::invalid_argument("import requires explicit --confirm-writes");
        }
        const auto plan = inspectImport({argv[2], argv[3], argv[4]});
        std::cout << "SOURCE=" << plan.sourceFile << "\n"
                  << "TITLE=" << plan.title << "\n"
                  << "SOURCE_BYTES=" << plan.sourceBytes << "\n"
                  << "TARGET_PARENT=" << plan.titleDirectory << "\n";
        if (action == "plan") {
            std::cout << "IMPORT_STATUS=PLAN_ONLY\n";
            return 0;
        }
        NativeProcessRunner runner;
        const auto result = executeRemuxImport(plan, runner, requireProductionCapacity);
        std::cout << "IMPORT_STATUS=COMMITTED\nRECORDING=" << result << "\n"
                  << "SOURCE_PRESERVED=YES\n"
                  << "SUITE_RECONCILIATION=NOT_CONNECTED\n";
        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "MEDIA_IMPORT_ERROR=" << exc.what() << "\n";
        return 1;
    }
}
