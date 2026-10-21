#include "PinnedMediaImportAgentExecutor.h"
#include "MediaImportRecordingEvidence.h"

#include <openssl/evp.h>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <sys/wait.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace vdrsuite::mediatools;

namespace {
std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string sha256(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("fixture binary read");
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx || EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
        throw std::runtime_error("sha init");
    char block[8192];
    while (stream)
    {
        stream.read(block, sizeof block);
        if (stream.gcount() > 0 &&
            EVP_DigestUpdate(ctx, block,
                static_cast<std::size_t>(stream.gcount())) != 1)
            throw std::runtime_error("sha update");
    }
    unsigned char bytes[EVP_MAX_MD_SIZE];
    unsigned count = 0;
    if (EVP_DigestFinal_ex(ctx, bytes, &count) != 1 || count != 32)
        throw std::runtime_error("sha final");
    EVP_MD_CTX_free(ctx);
    std::string hex;
    const char* digits = "0123456789abcdef";
    for (unsigned index = 0; index < count; ++index) {
        hex.push_back(digits[bytes[index] >> 4]);
        hex.push_back(digits[bytes[index] & 15]);
    }
    return hex;
}

struct Fixture {
    fs::path root, binary, approval;
    explicit Fixture(const char* fixtureBinary = "/bin/true")
    {
        if (::geteuid() != 0 || !std::getenv("TMPDIR") ||
            std::string(std::getenv("TMPDIR")) != "/root")
            throw std::runtime_error("CI root fixture only");
        struct stat parent{};
        if (::stat("/run", &parent) != 0 || !S_ISDIR(parent.st_mode) ||
            parent.st_uid != 0 || (parent.st_mode & 0022) != 0)
            throw std::runtime_error("unsafe test fixture parent");
        std::string pattern = "/run/vdr-a6-pinned-XXXXXX";
        std::vector<char> name(pattern.begin(), pattern.end());
        name.push_back('\0');
        if (!::mkdtemp(name.data())) throw std::runtime_error("mkdtemp");
        root = name.data();
        binary = root / "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import";
        approval = root / "etc/vdr-suite/addon-approvals/rectools.json";
        fs::create_directories(binary.parent_path());
        fs::create_directories(approval.parent_path());
        for (const char* dir : {"", "usr", "usr/libexec",
            "usr/libexec/vdr-suite", "usr/libexec/vdr-suite/addons",
            "usr/libexec/vdr-suite/addons/rectools", "etc",
            "etc/vdr-suite", "etc/vdr-suite/addon-approvals"})
            if (::chmod((root / dir).c_str(), 0755) != 0)
                throw std::runtime_error("chmod directory");
        // Real executable bytes, deliberately NOT a Media Import binary.
        // A successful isolated fd-exec can therefore mutate no recordings.
        fs::copy_file(fixtureBinary, binary);
        if (::chmod(binary.c_str(), 0755) != 0)
            throw std::runtime_error("chmod binary");
        writeApproval(false);
    }
    void writeApproval(bool revoked)
    {
        std::ofstream out(approval, std::ios::trunc);
        out << "{\"id\":\"rectools\",\"origin\":\"manual\","
               "\"package\":\"vdr-suite-addon-media-tools\","
               "\"schemaVersion\":1,\"sha256\":\""
            << sha256(binary) << "\",\"state\":\""
            << (revoked ? "revoked" : "approved")
            << "\",\"version\":\"0.1.0\"}\n";
        out.close();
        if (::chmod(approval.c_str(), 0644) != 0)
            throw std::runtime_error("chmod approval");
    }
    ~Fixture()
    {
        std::error_code error;
        fs::remove_all(root, error);
    }
};

struct Inventory final : ILocalMediaImportInventory {
    bool valid = true;
    bool swapped = false;
    bool swappedTarget = false;
    bool swappedIdentity = false;
    int resolutions = 0;
    bool resolve(const std::string& backend, const std::string& sourceId,
                 LocalMediaImportSource& out) override
    {
        ++resolutions;
        if (backend != "backend" || sourceId != "inventory_17") return false;
        out = {"/ci-only/media-in",
               swappedTarget ? "/ci-only/other-destination" : "/ci-only/media-out",
               !valid ? "/ci-only/media-in/../private" :
               swapped ? "/ci-only/media-in/another.ts" :
               "/ci-only/media-in/recording.ts"};
        out.sourceDevice = 10;
        out.sourceInode = swappedIdentity ? 99 : 11;
        out.sourceBytes = 1024;
        out.sourceMtimeNanoseconds = 42;
        return true;
    }
};
struct Grants final : ILocalMediaImportAuthorization {
    bool allowed = true;
    int checks = 0;
    bool granted(const BackendAgentCommandAssignment& assignment) override
    {
        ++checks;
        return allowed && assignment.operationId == "op_a6" &&
            assignment.claimEpoch == 5 && assignment.backendGeneration == 19;
    }
};
BackendAgentCommandAssignment command()
{
    BackendAgentCommandAssignment a;
    a.present = true;
    a.requestId = "req_a6";
    a.correlationId = "corr_a6";
    a.operationId = "op_a6";
    a.jobId = "job_a6";
    a.attemptId = "att_a6";
    a.claimEpoch = 5;
    a.commandId = "cmd_a6";
    a.backendId = "backend";
    a.agentId = "agent";
    a.agentInstanceId = "instance";
    a.backendGeneration = 19;
    a.commandType = kMediaImportCommandType;
    a.payloadVersion = 1;
    a.payload = "{\"sourceId\":\"inventory_17\"}";
    a.verificationPolicy = "reconciliation_required";
    a.assignedAt = nowSeconds() - 1;
    a.deadline = nowSeconds() + 40;
    a.requestFingerprint = backendAgentCommandFingerprint(a);
    assert(backendAgentCommandValidAssignment(a));
    return a;
}
}

