#pragma once

#include <unistd.h>
#include <utility>
#include <string>

namespace vdrsuite::addons {

// Owns a VERIFIED binary descriptor. No executable loader or dispatch.
// Future Agent code must use this same descriptor after all grants/fencing,
// not reopen a path selected by a request or a manifest.
class PinnedAddonHandler {
public:
    PinnedAddonHandler() = default;
    explicit PinnedAddonHandler(int fd) : fd_(fd) {}
    ~PinnedAddonHandler() { if (fd_ >= 0) ::close(fd_); }
    PinnedAddonHandler(const PinnedAddonHandler&) = delete;
    PinnedAddonHandler& operator=(const PinnedAddonHandler&) = delete;
    PinnedAddonHandler(PinnedAddonHandler&& other) noexcept
        : fd_(std::exchange(other.fd_, -1)) {}
    PinnedAddonHandler& operator=(PinnedAddonHandler&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }
    int get() const { return fd_; }
    explicit operator bool() const { return fd_ >= 0; }
private:
    int fd_ = -1;
};

enum class HandlerDescriptorReason {
    verified, invalidRoot, invalidApprovedDigest, unsafeDirectory,
    missingBinary, unsafeBinary, digestMismatch, changedDuringCheck,
    ioError
};

struct HandlerDescriptorResult {
    HandlerDescriptorReason reason = HandlerDescriptorReason::ioError;
    PinnedAddonHandler descriptor;
    bool verified() const {
        return reason == HandlerDescriptorReason::verified && bool(descriptor);
    }
};

// root="/" in production; another absolute secure root for isolated tests.
// The executable path is compiled into this implementation, never caller data.
HandlerDescriptorResult inspectPinnedMediaToolsHandler(
    const std::string& root, const std::string& independentlyApprovedSha256);

} // namespace vdrsuite::addons
