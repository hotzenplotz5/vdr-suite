#include "AddonPinnedHandler.h"
#include "AddonInstallSnapshot.h"
#include "AddonRootApproval.h"

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
constexpr const char* Digest =
    "a0711fa9587ab14350cbfb43a45f97bf4157e0378c85fc22cf5cdded3e6779cc";

struct Fixture {
    fs::path root, approval, handler, manifest;
    Fixture() {
        if (::geteuid() != 0 || std::getenv("TMPDIR") == nullptr ||
            std::string(std::getenv("TMPDIR")) != "/root")
            throw std::runtime_error("root-owned isolated TMPDIR=/root required");
        std::string format = "/root/vdr-addon-approval-XXXXXX";
        std::vector<char> name(format.begin(), format.end());
        name.push_back('\0');
        const char* directory = ::mkdtemp(name.data());
        if (!directory) throw std::runtime_error("mkdtemp failed");
        root = directory;
        handler = root / "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import";
        approval = root / "etc/vdr-suite/addon-approvals/rectools.json";
        manifest = root / "usr/share/vdr-suite/addons/rectools/addon.json";
        fs::create_directories(handler.parent_path());
        fs::create_directories(approval.parent_path());
        fs::create_directories(manifest.parent_path());
        writeManifest();
        {
            std::ofstream binary(handler, std::ios::binary);
            binary << "nonexecuting media tools handler fixture\n";
        }
        if (::chmod(handler.c_str(), 0700) != 0)
            throw std::runtime_error("fixture chmod failed");
        write("approved", Digest);
    }
    void writeManifest(const std::string& version = "0.1.0",
                       const std::string& capabilities = "[]") {
        std::ofstream out(manifest, std::ios::trunc | std::ios::binary);
        out << "{\"schemaVersion\":1,\"id\":\"rectools\",\"version\":\""
            << version
            << "\",\"package\":\"vdr-suite-addon-media-tools\","
               "\"state\":\"scaffold\",\"displayName\":\"Media Tools\","
               "\"description\":\"Inactive metadata only\","
               "\"entrypoints\":[],\"capabilities\":"
            << capabilities << ",\"permissions\":[],\"dependencies\":[]}\n";
        out.close();
        if (::chmod(manifest.c_str(), 0644) != 0)
            throw std::runtime_error("manifest chmod failed");
    }
    void write(const std::string& state, const std::string& digest,
               const std::string& version = "0.1.0") {
        std::ofstream out(approval, std::ios::trunc | std::ios::binary);
        out << "{\"id\":\"rectools\",\"origin\":\"manual\","
               "\"package\":\"vdr-suite-addon-media-tools\","
               "\"schemaVersion\":1,\"sha256\":\""
            << digest << "\",\"state\":\"" << state
            << "\",\"version\":\"" << version << "\"}\n";
        out.close();
        if (::chmod(approval.c_str(), 0644) != 0)
            throw std::runtime_error("approval chmod failed");
    }
    ~Fixture() { std::error_code ec; fs::remove_all(root, ec); }
};
}

int main()
{
    using namespace vdrsuite::addons;
    Fixture fixture;
    auto approval = readRootMediaToolsApproval(fixture.root.string());
    assert(approval.approved());
    assert(approval.origin == InstallOrigin::manual);
    assert(approval.version == "0.1.0");
    assert(approval.sha256 == Digest);
    auto binary = inspectPinnedMediaToolsHandler(fixture.root.string(),
                                                  approval.sha256);
    assert(binary.verified());
    auto snapshot = inspectMediaToolsInstallSnapshot(fixture.root.string());
    assert(snapshot.bytesVerified());
    assert(snapshot.origin == InstallOrigin::manual);
    assert(snapshot.approvedVersion == "0.1.0");
    assert(snapshot.pinnedBinary.get() >= 0);

    // Installed metadata, not only approved binary bytes, must be present
    // and match the same source/manual-install version. Never activate a
    // handler solely because an approval and executable still exist.
    fs::remove(fixture.manifest);
    assert(inspectMediaToolsInstallSnapshot(fixture.root.string()).reason ==
           InstallSnapshotReason::invalidMetadata);
    fixture.writeManifest("0.2.0");
    assert(!inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());
    fixture.writeManifest("0.2.0");
    fixture.write("approved", Digest, "0.2.0");
    assert(!inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());
    fixture.write("approved", Digest);
    fixture.writeManifest("0.1.0", "[\"addons.media.import\"]");
    assert(!inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());
    fixture.writeManifest();
    ::chmod(fixture.manifest.c_str(), 0666);
    assert(!inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());
    ::chmod(fixture.manifest.c_str(), 0644);
    ::chmod(fixture.manifest.parent_path().c_str(), 0777);
    assert(!inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());
    ::chmod(fixture.manifest.parent_path().c_str(), 0755);
    const fs::path saved = fixture.manifest.string() + ".saved";
    fs::rename(fixture.manifest, saved);
    fs::create_symlink(saved.filename(), fixture.manifest);
    assert(!inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());
    fs::remove(fixture.manifest);
    fs::rename(saved, fixture.manifest);
    assert(inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());

    fixture.write("revoked", Digest);
    assert(readRootMediaToolsApproval(fixture.root.string()).status ==
           RootApprovalStatus::revoked);
    assert(!inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());
    fixture.write("approved", std::string(64, '0'));
    assert(readRootMediaToolsApproval(fixture.root.string()).approved());
    assert(!inspectPinnedMediaToolsHandler(fixture.root.string(),
                                            std::string(64, '0')).verified());
    assert(!inspectMediaToolsInstallSnapshot(fixture.root.string()).bytesVerified());
    fixture.write("approved", Digest);

    ::chmod(fixture.approval.c_str(), 0666);
    assert(readRootMediaToolsApproval(fixture.root.string()).status ==
           RootApprovalStatus::unsafeRecord);
    ::chmod(fixture.approval.c_str(), 0644);
    ::chmod(fixture.approval.parent_path().c_str(), 0777);
    assert(readRootMediaToolsApproval(fixture.root.string()).status ==
           RootApprovalStatus::unsafeDirectory);
    ::chmod(fixture.approval.parent_path().c_str(), 0700);

    const fs::path original = fixture.approval.string() + ".original";
    fs::rename(fixture.approval, original);
    fs::create_symlink(original.filename(), fixture.approval);
    assert(!readRootMediaToolsApproval(fixture.root.string()).approved());
    fs::remove(fixture.approval);
    fs::rename(original, fixture.approval);

    {
        std::ofstream out(fixture.approval, std::ios::trunc);
        out << "{\"state\":\"approved\"}\n";
    }
    assert(readRootMediaToolsApproval(fixture.root.string()).status ==
           RootApprovalStatus::invalidRecord);
    assert(readRootMediaToolsApproval("../unsafe").status ==
           RootApprovalStatus::invalidRoot);
}