int main()
{
    Fixture fixture;
    Inventory inventory;
    Grants grants;
    PinnedMediaImportAgentExecutor executor(
        fixture.root.string(), "backend", 19, inventory, grants);
    auto a = command();
    assert(executor.available());
    assert(executor.authorize(a, "backend", 19));
    assert(executor.execute(a) == MediaImportAgentOutcome::submitted);
    assert(executor.execute(a) == MediaImportAgentOutcome::rejected);
    assert(grants.checks >= 2);

    assert(executor.authorize(a, "backend", 19));
    fixture.writeApproval(true);
    assert(executor.execute(a) == MediaImportAgentOutcome::rejected);
    fixture.writeApproval(false);

    // A previously authorized opaque inventory token must not be remapped
    // to different source bytes or a different output directory before exec.
    assert(executor.authorize(a, "backend", 19));
    inventory.swapped = true;
    assert(executor.execute(a) == MediaImportAgentOutcome::rejected);
    inventory.swapped = false;
    assert(executor.authorize(a, "backend", 19));
    inventory.swappedTarget = true;
    assert(executor.execute(a) == MediaImportAgentOutcome::rejected);
    inventory.swappedTarget = false;
    assert(executor.authorize(a, "backend", 19));
    inventory.swappedIdentity = true;
    assert(executor.execute(a) == MediaImportAgentOutcome::rejected);
    inventory.swappedIdentity = false;

    grants.allowed = false;
    assert(!executor.authorize(a, "backend", 19));
    assert(executor.execute(a) == MediaImportAgentOutcome::rejected);
    grants.allowed = true;

    inventory.valid = false;
    assert(!executor.authorize(a, "backend", 19));
    inventory.valid = true;
    assert(!executor.authorize(a, "backend", 20));

    a.payload = "{\"sourceId\":\"../../etc/passwd\"}";
    a.requestFingerprint = backendAgentCommandFingerprint(a);
    assert(!executor.authorize(a, "backend", 19));
    assert(executor.execute(a) == MediaImportAgentOutcome::rejected);

    // The normal Agent process runs unprivileged. Prove that the same
    // root-owned approval and pinned executable work after dropping every
    // supplemental group and uid/gid in this disposable CI fixture.
    const passwd* nobody = ::getpwnam("nobody");
    assert(nobody != nullptr && nobody->pw_uid != 0);
    const pid_t child = ::fork();
    assert(child >= 0);
    if (child == 0)
    {
        if (::setgroups(0, nullptr) != 0 ||
            ::setgid(nobody->pw_gid) != 0 ||
            ::setuid(nobody->pw_uid) != 0 ||
            ::geteuid() == 0)
            ::_exit(2);
        Inventory unprivilegedInventory;
        Grants unprivilegedGrants;
        PinnedMediaImportAgentExecutor unprivileged(
            fixture.root.string(), "backend", 19,
            unprivilegedInventory, unprivilegedGrants);
        const auto operation = command();
        if (!unprivileged.available() ||
            !unprivileged.authorize(operation, "backend", 19) ||
            unprivileged.execute(operation) != MediaImportAgentOutcome::submitted)
            ::_exit(3);
        ::_exit(0);
    }
    int status = 0;
    assert(::waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);

    // This separate reviewed synthetic ELF outputs a committed .rec receipt
    // but writes no files. It proves the actual pinned fd/pipe parser.
    const char* receiptBinary = std::getenv(
        "MEDIA_IMPORT_TEST_RECEIPT_BINARY");
    assert(receiptBinary != nullptr && *receiptBinary != '\0');
    Fixture receiptFixture(receiptBinary);
    Inventory receiptInventory;
    Grants receiptGrants;
    PinnedMediaImportAgentExecutor receiptExecutor(
        receiptFixture.root.string(), "backend", 19,
        receiptInventory, receiptGrants);
    const auto receiptAssignment = command();
    assert(receiptExecutor.authorize(receiptAssignment, "backend", 19));
    assert(receiptExecutor.execute(receiptAssignment) ==
        MediaImportAgentOutcome::submitted);
    const std::string proof = receiptExecutor.takeRecordingEvidence();
    assert(proof == recordingPathEvidence(
        "/ci-only/media-out/Media/2026-10-21.18.00.00-0.rec"));
    assert(validRecordingPathEvidence(proof));
    assert(receiptExecutor.takeRecordingEvidence().empty());
}
