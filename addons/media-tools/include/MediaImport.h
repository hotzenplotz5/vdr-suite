#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace vdrsuite::mediatools {
namespace fs = std::filesystem;

// This component is not a public or actor-facing API. Only an authorized
// backend-local service may provide these roots in a later integration.
struct ImportRequest {
    fs::path sourceRoot;
    fs::path videoRoot;
    fs::path sourceFile;
};

struct ImportPlan {
    fs::path sourceRoot;
    fs::path videoRoot;
    fs::path sourceFile;
    fs::path titleDirectory;
    std::string title;
    std::uintmax_t sourceBytes = 0;
    std::uint64_t sourceDevice = 0;
    std::uint64_t sourceInode = 0;
    std::int64_t sourceMtimeSeconds = 0;
    std::int64_t sourceMtimeNanoseconds = 0;
};

struct ProcessResult {
    int exitCode = -1;
    std::string output;
};

// No shell interpolation: implementation uses execv with fixed executable paths.
struct ProcessRunner {
    virtual ~ProcessRunner() = default;
    virtual ProcessResult run(const std::vector<std::string>& argv, bool captureOutput) = 0;
};

class NativeProcessRunner final : public ProcessRunner {
public:
    explicit NativeProcessRunner(
        std::chrono::seconds maximum = std::chrono::seconds(540));
    ProcessResult run(const std::vector<std::string>& argv, bool captureOutput) override;
private:
    std::chrono::seconds maximum_;
};

ImportPlan inspectImport(const ImportRequest& request);

// Import is intentionally a separate call. Preflight is supplied by the
// backend-local caller; the CLI always passes the hard reserve policy.
fs::path executeRemuxImport(const ImportPlan& plan, ProcessRunner& runner,
                            const std::function<void(const ImportPlan&)>& capacityPreflight);

void requireProductionCapacity(const ImportPlan& plan);

} // namespace vdrsuite::mediatools
