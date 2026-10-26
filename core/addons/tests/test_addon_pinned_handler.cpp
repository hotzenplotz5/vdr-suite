#include "AddonPinnedHandler.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <cerrno>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr const char* ApprovedDigest =
    "a0711fa9587ab14350cbfb43a45f97bf4157e0378c85fc22cf5cdded3e6779cc";
constexpr const char* FixtureBytes = "nonexecuting media tools handler fixture\n";

struct Fixture {
    fs::path root;
    fs::path handler;
    Fixture() {
        if (::geteuid() != 0 || std::getenv("TMPDIR") == nullptr ||
            std::string(std::getenv("TMPDIR")) != "/root")
            throw std::runtime_error("root-owned isolated TMPDIR=/root required");
        std::string format = "/root/vdr-addon-fd-XXXXXX";
        std::vector<char> name(format.begin(), format.end());
        name.push_back('\0');
        char* path = ::mkdtemp(name.data());
        if (!path) throw std::runtime_error("mkdtemp failed");
        root = path;
        handler = root / "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import";
        fs::create_directories(handler.parent_path());
        std::ofstream output(handler, std::ios::binary);
        output << FixtureBytes;
        output.close();
        if (::chmod(handler.c_str(), 0700) != 0)
            throw std::runtime_error("chmod test fixture failed");
    }
    ~Fixture() { std::error_code ec; fs::remove_all(root, ec); }
};

void expect(vdrsuite::addons::HandlerDescriptorReason reason,
            const vdrsuite::addons::HandlerDescriptorResult& result) {
    assert(!result.verified());
    assert(result.reason == reason);
}
}

int main()
{
    using namespace vdrsuite::addons;
    Fixture fixture;
    const std::string root = fixture.root.string();
    auto valid = inspectPinnedMediaToolsHandler(root, ApprovedDigest);
    assert(valid.verified() && valid.descriptor.get() >= 0);
    assert(pinnedMediaToolsHandlerBytesCurrent(
        valid.descriptor.get(), ApprovedDigest));
    assert(!pinnedMediaToolsHandlerBytesCurrent(
        valid.descriptor.get(), std::string(64, '0')));
    assert(!pinnedMediaToolsHandlerBytesCurrent(-1, ApprovedDigest));

    // The digest must be read from the held descriptor, not a cached
    // authorization-time result or a fresh pathname lookup.
    {
        std::fstream tamper(fixture.handler,
                           std::ios::in | std::ios::out | std::ios::binary);
        assert(static_cast<bool>(tamper));
        tamper.put('X');
        tamper.flush();
        assert(static_cast<bool>(tamper));
        assert(!pinnedMediaToolsHandlerBytesCurrent(
            valid.descriptor.get(), ApprovedDigest));
        tamper.seekp(0);
        tamper.put(FixtureBytes[0]);
        tamper.flush();
        assert(static_cast<bool>(tamper));
    }
    assert(pinnedMediaToolsHandlerBytesCurrent(
        valid.descriptor.get(), ApprovedDigest));

    // Replacement after inspection must not redirect the pinned descriptor.
    const fs::path replaced = fixture.handler.string() + ".original";
    fs::rename(fixture.handler, replaced);
    {
        std::ofstream output(fixture.handler, std::ios::binary);
        output << "malicious replacement";
    }
    ::chmod(fixture.handler.c_str(), 0700);
    const std::size_t size = std::strlen(FixtureBytes);
    std::string heldBytes(size, '\0');
    assert(::read(valid.descriptor.get(), heldBytes.data(), size) ==
           static_cast<ssize_t>(size));
    assert(heldBytes == FixtureBytes);
    // A path replacement does not invalidate immutable bytes held by fd;
    // this must still read the ORIGINAL opened inode.
    assert(pinnedMediaToolsHandlerBytesCurrent(
        valid.descriptor.get(), ApprovedDigest));
    expect(HandlerDescriptorReason::digestMismatch,
           inspectPinnedMediaToolsHandler(root, ApprovedDigest));
    fs::remove(fixture.handler);
    fs::rename(replaced, fixture.handler);

    expect(HandlerDescriptorReason::invalidRoot,
           inspectPinnedMediaToolsHandler("../unsafe", ApprovedDigest));
    expect(HandlerDescriptorReason::invalidApprovedDigest,
           inspectPinnedMediaToolsHandler(root, "invalid"));
    expect(HandlerDescriptorReason::digestMismatch,
           inspectPinnedMediaToolsHandler(root, std::string(64, '0')));

    ::chmod(fixture.handler.c_str(), 0600);
    expect(HandlerDescriptorReason::unsafeBinary,
           inspectPinnedMediaToolsHandler(root, ApprovedDigest));
    ::chmod(fixture.handler.c_str(), 0700);

    ::chmod(fixture.handler.parent_path().c_str(), 0777);
    expect(HandlerDescriptorReason::unsafeDirectory,
           inspectPinnedMediaToolsHandler(root, ApprovedDigest));
    ::chmod(fixture.handler.parent_path().c_str(), 0700);

    fs::rename(fixture.handler, replaced);
    fs::create_symlink(replaced.filename(), fixture.handler);
    auto symlink = inspectPinnedMediaToolsHandler(root, ApprovedDigest);
    assert(!symlink.verified());
    fs::remove(fixture.handler);
    fs::rename(replaced, fixture.handler);

    auto current = inspectPinnedMediaToolsHandler(root, ApprovedDigest);
    assert(current.verified());
    const pid_t child = ::fork();
    assert(child >= 0);
    if (child == 0) {
        // A service manager is allowed to close all standard descriptors.
        // The reviewed pinned executable still needs its OWN fd >= 3.
        ::close(STDIN_FILENO);
        ::close(STDOUT_FILENO);
        ::close(STDERR_FILENO);
        auto lowFd = inspectPinnedMediaToolsHandler(root, ApprovedDigest);
        if (!lowFd.verified() || lowFd.descriptor.get() < 3)
            ::_exit(61);
        std::string observed(std::strlen(FixtureBytes), '\0');
        if (::read(lowFd.descriptor.get(), observed.data(), observed.size()) !=
                static_cast<ssize_t>(observed.size()) ||
            observed != FixtureBytes)
            ::_exit(62);
        ::_exit(0);
    }
    int status = 0;
    pid_t waited;
    do { waited = ::waitpid(child, &status, 0); }
    while (waited < 0 && errno == EINTR);
    assert(waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
