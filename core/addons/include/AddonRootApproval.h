#pragma once

#include "AddonProviderAdmission.h"

#include <string>

namespace vdrsuite::addons {

// Root-owned on-disk approval, never a manifest or HTTP parameter.
// Result is non-executing installation evidence only, not a grant.
enum class RootApprovalStatus {
    approved, revoked, missing, invalidRoot, unsafeDirectory,
    unsafeRecord, invalidRecord, changedDuringRead, ioError
};

struct RootApprovalResult {
    RootApprovalStatus status = RootApprovalStatus::missing;
    InstallOrigin origin = InstallOrigin::absent;
    std::string version;
    std::string sha256;
    bool approved() const { return status == RootApprovalStatus::approved; }
};

// Reads ONLY <root>/etc/vdr-suite/addon-approvals/rectools.json.
// Accepts the exact canonical format emitted by admin_approval_issuer.py,
// with independently verified root ownership, safe modes and file identity.
// Backend scope is mandatory for Core authority; empty scope is for the
// existing backend-local installation approval, never a Core fallback.
RootApprovalResult readRootMediaToolsApproval(
    const std::string& root, const std::string& backendId = {});

} // namespace vdrsuite::addons
