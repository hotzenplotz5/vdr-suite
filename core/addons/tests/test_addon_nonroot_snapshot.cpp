#include "AddonInstallSnapshot.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr const char* Digest =
    "a0711fa9587ab14350cbfb43a45f97bf4157e0378c85fc22cf5cdded3e6779cc";

struct Fixture {
    fs::path root, approval, handler, manifest;
    Fixture() {
        if (::geteuid() != 0 || std::getenv("TMPDIR") == nullptr ||
            std::string(std::getenv("TMPDIR")) != "/root")
            throw std::runtime_error("isolated CI root fixture required");
        struct stat run{};
        if (::stat("/run", &run) != 0 || !S_ISDIR(run.st_mode) ||
            run.st_uid != 0 || (run.st_mode & 0022) != 0)
            throw std::runtime_error("unsafe CI fixture parent");
        std::string format = "/run/vdr-addon-nonroot-XXXXXX";
        std::vector<char> name(format.begin(), format.end());
        name.push_back('\0');
        const char* created = ::mkdtemp(name.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        root = created;
        handler = root / "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import";
        approval = root / "etc/vdr-suite/addon-approvals/rectools.json";
        manifest = root / "usr/share/vdr-suite/addons/rectools/addon.json";
        fs::create_directories(handler.parent_path());
        fs::create_directories(approval.parent_path());
        fs::create_directories(manifest.parent_path());

        // Only exact, new fixture paths are chmodded. Never the OS tree.
        for (const char* part : {"", "usr", "usr/libexec",
                "usr/libexec/vdr-suite", "usr/libexec/vdr-suite/addons",
                "usr/libexec/vdr-suite/addons/rectools",
                "etc", "etc/vdr-suite", "etc/vdr-suite/addon-approvals",
                "usr/share", "usr/share/vdr-suite",
                "usr/share/vdr-suite/addons",
                "usr/share/vdr-suite/addons/rectools"}) {
            if (::chmod((root / part).c_str(), 0755) != 0)
                throw std::runtime_error("fixture directory chmod failed");
        }
        {
            std::ofstream file(handler, std::ios::binary);
            file << "nonexecuting media tools handler fixture\n";
        }
        {
            std::ofstream file(approval, std::ios::binary);
            file << "{\"id\":\"rectools\",\"origin\":\"manual\","
                    "\"package\":\"vdr-suite-addon-media-tools\","
                    "\"schemaVersion\":1,\"sha256\":\""
                 << Digest << "\",\"state\":\"approved\","
                    "\"version\":\"0.1.0\"}\n";
        }
        {
            std::ofstream file(manifest, std::ios::binary);
            file << "{\"schemaVersion\":1,\"id\":\"rectools\",\"version\":\"0.1.0\","
                    "\"package\":\"vdr-suite-addon-media-tools\",\"state\":\"scaffold\","
                    "\"displayName\":\"Media Tools\",\"description\":\"Inactive\","
                    "\"entrypoints\":[],\"capabilities\":[],\"permissions\":[],"
                    "\"dependencies\":[]}\n";
        }
        if (::chmod(manifest.c_str(), 0644) != 0 ||
            ::chmod(handler.c_str(), 0755) != 0 ||
            ::chmod(approval.c_str(), 0644) != 0)
            throw std::runtime_error("fixture file chmod failed");
    }
    ~Fixture() { std::error_code error; fs::remove_all(root, error); }
};

bool asUnprivileged(const fs::path& root) {
    const passwd* account = ::getpwnam("nobody");
    if (!account || account->pw_uid == 0) return false;
    const uid_t uid = account->pw_uid;
    const gid_t gid = account->pw_gid;
    const pid_t child = ::fork();
    if (child < 0) return false;
    if (child == 0) {
        if (::setgroups(0, nullptr) != 0 || ::setgid(gid) != 0 ||
            ::setuid(uid) != 0 || ::geteuid() == 0)
            ::_exit(2);
        const auto snapshot = vdrsuite::addons::inspectMediaToolsInstallSnapshot(
            root.string());
        ::_exit(snapshot.bytesVerified() ? 0 : 1);
    }
    int status = 0;
    if (::waitpid(child, &status, 0) != child || !WIFEXITED(status))
        return false;
    return WEXITSTATUS(status) == 0;
}
}

int main()
{
    Fixture fixture;
    assert(asUnprivileged(fixture.root));
    ::chmod(fixture.approval.c_str(), 0600);
    assert(!asUnprivileged(fixture.root));
    ::chmod(fixture.approval.c_str(), 0644);
    ::chmod(fixture.handler.c_str(), 0700);
    assert(!asUnprivileged(fixture.root));
    ::chmod(fixture.handler.c_str(), 0755);
    assert(asUnprivileged(fixture.root));
}
