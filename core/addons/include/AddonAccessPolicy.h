#pragma once

#include "BackendAccessPolicy.h"
#include "SecurityIdentity.h"

#include <string>
#include <vector>

namespace vdrsuite::addons {

// All fields are core/backend evidence, never copied from untrusted
// addon.json. This is an evaluation-only contract, not a module loader.
struct RuntimeEvidence {
    std::string moduleId;
    std::string packageName;
    std::string backendId;
    bool installed = false;
    bool manifestValid = false;
    bool packageTrusted = false;
    bool versionCompatible = false;
    bool administratorEnabled = false;
    bool handlerRegistered = false;
    bool handlerHealthy = false;
};

enum class AccessReason {
    allowed,
    unauthenticated,
    grantsUnavailable,
    notAdministrator,
    unknownModule,
    invalidInstallation,
    untrustedPackage,
    incompatibleVersion,
    disabled,
    handlerUnavailable,
    backendUnavailable,
    permissionDenied,
    unsupportedAction
};

struct AccessDecision {
    bool allowed = false;
    AccessReason reason = AccessReason::unauthenticated;
};

// Only a global administrator can inspect package inventory.
// No API routes or registered handlers are exposed by this policy.
AccessDecision canInspectInventory(const RequestSecurityContext& actor);

// Demonstrates the full per-operation admission conjunction, but cannot be
// invoked from public routes until a core-reviewed handler, persistent
// administration, and source-side authorization are connected. The only
// presently recognized operation is media.import on the rectools module.
//
// The required "addons.media.import" permission is deliberately not added to
// the supported grant vocabulary in this slice: no live actor can obtain
// that grant via canonical account/device administration yet.
AccessDecision canUseMediaImport(
    const RequestSecurityContext& actor,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence);

} // namespace vdrsuite::addons
