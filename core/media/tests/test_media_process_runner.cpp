#include "MediaProcessRunner.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace
{

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path.string(), std::ios::binary);
    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

} // namespace

int main()
{
    MediaProcessRunner runner;
    const std::string workingDirectory =
        std::filesystem::temp_directory_path().string();

    {
        const auto result = runner.runAndCapture(
            {"/usr/bin/printf", "codec_name=h264|codec_type=video\\n"},
            workingDirectory,
            std::chrono::milliseconds(1000),
            4096);
        assert(result.started);
        assert(result.completed);
        assert(result.success);
        assert(result.exitCode == 0);
        assert(result.output == "codec_name=h264|codec_type=video\n");
    }

    {
        const auto result = runner.runAndCapture(
            {"printf", "unsafe"},
            workingDirectory,
            std::chrono::milliseconds(1000),
            4096);
        assert(!result.started);
        assert(!result.success);
        assert(result.reasonCode == "invalid_process_invocation");
    }

    {
        const auto result = runner.runAndCapture(
            {"/usr/bin/printf", "0123456789"},
            workingDirectory,
            std::chrono::milliseconds(1000),
            4);
        assert(result.started);
        assert(!result.success);
        assert(result.outputLimitExceeded);
        assert(result.reasonCode == "process_output_limit_exceeded");
    }

    const std::filesystem::path testRoot =
        std::filesystem::temp_directory_path() /
        "vdr-suite-phase65-process-runner-test";
    std::error_code ignored;
    std::filesystem::remove_all(testRoot, ignored);
    std::filesystem::create_directories(testRoot);

    {
        const std::filesystem::path logPath = testRoot / "worker.log";
        const pid_t pid = runner.spawnLogged(
            {"/usr/bin/printf", "worker-finished\\n"},
            testRoot.string(),
            logPath.string());
        assert(pid > 0);
        assert(runner.terminateAndWait(pid, std::chrono::milliseconds(1000)));
        assert(readFile(logPath).find("worker-finished") != std::string::npos);
    }

    {
        const std::filesystem::path logPath = testRoot / "sleep.log";
        const pid_t pid = runner.spawnLogged(
            {"/usr/bin/sleep", "5"},
            testRoot.string(),
            logPath.string());
        assert(pid > 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        assert(runner.terminateAndWait(pid, std::chrono::milliseconds(200)));
    }

    std::filesystem::remove_all(testRoot, ignored);
    return 0;
}
