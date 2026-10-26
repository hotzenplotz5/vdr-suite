#pragma once

#include <string_view>

namespace vdrsuite::addons {

// Compile-time reviewed identifiers. This registry declares identities ONLY;
// it cannot register executable modules or turn package/file diagnostics
// into RuntimeEvidence.packageTrusted.
enum class HandlerTransport { none, backendAgent };

struct HandlerDefinition {
    std::string_view moduleId;
    std::string_view packageName;
    std::string_view binaryPath;
    HandlerTransport transport;
    bool executable;
};

// The pinned CLI/receipt protocol is reviewed only for this implementation
// version. A newer add-on package is not implicitly compatible with Core.
inline constexpr std::string_view kMediaToolsHandlerVersion = "0.1.0";

inline constexpr HandlerDefinition kMediaToolsCandidate{
    "rectools",
    "vdr-suite-addon-media-tools",
    "/usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import",
    HandlerTransport::backendAgent,
    false, // no reviewed runtime execution/Agent adapter yet
};

inline constexpr const HandlerDefinition* reviewedHandler(
    std::string_view moduleId, std::string_view packageName)
{
    return moduleId == kMediaToolsCandidate.moduleId &&
                   packageName == kMediaToolsCandidate.packageName
        ? &kMediaToolsCandidate : nullptr;
}

} // namespace vdrsuite::addons
