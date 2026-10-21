// Executable-only isolated CI fixture. It NEVER creates a recording and
// intentionally simulates just the pinned importer's commit output contract.
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 6 || std::string(argv[1]) != "import" ||
        std::string(argv[5]) != "--confirm-writes")
        return 2;
    std::cout << "IMPORT_STATUS=COMMITTED\n"
              << "RECORDING=" << argv[3]
              << "/Media/2026-10-21.18.00.00-0.rec\n"
              << "SOURCE_PRESERVED=YES\n";
    return 0;
}
