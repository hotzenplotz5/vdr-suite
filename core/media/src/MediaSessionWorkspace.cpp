#include "MediaSessionWorkspace.h"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace
{

bool safeWorkspaceId(const std::string& value)
{
    if (value.empty() || value.size() > 96) {
        return false;
    }

    for (unsigned char character : value) {
        const bool alphaNumeric =
            (character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9');
        if (!alphaNumeric && character != '-' && character != '_') {
            return false;
        }
    }
    return true;
}

std::string safeSourceName(std::size_t index, const std::string& sourcePath)
{
    const std::filesystem::path path(sourcePath);
    const std::string extension = path.extension() == ".vdr" ? ".vdr" : ".ts";

    std::ostringstream name;
    name << "source-" << std::setw(6) << std::setfill('0') << (index + 1)
         << extension;
    return name.str();
}

bool writeAll(int fd, const std::string& value)
{
    std::size_t offset = 0;
    while (offset < value.size()) {
        const ssize_t written = ::write(
            fd,
            value.data() + offset,
            value.size() - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (written == 0) {
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

} // namespace

MediaSessionWorkspace::MediaSessionWorkspace(std::string rootDirectory)
    : rootDirectory_(std::move(rootDirectory))
{
}

MediaSessionWorkspace::~MediaSessionWorkspace()
{
    cleanup();
}

MediaSessionWorkspaceResult MediaSessionWorkspace::prepare(
    const std::string& workspaceId,
    const std::vector<std::string>& sourceSegments)
{
    MediaSessionWorkspaceResult result;
    cleanup();

    const std::filesystem::path root(rootDirectory_);
    if (!root.is_absolute() || !safeWorkspaceId(workspaceId) || sourceSegments.empty()) {
        result.reasonCode = "invalid_workspace_request";
        return result;
    }

    std::error_code error;
    std::filesystem::create_directories(root, error);
    if (error) {
        result.reasonCode = "workspace_root_unavailable";
        return result;
    }
    ::chmod(root.c_str(), 0700);

    const std::filesystem::path workspace = root / workspaceId;
    if (!std::filesystem::create_directory(workspace, error) || error) {
        result.reasonCode = "workspace_create_failed";
        return result;
    }
    ::chmod(workspace.c_str(), 0700);
    directory_ = workspace.string();

    std::string concat = "ffconcat version 1.0\n";

    for (std::size_t index = 0; index < sourceSegments.size(); ++index) {
        const std::filesystem::path source(sourceSegments[index]);
        if (!source.is_absolute()) {
            result.reasonCode = "source_segment_not_absolute";
            cleanup();
            return result;
        }

        std::error_code typeError;
        if (!std::filesystem::is_regular_file(source, typeError) || typeError) {
            result.reasonCode = "source_segment_unavailable";
            cleanup();
            return result;
        }

        const std::string localName = safeSourceName(index, sourceSegments[index]);
        std::filesystem::create_symlink(source, workspace / localName, error);
        if (error) {
            result.reasonCode = "source_segment_link_failed";
            cleanup();
            return result;
        }

        concat += "file '" + localName + "'\n";
    }

    const std::filesystem::path concatFile = workspace / "input.ffconcat";
    const int fd = ::open(
        concatFile.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC,
        0600);
    if (fd < 0) {
        result.reasonCode = "concat_create_failed";
        cleanup();
        return result;
    }

    const bool written = writeAll(fd, concat);
    const int closeResult = ::close(fd);
    if (!written || closeResult != 0) {
        result.reasonCode = "concat_write_failed";
        cleanup();
        return result;
    }

    result.ready = true;
    return result;
}

void MediaSessionWorkspace::cleanup()
{
    if (directory_.empty()) {
        return;
    }

    const std::filesystem::path root(rootDirectory_);
    const std::filesystem::path workspace(directory_);
    std::error_code error;

    if (root.is_absolute() &&
        workspace.parent_path() == root &&
        workspace != root) {
        std::filesystem::remove_all(workspace, error);
    }

    directory_.clear();
}

const std::string& MediaSessionWorkspace::directory() const
{
    return directory_;
}

std::string MediaSessionWorkspace::concatPath() const
{
    return directory_.empty()
        ? std::string{}
        : (std::filesystem::path(directory_) / "input.ffconcat").string();
}

std::string MediaSessionWorkspace::logPath() const
{
    return directory_.empty()
        ? std::string{}
        : (std::filesystem::path(directory_) / "ffmpeg.log").string();
}
