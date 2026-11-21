#include "RecordingTrickplayPreviewRenderer.h"

#include "MediaSessionWorkspace.h"

#include <chrono>
#include <cerrno>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace {
constexpr std::size_t MaximumJpegBytes = 256 * 1024;
constexpr std::size_t MaximumConcatBytes = 256 * 1024;
constexpr std::size_t MaximumMemoryBytes = 768 * 1024 * 1024;
std::mutex previewWorkerMutex;

bool validJpeg(const std::string& image)
{
    // Minimal MIME framing verification; this is not a pixel-decoder test.
    // FFmpeg itself produces the JPEG, subject to capture output limits.
    if (image.size() < 16 || image.size() > MaximumJpegBytes)
        return false;
    const auto byte = [&image](std::size_t at) {
        return static_cast<unsigned char>(image[at]);
    };
    return byte(0) == 0xff && byte(1) == 0xd8 &&
           byte(image.size() - 2) == 0xff &&
           byte(image.size() - 1) == 0xd9;
}

bool ownsPrivateConcat(const MediaSessionWorkspace& workspace)
{
    const std::string& cwd = workspace.directory();
    const std::string concat = workspace.concatPath();
    if (cwd.empty() || cwd.front() != '/' ||
        concat != cwd + "/input.ffconcat") return false;

    const int dir = ::open(cwd.c_str(),
                           O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dir < 0) return false;
    struct stat directory{};
    const bool privateDirectory = ::fstat(dir, &directory) == 0 &&
        S_ISDIR(directory.st_mode) && directory.st_uid == ::geteuid() &&
        (directory.st_mode & 0077) == 0;

    int fd = -1;
    if (privateDirectory) {
        fd = ::openat(dir, "input.ffconcat",
                      O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    }
    struct stat file{};
    const bool trusted = fd >= 0 && ::fstat(fd, &file) == 0 &&
        S_ISREG(file.st_mode) && file.st_uid == ::geteuid() &&
        file.st_nlink == 1 && file.st_size > 0 &&
        file.st_size <= static_cast<off_t>(MaximumConcatBytes);
    if (fd >= 0) ::close(fd);
    ::close(dir);
    return trusted;
}

RecordingTrickplayPreviewResult error(const std::string& code)
{
    RecordingTrickplayPreviewResult result;
    result.reasonCode = code;
    return result;
}
}

RecordingTrickplayPreviewRenderer::RecordingTrickplayPreviewRenderer(Capture capture)
    : capture_(std::move(capture))
{
    if (!capture_) {
        capture_ = [](const std::vector<std::string>& argv,
                      const std::string& directory,
                      std::chrono::milliseconds timeout,
                      std::size_t limit,
                      MediaProcessLimits resources) {
            return MediaProcessRunner().runAndCapture(
                argv, directory, timeout, limit, resources);
        };
    }
}

RecordingTrickplayPreviewResult RecordingTrickplayPreviewRenderer::render(
    const RecordingTrickplayPreviewPlan& plan,
    const MediaSessionWorkspace& workspace) const
{
    // Refuse arbitrary argv built outside the planner, even from an in-process
    // caller. The only variable accepted by this exact command template is
    // the indexed, quantized timestamp. Never expose this API to user paths.
    if (!plan.valid || plan.argv.empty() ||
        plan.sampleSeconds < 0 ||
        plan.sampleSeconds > 12 * 60 * 60 ||
        RecordingTrickplayPreviewPlanner().build(
            plan.sampleSeconds, plan.sampleSeconds + 1, 1,
            {static_cast<double>(plan.sampleSeconds + 1)}).argv != plan.argv) {
        // Rebuilding with a synthetic one-segment index is only an argv
        // equality guard; it confers NO source/index authorization.
        return error("trickplay_plan_invalid");
    }
    if (!ownsPrivateConcat(workspace))
        return error("trickplay_workspace_invalid");

    // Avoid concurrent FFmpeg storms across parallel API requests.
    std::unique_lock<std::mutex> guard(previewWorkerMutex, std::try_to_lock);
    if (!guard.owns_lock())
        return error("trickplay_worker_busy");

    const auto result = capture_(plan.argv, workspace.directory(),
                                 std::chrono::seconds(3),
                                 MaximumJpegBytes,
                                 {MaximumMemoryBytes, 2});
    if (!result.success || !result.completed || result.timedOut ||
        result.outputLimitExceeded || !validJpeg(result.output))
        return error("trickplay_frame_not_ready");

    RecordingTrickplayPreviewResult ready;
    ready.ready = true;
    ready.sampleSeconds = plan.sampleSeconds;
    ready.jpeg = result.output;
    return ready;
}
