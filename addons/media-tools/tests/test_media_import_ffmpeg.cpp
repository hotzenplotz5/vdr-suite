#include "MediaImport.h"
#include "MediaCheck.h"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace vdrsuite::mediatools;

namespace {
struct Fixture {
    fs::path root, input, video, source;
    Fixture() {
        const char* tmp = std::getenv("TMPDIR");
        const fs::path parent = tmp && *tmp ? tmp : "/tmp";
        std::string pattern = (parent / "suite-real-ffmpeg-XXXXXX").string();
        std::vector<char> buffer(pattern.begin(), pattern.end());
        buffer.push_back('\0');
        char* created = ::mkdtemp(buffer.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        root = created;
        input = root / "source";
        video = root / "video";
        fs::create_directory(input);
        fs::create_directory(video);
        source = input / "RealSample.mp4";
    }
    ~Fixture() {
        std::error_code error;
        fs::remove_all(root, error);
    }
};

struct NativeFfmpegWithSyntheticVdrIndex final : ProcessRunner {
    NativeProcessRunner tools{std::chrono::seconds(90)};
    int indexRequests = 0;
    ProcessResult run(const std::vector<std::string>& command, bool capture) override {
        assert(!command.empty());
        if (command[0] != "/usr/bin/vdr") {
            const auto result = tools.run(command, capture);
            if (result.exitCode != 0 && command[0] == "/usr/bin/ffmpeg") {
                // Test fixture only: re-run failed FFmpeg against our own
                // disposable file, printing bounded diagnostic stderr.
                const pid_t pid = ::fork();
                if (pid == 0) {
                    ::alarm(15);
                    ::dup2(STDOUT_FILENO, STDERR_FILENO);
                    std::vector<char*> argv;
                    for (const auto& arg : command)
                        argv.push_back(const_cast<char*>(arg.c_str()));
                    argv.push_back(nullptr);
                    ::execv(argv[0], argv.data());
                    _exit(127);
                }
                if (pid > 0) {
                    int status = 0;
                    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
                }
            }
            return result;
        }
        ++indexRequests;
        if (capture || command.size() != 2 ||
            command[1].rfind("--genindex=", 0) != 0)
            throw std::runtime_error("invalid simulated VDR invocation");
        const fs::path recording(command[1].substr(11));
        if (!recording.is_absolute() || recording.extension() != ".rec" ||
            !fs::is_regular_file(recording / "00001.ts") ||
            !fs::is_regular_file(recording / "info"))
            throw std::runtime_error("VDR recording structure invalid");
        const char* realVdr = std::getenv("MEDIA_TOOLS_TEST_REAL_VDR");
        if (realVdr && std::string(realVdr) == "1") {
            const auto result = tools.run(command, capture);
            if (result.exitCode != 0) {
                std::cerr << "REAL_VDR_INDEX=FAILED exit="
                          << result.exitCode << " retry_diagnostic=START\n";
                // CI fixture only: re-run with native stderr attached to
                // Actions console. No shell or access to installed recordings.
                const pid_t pid = ::fork();
                if (pid == 0) {
                    ::alarm(15);
                    ::dup2(STDOUT_FILENO, STDERR_FILENO);
                    std::vector<char*> argv;
                    for (const auto& arg : command)
                        argv.push_back(const_cast<char*>(arg.c_str()));
                    argv.push_back(nullptr);
                    ::execv(argv[0], argv.data());
                    _exit(127);
                }
                if (pid > 0) {
                    int status = 0;
                    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
                }
            }
            return result;
        }
        // Synthetic fallback must never count as actual VDR index proof.
        std::ofstream index(recording / "index", std::ios::binary);
        index << "TEST_ONLY_SYNTHETIC_INDEX";
        if (!index) throw std::runtime_error("index fixture creation failed");
        return {0, ""};
    }
};
}

int main() {
    Fixture fixture;
    NativeFfmpegWithSyntheticVdrIndex runner;
    const auto generated = runner.tools.run({
        "/usr/bin/ffmpeg", "-nostdin", "-hide_banner", "-v", "error",
        "-f", "lavfi", "-i", "testsrc2=size=160x90:rate=25",
        "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=48000",
        "-t", "6", "-c:v", "libx264", "-preset", "ultrafast",
        "-g", "25", "-keyint_min", "25",
        "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "96k",
        "-y", fixture.source.string()
    }, false);
    if (generated.exitCode != 0 || !fs::exists(fixture.source))
        throw std::runtime_error("real FFmpeg source generation failed");
    const auto originalBytes = fs::file_size(fixture.source);
    if (originalBytes == 0)
        throw std::runtime_error("real source unexpectedly empty");

    const auto plan = inspectImport({fixture.input, fixture.video, fixture.source});
    const auto recording = executeRemuxImport(
        plan, runner, [](const ImportPlan&) {});
    assert(runner.indexRequests == 1 && recording.extension() == ".rec");
    assert(fs::is_regular_file(recording / "index") &&
           fs::file_size(recording / "index") > 0);
    assert(fs::file_size(fixture.source) == originalBytes);
    assert(fs::is_regular_file(recording / "00001.ts"));
    const auto report = checkRecordingTransportStream(
        fixture.video, recording.lexically_relative(fixture.video));
    assert(report.segments == 1 && report.packets > 0);
    assert(report.syncErrors == 0 && report.transportErrors == 0);
    assert(report.trailingBytes == 0);
    assert(report.status() != "errors");
    std::cout << "MEDIA_IMPORT_REAL_FFMPEG_SMOKE=PASS"
              << " ts_packets=" << report.packets
              << " source_preserved=YES vdr_index="
              << (std::getenv("MEDIA_TOOLS_TEST_REAL_VDR") &&
                  std::string(std::getenv("MEDIA_TOOLS_TEST_REAL_VDR")) == "1"
                  ? "REAL" : "SIMULATED")
              << "\n";
}
