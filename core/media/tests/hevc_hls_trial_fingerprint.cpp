#include "RecordingSourceFingerprint.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// Read-only operator diagnostic. Print exact backend fingerprint for one
// recording, not a client-supplied recording identifier. No VDR modifications.
int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "usage: hevc_hls_trial_fingerprint <absolute-recording-directory>\n";
        return 2;
    }
    const std::filesystem::path dir(argv[1]);
    std::error_code ec;
    if (!dir.is_absolute() || !std::filesystem::is_directory(dir, ec) || ec) {
        std::cerr << "STOP_INVALID_RECORDING_DIRECTORY\n";
        return 2;
    }
    std::vector<std::string> files;
    for (std::filesystem::directory_iterator it(dir, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec) || ec) break;
        const auto name = it->path().filename().string();
        const auto extension = it->path().extension().string();
        if (name.size() == 5 + extension.size() &&
            (extension == ".ts" || extension == ".vdr") &&
            std::all_of(name.begin(), name.begin() + 5,
                        [](unsigned char c) { return c >= '0' && c <= '9'; })) {
            files.push_back(it->path().string());
        }
    }
    if (ec || files.empty()) {
        std::cerr << "STOP_SEGMENT_DISCOVERY\n";
        return 2;
    }
    std::sort(files.begin(), files.end());
    const auto fingerprint = inspectRecordingSource(dir.string(), files);
    if (!fingerprint.valid || fingerprint.growing) {
        std::cerr << "STOP_FINGERPRINT=" << fingerprint.reasonCode << "\n";
        return 1;
    }
    std::cout << "RECORDING_SEGMENTS=" << files.size() << "\n";
    std::cout << "SOURCE_FINGERPRINT=" << fingerprint.value << "\n";
    std::cout << "SOURCE_STABLE=YES\n";
    return 0;
}
