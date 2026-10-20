#include "AddonPinnedHandler.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
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

    // Replacement after inspection must not redirect the pinned descriptor.
    const fs::path replaced = fixture.handler.string() + ".original";
    fs::rename(fixture.handler, replaced);
    {
        std::ofstream output(fixture.handler, std::ios::binary);
        output << "malicious replacement";
    }
    ::chmod(fixture.handler.c_str(), 0700);
    char buffer[sizeof(FixtureBytes)]{};
    assert(::read(valid.descriptor.get(), buffer, sizeof(FixtureBytes) - 1) ==
           static_cast<ssize_t>(sizeof(FixtureBytes) - 1));
    assert(std::string(buffer) == FixtureBytes);
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
}
